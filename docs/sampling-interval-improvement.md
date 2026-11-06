# Improvement 1: Per-Slot Sampling Intervals

Implementation brief for a future development session. It is self-contained: read it, then implement.

Status: **Phases A & B complete** (SamplingScheduler implemented, wired into Device, interval constants added to all sensor headers, Hybrid approach with virtual sampling interval getters and Sensor* overload implemented, [[deprecated]] removed from EventHandler* overload, host tests passing, examples migrated). Phase C (docs, README, class diagram) remains.

Related: [improvements-roadmap.md](improvements-roadmap.md) (priorities and dependencies), [threading-model-improvement.md](threading-model-improvement.md) (what runs where, handler budgets), [diagnostics-improvement.md](diagnostics-improvement.md) (counters and logging used here), [interrupt-power-improvement.md](interrupt-power-improvement.md) (the alternative to polling for edge sensors), [connectivity-improvement.md](connectivity-improvement.md), [telemetry-improvement.md](telemetry-improvement.md).

---

## 1. Problem

`Device` runs one hardware timer. Each tick wakes `HardwareWorker`, which services **exactly one** slot and advances a round-robin pointer (`src/Device.cpp`, `executeHardwareSamplingContext`).

Consequences:

| Issue | Detail |
|---|---|
| Coupled rates | A sensor's period is `tick × registeredSlotCount`. Adding a sensor slows all others. |
| No per-sensor timing | A button (needs ~20 ms) and a DHT22 (needs ≥ 2 s) cannot coexist at correct rates. |
| Silent tick loss | `ulTaskNotifyTake(pdTRUE, …)` clears the count and returns it. The current worker ignores that returned count, so ticks missed while a handler blocks are lost. |
| Timing knowledge is implicit | Datasheet limits live in the developer's head, not in the library. |
| Examples drift | Every example hard-codes a `SAMPLING_INTERVAL_MS` that is really "tick", with an implicit divisor. |

## 2. Goals and non-goals

**Goals**
1. Each slot declares its own period in milliseconds.
2. Sensor classes expose sensible default and minimum intervals.
3. Missed ticks are counted, not lost.
4. Scheduling logic is testable on the host (no FreeRTOS or ESP32 headers).
5. Backward compatible API for existing examples, with an explicit, documented behaviour change (see §8).

**Non-goals (separate work)**
- One FreeRTOS task per sensor.
- Dynamic slot registration or removal at runtime.
- Actuator default constants (settle time, tone duration).
- Replacing the hardware timer with `esp_timer` or `vTaskDelayUntil`.
- Cross-core or multi-timer scheduling.

## 3. Current code facts (verified)

- `src/Device.h`: `ScheduledTaskSlot { EventHandler* sensorInstance; int targetEventIdentifier; }`, `MAXIMUM_SCHEDULED_SLOTS = 8`, `registeredSlotCount`, `activeSequencePointer`, `pollingIntervalInMilliseconds` (const), `hardwareTimerIndex`.
- `src/Device.cpp`:
  - Timer set up with `timerBegin(1000000)`, `timerAttachInterruptArg`, `timerAlarm(...)`. This is the **Arduino-ESP32 core 3.x API**. Core 2.x differs.
  - `hardwareTimerIndex` is stored but **never used**.
  - ISR does `vTaskNotifyGiveFromISR` to the sampling task.
  - `HardwareWorker`: priority 3, 4096 B stack, core 1. `NetworkWorker`: priority 1, 8192 B, core 1.
  - `appendSensorToScheduler` is not guarded against being called after `initializeAsynchronousEngine`.
  - `enqueueTelemetryPayload` uses a zero timeout, so a full queue drops data silently.
- `Sensor::on()` calls `processEvent()`, stops when `event.identifier == -1`, then forwards the event to its parent. Sensors are "polled" by sending `Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER`.
- Examples calling `appendSensorToScheduler` (all must be reviewed): `AirConditioningDevice`, `AirQualityMonitor`, `ClassicBlinker`, `DriverAssistantDevice`, `OutdoorSecurityLightController`, `PerimeterMonitor`, `SmartBandMqtt`, `SmartScaleDevice`, `ToggableLedDevice`.
- Tests exist for 10 suites under `tests/fakes/` shared harness: digital_light, rgb_led, rfid_sensor, ble_connectivity, log, mqtt_gateway, engine_config, diagnostics, sensor_fault, scheduler. There is no `Device` test.

### Pre-existing issue to fix in the same session
`examples/OutdoorSecurityLightController/OutdoorSecurityLightController.ino:56` calls `Device(SAMPLING_INTERVAL_MS, DEVICE_EVENT_DELAY_MS)`. The second parameter is `timerGroupIndex`, so a delay constant is being passed as a timer index. It is harmless today only because the index is unused. Fix it when touching the example.

## 4. Design

### 4.1 Pure scheduler table (new)

Extract the timing logic into a class with no platform dependency, so it compiles on the host.

```cpp
// src/SamplingScheduler.h  (header-only or .h + .cpp, no Arduino/FreeRTOS includes)
class SamplingScheduler {
public:
    static constexpr uint8_t MAXIMUM_SLOTS = 8;

    enum class AddResult : uint8_t { Ok, ClampedToMinimum, ClampedToTick, TableFull, InvalidArgument };

    struct Slot {
        EventHandler* handler = nullptr;   // a Sensor, or any EventHandler (ClassicBlinker registers the Device itself)
        int eventIdentifier = -1;
        uint32_t periodInTicks = 1;
        uint32_t ticksUntilDue = 1;        // always >= 1: ticks remaining until the slot fires
    };

    explicit SamplingScheduler(uint32_t baseTickInMilliseconds);

    AddResult add(EventHandler* handler, int eventIdentifier, uint32_t periodInMilliseconds,
                  uint32_t minimumPeriodInMilliseconds = 0);

    // Advances time by `elapsedTicks` and invokes every due slot.
    // Returns the number of slots dispatched.
    uint8_t advance(uint32_t elapsedTicks);

    uint8_t count() const;
    const Slot& slotAt(uint8_t index) const;
};
```

Rules:
- `periodInTicks = max(1, ceil(periodMs / baseTickMs))`. Use integer ceiling, never float.
- Period `0` means "every tick".
- Period below the minimum: **clamp up to the minimum and report it** (see §6). Do not silently accept.
- Countdown semantics: `ticksUntilDue` is the number of ticks remaining until the slot fires and is **always >= 1**. A slot fires on the tick on which its countdown reaches zero.
- Initial phase: `ticksUntilDue = 1 + (offset % periodInTicks)`, where `offset` is the number of slots already registered with the **same period**. Equal-period slots therefore first fire on consecutive ticks (1, 2, 3, ...), and each then repeats every `periodInTicks`. The first sample is taken on the first tick (offset 0), which matches today's behaviour. A "warm-up" variant (first fire after one full period) is a possible per-slot option for slow sensors such as the DHT, not the default.
- `advance(n)` (`n >= 1`; `advance(0)` is a no-op):
  - If `n >= ticksUntilDue`, the slot is due. Dispatch **once** (do not replay missed samples; a sensor read is a present-time observation, not a backlog). Then set `overshoot = n - ticksUntilDue` and re-arm with `ticksUntilDue = periodInTicks - (overshoot % periodInTicks)`. This keeps the cadence aligned to the original phase with no drift, even when `n` spans several periods.
  - Otherwise `ticksUntilDue -= n`.
- Worked example, period 5, offset 0: `ticksUntilDue = 1`. `advance(1)` fires (tick 1) and re-arms to 5, so the next fire is at tick 6: a steady five-tick interval. After that, `advance(7)` fires once, overshoot is 2, and the next fire comes 3 ticks later.
- Overflow safety: all counters `uint32_t`, countdown only (no absolute-time comparisons), so `millis()` rollover is not an issue.
- Dispatch order: slot registration order. Document it.
- Reentrancy: a handler must not call `add()`. Registration is only valid before the engine starts (see §4.3).

### 4.2 `Device` integration

- `Device` owns a `SamplingScheduler` built from the constructor's tick.
- `executeHardwareSamplingContext`:

```cpp
uint32_t elapsed = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);   // count of ticks since last wake
self->scheduler.advance(elapsed);
```

- Remove `activeSequencePointer` and the per-tick single dispatch.
- Keep the 4096 B stack unless measurement of `uxTaskGetStackHighWaterMark` says otherwise, because several handlers now run per wake.
- Track overruns: if `elapsed > 1`, increment a `missedTickCount` (cheap diagnostic counter, readable from a protected getter).

### 4.3 Public API

Keep the existing overload and add one:

```cpp
bool appendSensorToScheduler(EventHandler* handler, int eventId);                                            // existing
bool appendSensorToScheduler(EventHandler* handler, int eventId, uint32_t periodMs, uint32_t minimumMs = 0); // new
```

The `minimumMs` parameter is what makes minimums enforceable: the `Device` overload forwards it to `SamplingScheduler::add`. Callers pass the sensor's constant:

```cpp
appendSensorToScheduler(&lightSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER,
                        DigitalLightIntensitySensor::DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS,
                        DigitalLightIntensitySensor::MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS);
```

That is verbose, so a convenience template is worth considering (compile-time, no virtuals): `appendSensorToScheduler<DigitalLightIntensitySensor>(&lightSensor, eventId)` uses `T::DEFAULT_…` and `T::MINIMUM_…`, and `appendSensorToScheduler<T>(&sensor, eventId, periodMs)` uses the explicit period but still enforces `T::MINIMUM_…`. It fails to compile for classes without the constants, which flags the missing information early. It does not apply to `DhtSensor` (see §4.4); the template needs a per-model hook there.

Decision required (see §10): what does the **two-argument** form mean?

| Option | Behaviour | Trade-off |
|---|---|---|
| **A (recommended)** | Period = the old effective period `tick × slotCountAtInit`, computed at `initializeAsynchronousEngine()` | Existing examples behave exactly as before. Needs deferred period resolution. |
| B | Period = one tick | Simple, but existing sensors suddenly sample N× faster. Breaking. |
| C | Remove the overload | Cleanest API, breaks every example. |

Recommendation: Option A via a `legacyPeriod` sentinel, mark the overload `[[deprecated]]` after one release, and migrate the examples in the same change.

Legacy resolution rule (so that mixed registrations are well defined): at `initializeAsynchronousEngine()`, let `L` be the number of legacy (two-argument) slots. Each legacy slot gets `periodInTicks = L` and an initial `ticksUntilDue = 1 + legacyOrdinal`, which reproduces today's round-robin exactly (slot 0 on tick 1, slot 1 on tick 2, ..., then every `L` ticks). Slots with explicit periods are unaffected. With no explicit slots this equals `tick × slotCount`, the current behaviour. Note that `ClassicBlinker` registers the `Device` itself through the legacy overload, so it must keep working.

Guards:
- `appendSensorToScheduler` returns `false` after `initializeAsynchronousEngine()`. Track with a `bool engineStarted`.
- Returns `false` on null sensor or full table (as today).

### 4.4 Per-class interval constants

Add to every polled sensor, in its header:

```cpp
static constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS = 1000;
static constexpr uint32_t MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 180;
```

Naming: use the framework's long-form `_IN_MILLISECONDS` suffix, consistent with the existing constants style (decision to confirm, §10).

Provisional values, **all must be checked against datasheets before committing** and each must carry a `@note` with its source:

Class names are the ones in `src/`:

| Class | Default (ms) | Minimum (ms) | Basis to verify |
|---|---|---|---|
| `Button` | 20–50 | 10 | Debounce behaviour |
| `PassiveInfraredSensor` | 50–100 | ~20 | Module hold time |
| `DigitalLightIntensitySensor` (BH1750) | 1000 | 180 | Conversion time (high-res, up to ~180 ms) |
| `DhtSensor` | per model, see below | per model | DHT22 limit 0.5 Hz (2000 ms), DHT11 1 Hz (1000 ms) |
| `UltrasonicSensor` | 100 | ~60 | Echo decay and cross-talk |
| `PulseSensor` | 20 | 10 | Waveform resolution needed |
| `GasSensor` | 1000 | ~100 | Warm-up and response time |
| `LoadCellAmplifier` (HX711) | 100 | ~100 | Output rate depends on the RATE pin wiring: 10 SPS (100 ms) or 80 SPS (12.5 ms). Document the assumption. |
| `GeolocationSensor` | 1000 | 100 | Module update rate (typically 1 Hz) |

**`DhtSensor` is model-dependent.** The model (`DHT11`, `DHT22`, ...) is a constructor argument, so one pair of constants would be wrong for one of them. Expose per-model constants (`DHT11_MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS`, `DHT22_...`) plus a `static constexpr uint32_t minimumSamplingIntervalFor(uint8_t dhtModelType)` helper, or an instance getter that the example passes to `appendSensorToScheduler`. Choose when implementing and record the datasheet source for each model.

Also needed: a way for `Device` to enforce minimums. Options:
1. `minimumMs` passed explicitly by the caller (§4.3). Simple, no virtuals.
2. A `virtual uint32_t minimumSamplingIntervalInMilliseconds() const` on `Sensor`. Automatic, and the flash cost is negligible (one vtable entry per sensor class). The real limit is that the scheduler holds an `EventHandler*`, and `EventHandler` is not a `Sensor`: handlers such as `ClassicBlinker` register the `Device` itself, so the scheduler would need a cast or a second registration path.
3. A compile-time template overload (§4.3): automatic for sensors, no virtuals, and no change to `EventHandler`.

Recommendation: option 1 as the base mechanism, plus option 3 as the ergonomic layer. Revisit option 2 only if a runtime polymorphic need appears.

DHT note: if the minimum cannot be met, the sensor should return the cached reading rather than hit the bus. This is a separate, optional hardening.

## 4.5 Handler time budget (new, from the source)

All due slots run sequentially inside one wake, so the **sum of the worst-case handler times of the slots that can be due on the same tick must be shorter than the tick**, or `missedTickCount` grows. Worst cases measured from the source (not from hardware):

| Handler | Worst-case blocking | Source |
|---|---|---|
| `LoadCellAmplifier` | up to 100 ms busy-wait for data-ready, then 24 bit-banged clock pulses | `READ_TIMEOUT_MS = 100`, `readRawValue()` loop on `digitalRead` |
| `UltrasonicSensor` | up to 30 ms inside `pulseIn`, plus the trigger delays | `TIMEOUT_LIMIT_IN_MICROSECONDS = 30000` |
| `DigitalLightIntensitySensor` | I²C transaction (sub-millisecond to a few ms; longer on a bus fault) | `Wire` reads |
| `GeolocationSensor` | drains the serial buffer; time grows with the backlog | `updateInternalBuffer()` `while` loop |
| `DhtSensor` | roughly 20 to 25 ms bit-banged read, with interrupts disabled during bit timing | DHT library |
| `Button`, `PassiveInfraredSensor`, `PulseSensor`, `GasSensor` | microseconds (single read) | `digitalRead` / `analogRead` |

Consequences for the design:
- The **SmartScale** example registers four `LoadCellAmplifier` slots. With a 100 ms tick, four back-to-back worst-case reads could take about 400 ms in one wake. Explicit per-sensor periods must therefore be **staggered**, which the initial-phase rule does, and the document's default for `LoadCellAmplifier` (100 ms) should not be used for four load cells on one tick without staggering or a slower tick.
- The scheduler should expose `worstCaseWakeInMicroseconds` (measured with `micros()` around each dispatch, high-water mark) so a developer sees the real budget. Records feed the diagnostics counters.
- A later refinement is a per-handler declared budget (`MAXIMUM_BLOCKING_TIME_IN_MICROSECONDS` constant next to the interval constants) so that a build-time or start-up check can warn about an over-committed tick.
- Blocking sensors (ultrasonic, HX711) are the strongest argument for the optional per-slot task or the interrupt/DMA approaches in [interrupt-power-improvement.md](interrupt-power-improvement.md).

## 5. Timing model and edge cases

| Case | Required behaviour |
|---|---|
| Period shorter than tick | Clamped to one tick, reported (`ClampedToTick`) |
| Period below the sensor minimum | Clamped to the minimum first, then rounded up to a whole tick, reported (`ClampedToMinimum`) |
| Period not a multiple of tick | Rounded **up** to a whole tick. Document the effective period, expose it through `effectivePeriodInMilliseconds(slot)`. |
| Handler blocks longer than one tick | Next wake receives `elapsed > 1`. Slots dispatch once, cadence stays phase-aligned, `missedTickCount` increases. |
| Many slots due on the same tick | All dispatch in registration order within the same wake. Worst-case wake duration is the sum of their handlers, so document it. |
| Two sensors with the same period | Staggered initial phase |
| Zero slots | `advance` is a no-op |
| Table full (9th sensor) | `add` returns `false`. Consider raising `MAXIMUM_SLOTS` once the table is cheap. |
| Tick too coarse | Choose tick = GCD of the periods in use. Provide a helper `recommendedTickInMilliseconds()` or document the rule. |
| Event IDs are -1 | Rejected by `add` |

## 6. Reporting below-minimum requests

Silent clamping hides mistakes, so surface it:
- `add()` returns an enum, not a bool, or exposes `lastAddResult`:

```cpp
enum class AddResult : uint8_t { Ok, ClampedToMinimum, ClampedToTick, TableFull, InvalidArgument, EngineStarted };
```

- `Device::appendSensorToScheduler` can still return `bool` (`true` for `Ok` and for either clamp, `false` for `TableFull`, `InvalidArgument`, or an engine already started), and log the clamp via the framework's logging facility if one exists. There is **no logger in the framework today** (verified: no logging code in `src/`; `GasSensor` has its `Serial` calls commented out), so clamp reporting and `missedTickCount` depend on the logging and health facilities proposed in [diagnostics-improvement.md](diagnostics-improvement.md). Until those exist, expose the values through accessors only. Because the bool cannot distinguish the two clamps from `Ok`, also add a protected `lastSchedulerResult()` accessor (or a second overload that returns `AddResult`) so tests and diagnostics can see it.
- `EngineStarted` is a `Device`-level result (the scheduler does not know about the engine), so it is not in the scheduler's enum.

## 7. Concurrency and memory

- The table is written only before engine start, then read by the single sampler task. No lock needed. Enforce with `engineStarted`.
- The ISR touches only the task handle. Keep it that way.
- Memory: 8 slots × (pointer 4 + int 4 + 2×`uint32_t` 8) = 128 B. Negligible.
- `MAXIMUM_SLOTS` and `Slot` stay private to `Device` or the scheduler class, never in the public API.

## 8. Behaviour changes that must be called out in the README

1. Existing devices that register N sensors with the two-argument call keep the old effective period (Option A). The README must say this call is deprecated.
2. Sensors declared with explicit periods are independent of the slot count.
3. Handlers may now run back to back in one wake, so long handlers affect the next sensor in the same wake.
4. The tick is a granularity, not a sensor period.

## 9. Implementation plan

Order matters. Each step should leave the tree compiling and the tests passing.

1. **Test harness first.** Add `tests/scheduler/test.cpp` (plus a fake `EventHandler` recorder). Include only `SamplingScheduler.*`, `Event.h`, `EventHandler.h`. Confirm these headers have no Arduino dependency; if they do, add a minimal `tests/scheduler/fakes/Arduino.h` like the other suites.
2. **Write `SamplingScheduler`** to satisfy the tests in §11.
3. **Wire into `Device`:** new member, new worker body, `engineStarted` guard, `missedTickCount`, overload and `[[deprecated]]` marker.
4. **Add interval constants** to each sensor with datasheet citations.
5. **Migrate examples** to explicit periods using the constants. Set each `Device(tick, …)` to the GCD of its sensor periods. Fix the `OutdoorSecurityLightController` constructor arguments (§3).
6. **Docs:** Doxygen on every new symbol (follow `RgbLed.h` style: `@brief`, `@details`, `@param`, `@note`, `@code`), `docs/class-diagram.puml`, `README.md` (new "Sampling intervals" section and the behaviour notes of §8), `keywords.txt` (`SamplingScheduler`, constants), `docs/doxygen-groups.dox` if a new group is needed.
7. **Validate:** host tests, Doxygen with `docs/doxygen-groups.dox` as input, PlantUML parse, and a real compile (see §12).
8. **Optional:** Wokwi project for the security light for a behavioural check (`docs/` analysis from the previous session).

## 10. Open decisions

| # | Decision | Recommended |
|---|---|---|
| 1 | Meaning of the two-argument overload | `[RESOLVED]` `EventHandler*` overload is a valid fallback for non-Sensor handlers; `Sensor*` overload takes compiler precedence for Sensor arguments. `[[deprecated]]` removed. |
| 2 | Constant suffix | `_IN_MILLISECONDS` (matches the existing long-name style) |
| 3 | Minimum enforcement | `[RESOLVED]` Hybrid approach: virtual pure methods in `Sensor` base class (`getDefaultSamplingIntervalInMilliseconds`, `getMinimalSamplingIntervalInMilliseconds`) with per-instance override attributes. `Sensor*` overload queries the sensor's effective period polymorphically. |
| 4 | Clamp reporting | `AddResult` enum in the scheduler, `bool` from `Device` plus a diagnostic accessor |
| 5 | Replay vs coalesce missed samples | Coalesce (dispatch once) |
| 6 | Scheduler as a public class or a private detail | Public header, documented as advanced API, so it is host-testable |
| 7 | Raise `MAXIMUM_SLOTS` above 8 | Not yet, revisit after the table is cheap |
| 8 | First sample timing | First tick (current behaviour), with optional per-slot warm-up later |
| 9 | DHT minimum | Per-model constants and a helper, not one value |

## 11. Test plan (host)

Compile like the existing suites, e.g.:

```
c++ -std=c++17 -Wall -Itests/scheduler/fakes -Isrc tests/scheduler/test.cpp src/SamplingScheduler.cpp -o /tmp/scheduler-test && /tmp/scheduler-test
```

Cases:
1. Single slot, period = tick: dispatched every `advance(1)`.
2. Two slots, periods 1 and 5 ticks: over 100 ticks, counts are 100 and 20.
3. Period rounding: 150 ms on a 100 ms tick becomes 2 ticks.
4. Period below tick: clamped to 1 tick.
5. Below-minimum request: clamped and reported.
6. Two period-5 slots first fire on ticks 1 and 2, then every 5 ticks (staggered): fire ticks `[1, 6, 11]` and `[2, 7, 12]`.
7. Single period-5 slot: after `advance(1)` (fired at tick 1), `advance(7)` fires once at elapsed tick 8 and the next fire is at tick 11 (phase preserved, no drift).
8. `advance(0)`: no dispatch.
9. Table full returns `TableFull`; null sensor and event `-1` return `InvalidArgument`.
10. Large-count safety: `advance(UINT32_MAX)` and periods near `UINT32_MAX` do not overflow (use `uint64_t` in the intermediate arithmetic if needed) and keep `ticksUntilDue >= 1`.
11. Dispatch order equals registration order.
12. Legacy path (Option A): `L` legacy slots resolve to `periodInTicks = L` with initial countdowns `1..L` and reproduce the old round-robin order. A mix of legacy and explicit slots leaves the explicit slots unaffected.
13. `periodMs = 0` means every tick, and `baseTick = 0` is rejected at construction (or treated as 1; decide and test).
14. Same event on a second slot or the same handler registered twice is allowed and dispatches twice.

## 12. Verification on hardware or simulator

- `arduino-cli compile` currently **fails locally** (`ctags` "bad CPU type" on this machine), so use another machine, CI, or Wokwi.
- On an ESP32 (Arduino-ESP32 core 3.x), check:
  - Sensor periods measured with `micros()` stamps match the declared periods (± one tick).
  - `missedTickCount` stays at 0 under normal load.
  - `uxTaskGetStackHighWaterMark` of `HardwareWorker` has margin.
  - Free heap is stable over 24 h.
- A single-sensor benchmark sketch (motion to LED latency, loop jitter) can come after this change; it is separate work.
- If targeting Wokwi: it is not cycle-accurate, so use it for logic, not for jitter figures.

## 13. Risks

| Risk | Mitigation |
|---|---|
| Behaviour change in existing examples | Option A plus the README note |
| Several handlers in one wake delay each other | Document, add `missedTickCount`, keep handlers short |
| Wrong datasheet values encoded as constants | Require a source note per constant, review before merge |
| Timer API differs between core 2.x and 3.x | Already core-3.x only. State the requirement in `library.properties` and the README |
| `Device` still untestable on host | Scheduler extracted. A `Device` fake is out of scope |
| Constants becoming API promises | Mark as guidance (`@note`), allow override |

## 14. Acceptance criteria

- [ ] `SamplingScheduler` exists with no platform includes, and all §11 tests pass on the host.
- [ ] `Device` uses it. Elapsed ticks from `ulTaskNotifyTake` are honoured.
- [ ] New `appendSensorToScheduler(sensor, event, periodMs)` exists. Calls after engine start are rejected.
- [x] Two-argument overload behaves as before. `[[deprecated]]` removed; `EventHandler*` overload is a valid fallback for non-Sensor handlers, `Sensor*` overload takes compiler precedence for Sensor arguments.
- [x] Every polled sensor has `DEFAULT_` and `MINIMUM_` constants with cited sources (`DhtSensor` per model).
- [x] Minimums are enforceable through `appendSensorToScheduler`, and a below-minimum request is observable.
- [ ] A measured worst-case wake duration is available, and the SmartScale example is staggered so it stays within its tick.
- [x] All examples migrated. ClassicBlinker uses explicit periods via the four-argument `EventHandler*` overload. The other 9 examples automatically use the `Sensor*` overload, which resolves periods via virtual getters. `OutdoorSecurityLightController` constructor arguments were already correct.
- [ ] README, class diagram, `keywords.txt` and Doxygen updated, and Doxygen runs without warnings.
- [x] Existing `digital_light` and `rgb_led` tests still pass.
- [x] Compiled for ESP32, all 10 examples compile cleanly.

## 15. Follow-ups (not part of this change)

- Document the threading model: see [threading-model-improvement.md](threading-model-improvement.md).
- Telemetry pool and drop counters: see [telemetry-improvement.md](telemetry-improvement.md) and [diagnostics-improvement.md](diagnostics-improvement.md).
- Actuator default constants.
- Hardware benchmark sketch.
- Wokwi projects and `wokwi-cli` CI scenarios.
- Library readiness, CI and host tests: see [library-readiness-improvement.md](library-readiness-improvement.md). **Do this first**: the scheduler needs the host test harness and CI described there.
