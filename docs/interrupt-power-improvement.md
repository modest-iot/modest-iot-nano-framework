# Improvement 6: Interrupt-Driven Sensing and Power Management

Analysis for a future development session. Status: **proposed, not implemented.** Latency and power benefits are architectural reasoning; they must be measured (oscilloscope or logic analyzer for latency, a current meter for power) before any claim is published. Several ESP32 power-management facts depend on the Arduino-ESP32 core build and are marked "verify".

Related: [improvements-roadmap.md](improvements-roadmap.md), [sampling-interval-improvement.md](sampling-interval-improvement.md) (the scheduler this extends), [diagnostics-improvement.md](diagnostics-improvement.md) (ISR counters, wake reasons), [threading-model-improvement.md](threading-model-improvement.md) (ISR and task contexts).

---

## 1. Current design (verified)

- The only interrupt in the framework is the scheduler timer: `Device::handleHardwareTimerInterrupt` (an `IRAM_ATTR` handler that calls `vTaskNotifyGiveFromISR` on the sampler task). Verified: there is no `attachInterrupt` anywhere in `src/`.
- Every sensor is **polled**: the scheduler sends `MEASURE_DATA_REQUESTED` and the sensor reads its pin or bus on the sampler task, then reports a change (or suppresses the event).
- There is no sleep or power code: nothing in `src/` references `esp_sleep`, `esp_pm`, light sleep or deep sleep.
- The sampler task blocks on `ulTaskNotifyTake`, so when idle the CPU can run the FreeRTOS idle task, but the hardware timer keeps waking the system at the tick rate forever.

So the framework is event-driven **above** the sensor layer (events and commands) but polling-driven **at** it. That has three costs.

| Cost | Detail |
|---|---|
| Latency | An edge (button press, PIR trigger) is noticed on the next poll of that slot: up to one full period later, on average half of it. A button polled at 50 ms can feel sluggish; a slow sensor period makes this worse. |
| CPU and energy | A PIR or button sampled every 20 to 100 ms spends a wake-up and a sensor read per sample to learn, almost always, that nothing changed. |
| No sleep | Because the timer ticks continuously, the device can never usefully sleep, which rules out battery operation. |
| Missed short events | A pulse shorter than the polling period (a fast button tap, a brief PIR retrigger, a tachometer pulse) can be missed entirely. |

## 2. Goals and non-goals

**Goals**
1. Edge-type sensors can wake the framework on the event instead of being polled, while keeping the same `Event` interface to the mediator.
2. Polling stays available (and the default) for sensors that need it (analog, I²C, bit-banged protocols).
3. The scheduler can sleep between due slots (tickless operation), enabling light sleep and deep sleep cycles.
4. Everything is opt-in; existing sketches behave exactly as before.

**Non-goals**
- A general interrupt abstraction for user code outside sensors.
- Changing `Event` or `Command`.
- Dynamic frequency scaling as a requirement (see verify note in 4.4).

## 3. Interrupt-driven sensing

### 3.1 Candidates

| Sensor | Fit | Notes |
|---|---|---|
| `Button` | Excellent | GPIO edge interrupt plus debounce in the task (keep the existing debounce logic) |
| `PassiveInfraredSensor` | Excellent | Rising and falling edge, same reasoning |
| `LoadCellAmplifier` (HX711) | Good | The data-ready line (DOUT) falls when a conversion is ready. Interrupt on falling edge instead of a busy-wait of up to 100 ms (`READ_TIMEOUT_MS`) in the sampler; removes the largest blocking handler |
| `UltrasonicSensor` | Good, harder | Measure the echo width with an edge interrupt pair or the RMT/PCNT peripheral instead of blocking in `pulseIn` for up to 30 ms (`TIMEOUT_LIMIT_IN_MICROSECONDS`) |
| `GeolocationSensor` | Already buffered | UART RX is interrupt-driven inside the core's driver; polling the buffer is fine. Optionally wake on a new sentence |
| `PulseSensor`, `GasSensor` (analog) | No | Analog sampling needs periodic reads; keep polled (a timer-driven ADC/DMA design is out of scope) |
| `DhtSensor` | No | Bit-banged protocol with tight timing; keep polled |
| `DigitalLightIntensitySensor` (BH1750) | No | No interrupt pin; keep polled at its default interval |

### 3.2 Mechanism

The ISR does as little as possible: it records that "slot X needs servicing" and wakes the sampler. The sampler then runs the sensor's normal `processEvent` (the existing logic, including debounce and change detection) and forwards the resulting event to the mediator. No sensor logic moves into an ISR.

Scheduler extension (builds on the pure `SamplingScheduler` from the sampling document):

```cpp
// Registers a slot that is dispatched when triggered rather than on a period.
// Returns a trigger handle that is safe to call from an ISR.
TriggerHandle addTriggered(EventHandler* handler, int eventIdentifier);
void IRAM_ATTR trigger(TriggerHandle handle);   // ISR-safe: sets a bit and notifies the task
```

Sensors opt in with a method such as `enableInterruptMode(Device&)` or a constructor flag, which calls `attachInterruptArg(pin, isr, this, CHANGE)` with an `IRAM_ATTR` static trampoline, and registers its triggered slot.

#### Waking the sampler without losing tick counts
Today the tick ISR uses `vTaskNotifyGiveFromISR` (counting semantics), and the sampling document relies on the count returned by `ulTaskNotifyTake` for elapsed ticks. Mixing trigger bits into the same notification breaks that. Two clean options:

| Option | Description | Trade-off |
|---|---|---|
| **N1** | Notify with bit flags (`xTaskNotifyFromISR(handle, bit, eSetBits)`) for all sources; keep elapsed ticks in an ISR-incremented `std::atomic<uint32_t> pendingTicks` that the task exchanges to zero | Preserves tick counts, one wake path; needs a small atomic |
| **N2** | A second notification index (`xTaskNotifyIndexed...`) for triggers | Cleaner, but multiple notification indexes depend on `configTASK_NOTIFICATION_ARRAY_ENTRIES` in the core's FreeRTOS build. **Verify** before relying on it |
| **N3** | A small queue of trigger events (`xQueueSendFromISR`) | Preserves event order and count; slightly heavier, needs a queue |

**Recommendation: N1**, with the pending-tick atomic. Confirm lock-free atomics for `uint32_t` on the target (they are on Xtensa and RISC-V ESP32 variants).

### 3.3 Constraints (ISR rules)
- Mark handlers `IRAM_ATTR`; keep them tiny; no heap allocation, no `Serial`, no logging, no FreeRTOS calls other than the `FromISR` variants. The existing timer handler is the model.
- Use `portYIELD_FROM_ISR` when a higher-priority task was woken (the sampler is priority 3).
- Contact bounce: a bouncing button can raise dozens of interrupts. Rate-limit by disabling the pin interrupt in the ISR and re-enabling it after the debounce time from the task, or by ignoring triggers closer than the debounce window.
- Coalescing: several edges before the task runs produce one dispatch. For a button, the task reads the pin level and applies the existing debounce, so no information needed by the mediator is lost. For a pulse counter (tachometer, rain gauge) a coalesced dispatch would lose counts, so those need an ISR-side counter instead; call this out as a limitation.
- Never call `digitalRead` or other peripheral drivers that take locks from the ISR unless confirmed safe on the target (`digitalRead` is commonly used in ESP32 ISRs, but keep the ISR to setting a bit).

### 3.4 Benefits to quantify
- Edge-to-event latency: from up to the polling period down to the task wake-up time (tens of microseconds to a millisecond, depending on what else is running on core 1).
- CPU: no periodic read of idle buttons or PIRs; the slot disappears from the scheduler's periodic table and from the wake-duration budget.
- Blocking: removing the HX711 busy-wait (up to 100 ms) and the ultrasonic `pulseIn` (up to 30 ms) from the sampler path shrinks the worst-case wake, which directly relaxes the handler budget in the sampling document (4.5).

## 4. Power management

### 4.1 Tickless scheduling
A fixed tick wakes the system every tick regardless of what is due. With the pure scheduler the next due time is known, so the single timer alarm can be programmed for that moment instead of a fixed period:
- `SamplingScheduler::ticksUntilNextDue()` returns the smallest `ticksUntilDue` (and `UINT32_MAX` when there are no periodic slots).
- After each dispatch the sampler re-arms the one-shot alarm for that interval.
- Example: a security light with a PIR on interrupt and a BH1750 every 1000 ms wakes once per second instead of every 10 to 50 ms.
- Required scheduler change: stop treating the tick as a fixed period; keep the tick as a granularity unit. This needs the pure scheduler and elapsed-tick accounting from the sampling document first.

### 4.2 Sleep modes (ESP32)

| Mode | Wake sources | State kept | Use |
|---|---|---|---|
| Light sleep (`esp_light_sleep_start`) | Timer, GPIO, UART | RAM and CPU state preserved; tasks resume | Mains-powered or solar devices that need fast response; Wi-Fi association is lost unless modem-sleep configuration is used |
| Deep sleep (`esp_deep_sleep_start`) | Timer, `ext0`/`ext1` GPIO, touch | RAM lost except RTC memory (`RTC_DATA_ATTR`) | Battery devices that wake on a PIR or button, do their work, and go back to sleep |
| Modem sleep / automatic light sleep | Managed by the SDK | Everything | Lower idle current while staying connected. **Verify**: automatic light sleep and dynamic frequency scaling need `CONFIG_PM_ENABLE`, which the precompiled Arduino-ESP32 libraries may not enable, so this may require a custom core build |

### 4.3 A `PowerPolicy` for `Device`
Opt-in and deliberately small:

```cpp
enum class PowerMode : uint8_t { AlwaysOn, LightSleepWhenIdle, DeepSleepCycle };
void setPowerMode(PowerMode mode, uint32_t maximumSleepMilliseconds);
```

Rules the framework must enforce before sleeping:
1. The telemetry queue is empty and the network worker is idle (no send in progress), or a configurable grace period has expired.
2. No slot is due within the minimum useful sleep time (avoid sleep and wake thrash).
3. The next wake sources are armed: the timer for the next due slot plus the GPIO wake pins of interrupt-driven sensors (a sensor exposes its wake pin and level).

Deep-sleep cycle additions:
- Application state survives only in RTC memory: provide a documented `RTC_DATA_ATTR` pattern, not hidden state.
- The boot path must re-create devices and sensors quickly; recommend measuring boot-to-first-sample time.
- Report the wake reason (`esp_sleep_get_wakeup_cause()`) to the mediator as an event, so the application knows whether a PIR or the timer woke it.
- Wi-Fi reconnection costs most of a battery budget; recommend batching telemetry (send every N wakes) and storing readings in RTC memory between sends.

### 4.4 Sensor and peripheral power
- Sensors that support it should expose power-down: the BH1750 has a power-down command and a one-time measurement mode (already analyzed in an earlier session as the basis for a high-resolution one-shot mode), which avoids continuous conversion between samples.
- Allow a sensor to be powered from a GPIO (switchable VCC) via an optional `powerGpioPin`, with a documented settling time constant next to the interval constants. Out of scope for the first iteration.
- The HX711 has a power-down mode by holding the clock line high for more than 60 microseconds. The existing source already notes the 60 microsecond figure in `LoadCellAmplifier::reset`.

## 5. Phases

**Phase A: interrupt triggers (prerequisite: pure scheduler from the sampling document)**
1. Notification redesign (N1) and the `addTriggered`/`trigger` API, tested on the host with a fake notifier.
2. `Button` and `PassiveInfraredSensor` opt-in interrupt mode, with debounce and coalescing tests.
3. Example: security light with the PIR on interrupt (lowers CPU and latency); measure.

**Phase B: remove the blocking handlers**
4. `LoadCellAmplifier` data-ready interrupt.
5. `UltrasonicSensor` echo capture (RMT/PCNT or edge pair). Validate against the existing `pulseIn` behaviour.

**Phase C: power**
6. Tickless alarm using `ticksUntilNextDue()`.
7. `PowerMode::LightSleepWhenIdle` with the guards in 4.3.
8. `PowerMode::DeepSleepCycle`, wake-reason event, RTC state pattern, battery-oriented example (PIR wake, publish, sleep).
9. Document measured numbers (current in each mode) in the README.

## 6. Risks

| Risk | Mitigation |
|---|---|
| ISR bugs are hard to diagnose | Keep ISRs to one atomic update and one notify; host-test the dispatch logic; hardware-test with a logic analyzer |
| Contact bounce storms | Disable the pin interrupt during the debounce window |
| Lost counts on coalesced triggers | Document, and provide an ISR-side counter for pulse-counting use cases |
| Sleep interferes with Wi-Fi and MQTT sessions | Sleep only when the queue is empty and the link state allows it; reconnection logic belongs to the connectivity document |
| Automatic light sleep may be unavailable in the precompiled core | Verify first; fall back to explicit light and deep sleep |
| Light sleep stops peripherals and timers | Use `esp_sleep_enable_timer_wakeup` rather than depending on the general-purpose timer across sleep; verify behaviour on the target core |
| Behavioural change in examples | Everything is opt-in; existing sketches keep polling and a fixed tick |
| Wokwi cannot measure power | Use Wokwi only for logic; measure on hardware |

## 7. Open decisions

| # | Decision | Recommended |
|---|---|---|
| 1 | Notification mechanism | N1 (bit flags plus an atomic pending-tick counter) |
| 2 | First sensors to convert | `Button`, `PassiveInfraredSensor` |
| 3 | Opt-in style | Per-sensor `enableInterruptMode`, never automatic |
| 4 | Tickless | Yes, after the pure scheduler exists |
| 5 | Sleep modes in the first release | Light sleep first; deep sleep as a separate, battery-focused example |
| 6 | Ultrasonic via RMT or PCNT | Evaluate after the HX711 conversion; may be a separate document |
| 7 | Switchable sensor power | Defer |

## 8. Acceptance criteria

- [ ] An ISR-safe trigger path exists and preserves the elapsed-tick count of periodic slots.
- [ ] `Button` and `PassiveInfraredSensor` can run in interrupt mode with the same events as polling mode, covered by host tests for debounce and coalescing.
- [ ] Measured edge-to-event latency is recorded for polling versus interrupt mode.
- [ ] HX711 and ultrasonic handlers no longer block the sampler for tens of milliseconds (or the document records why not).
- [ ] Tickless mode reduces wake-ups for a slow-sensor example; the numbers are recorded.
- [ ] Light sleep (and deep sleep, if implemented) are available only when explicitly enabled, and measured current is documented.
- [ ] README and Doxygen document ISR rules and the opt-in API.
