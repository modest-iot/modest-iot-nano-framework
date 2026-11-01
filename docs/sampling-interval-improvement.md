# Improvement 1: Per-Slot Sampling Intervals

Implementation brief for a future development session. It is self-contained: read it, then implement.

Status: **proposed, not implemented.** Claims about performance are architectural reasoning, not measurements.

---

## 1. Problem

`Device` runs one hardware timer. Each tick wakes `HardwareWorker`, which services **exactly one** slot and advances a round-robin pointer (`src/Device.cpp`, `executeHardwareSamplingContext`).

Consequences:

| Issue | Detail |
|---|---|
| Coupled rates | A sensor's period is `tick × registeredSlotCount`. Adding a sensor slows all others. |
| No per-sensor timing | A button (needs ~20 ms) and a DHT22 (needs ≥ 2 s) cannot coexist at correct rates. |
| Silent tick loss | `ulTaskNotifyTake(pdTRUE, …)` clears the count and returns it. Ticks missed while a handler blocks are coalesced and discarded. |
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
- Tests exist only for `digital_light` and `rgb_led`, built with host fakes. There is no `Device` test and no scheduler test.

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

    struct Slot {
        EventHandler* sensor = nullptr;
        int eventIdentifier = -1;
        uint32_t periodInTicks = 1;
        uint32_t ticksUntilDue = 0;
    };

    explicit SamplingScheduler(uint32_t baseTickInMilliseconds);

    bool add(EventHandler* sensor, int eventIdentifier, uint32_t periodInMilliseconds,
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
- Initial phase: `ticksUntilDue = (slotIndex % periodInTicks)` so slots with equal periods do not all fire on the same tick (staggering).
- `advance(n)`:
  - For each slot, if `ticksUntilDue <= n`, the slot is due. Dispatch **once** (do not replay missed samples; a sensor read is a present-time observation, not a backlog).
  - Re-arm: `ticksUntilDue = periodInTicks - ((n - ticksUntilDue) % periodInTicks)` so the cadence stays aligned to the original phase with no drift.
  - Otherwise `ticksUntilDue -= n`.
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
bool appendSensorToScheduler(EventHandler* sensor, int eventId);                       // existing
bool appendSensorToScheduler(EventHandler* sensor, int eventId, uint32_t periodMs);   // new
```

Decision required (see §10): what does the **two-argument** form mean?

| Option | Behaviour | Trade-off |
|---|---|---|
| **A (recommended)** | Period = the old effective period `tick × slotCountAtInit`, computed at `initializeAsynchronousEngine()` | Existing examples behave exactly as before. Needs deferred period resolution. |
| B | Period = one tick | Simple, but existing sensors suddenly sample N× faster. Breaking. |
| C | Remove the overload | Cleanest API, breaks every example. |

Recommendation: Option A via a `legacyPeriod` sentinel, mark the overload `[[deprecated]]` after one release, and migrate the examples in the same change.

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

| Sensor | Default (ms) | Minimum (ms) | Basis to verify |
|---|---|---|---|
| `Button` | 20–50 | 10 | Debounce behaviour |
| `PassiveInfraredSensor` | 50–100 | ~20 | Module hold time |
| `DigitalLightIntensitySensor` (BH1750) | 1000 | 180 | Conversion time (high-res, up to ~180 ms) |
| `HumidityAndTemperatureSensor` (DHT) | 2000 | 2000 | DHT22 limit 0.5 Hz (DHT11 is 1 Hz) |
| `UltrasonicSensor` | 100 | ~60 | Echo decay and cross-talk |
| `PulseSensor` | 20 | 10 | Waveform resolution needed |
| `GasSensor` | 1000 | ~100 | Warm-up and response time |
| `LoadCellSensor` (HX711) | 100 | ~100 | 10 SPS mode (80 SPS mode is ~12.5 ms) |
| `GpsSensor` | 1000 | 100 | Module update rate |

Also needed: a way for `Device` to enforce minimums. Options:
1. `add(..., minimumPeriodMs)` is passed explicitly by the caller (`Sensor::MINIMUM…`). Simple, no virtuals, ROM-free.
2. A `virtual uint32_t minimumSamplingIntervalInMilliseconds() const` on `Sensor`. Automatic, but costs a vtable slot per sensor and changes the base interface.

Recommendation: start with option 1, revisit option 2 only if callers forget to pass the minimum.

DHT note: if the minimum cannot be met, the sensor should return the cached reading rather than hit the bus. This is a separate, optional hardening.

## 5. Timing model and edge cases

| Case | Required behaviour |
|---|---|
| Period shorter than tick | Clamped to one tick, reported |
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

- `Device::appendSensorToScheduler` can still return `bool` (`true` for `Ok` and clamped), and log the clamp via the framework's logging facility if one exists. **Check for an existing logger before inventing one.**

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
| 1 | Meaning of the two-argument overload | Option A (preserve old effective period), deprecate |
| 2 | Constant suffix | `_IN_MILLISECONDS` (matches the existing long-name style) |
| 3 | Minimum enforcement | Explicit parameter now, virtual getter later if needed |
| 4 | Clamp reporting | `AddResult` enum internally, `bool` publicly |
| 5 | Replay vs coalesce missed samples | Coalesce (dispatch once) |
| 6 | Scheduler as a public class or a private detail | Public header, documented as advanced API, so it is host-testable |
| 7 | Raise `MAXIMUM_SLOTS` above 8 | Not yet, revisit after the table is cheap |

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
6. Equal periods are staggered (no tick where all fire if staggering is possible).
7. `advance(7)` on a 5-tick slot: dispatched once, next due 3 ticks later (phase preserved, no drift).
8. `advance(0)`: no dispatch.
9. Table full returns `TableFull`; null sensor and event `-1` return `InvalidArgument`.
10. Counter wrap safety: run 2^32/… is impractical, so run with a large `ticksUntilDue` near `UINT32_MAX` and check no overflow.
11. Dispatch order equals registration order.
12. Legacy path (Option A): `N` legacy slots resolve to `periodInTicks = N` and reproduce the old round-robin cadence.

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
- [ ] Two-argument overload behaves as before, and is marked deprecated.
- [ ] Every polled sensor has `DEFAULT_` and `MINIMUM_` constants with cited sources.
- [ ] All examples migrated, and `OutdoorSecurityLightController` constructor arguments fixed.
- [ ] README, class diagram, `keywords.txt` and Doxygen updated, and Doxygen runs without warnings.
- [ ] Existing `digital_light` and `rgb_led` tests still pass.
- [ ] Compiled for ESP32 in CI or on another machine, with measured periods within one tick of declared.

## 15. Follow-ups (not part of this change)

- Document the threading model (priorities, which task runs which handler).
- Telemetry packet pool to reduce heap churn, and a blocking or counted enqueue instead of silent drops.
- Actuator default constants.
- Hardware benchmark sketch.
- Wokwi projects and `wokwi-cli` CI scenarios.
- Library-readiness cleanup (remove `main.cpp` and the empty `.ino`, ignore `.DS_Store` and `cmake-build-arduino/`, trim `depends=`, tag `v1.0.0`).
