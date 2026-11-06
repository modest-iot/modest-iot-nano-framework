# Improvement 5: Diagnostics, Logging and Sensor Health

Analysis for a future development session. Status: **Phases A & B complete** (logging, counters, diagnostics, first-drop/failure logging, sensor fault events, and sensor fault conversion implemented; resilience proposed). Findings come from reading `src/`; memory and overhead figures are estimates, not measurements.

Related: [improvements-roadmap.md](improvements-roadmap.md), [sampling-interval-improvement.md](sampling-interval-improvement.md) (`missedTickCount`, clamp reports), [telemetry-improvement.md](telemetry-improvement.md) (queue counters), [connectivity-improvement.md](connectivity-improvement.md) (send status, link events), [threading-model-improvement.md](threading-model-improvement.md), [interrupt-power-improvement.md](interrupt-power-improvement.md).

---

## 1. Problem

The other improvement documents each propose their own counters and messages: missed ticks and clamped periods (sampling), dropped packets and queue high-water (telemetry), send failures (connectivity). None of them can report anything today, because **the framework has no diagnostics facility at all**.

Verified in the source:

| Finding | Evidence |
|---|---|
| Logger implemented | `ModestLog.h` provides `MODEST_LOG_*` macros with compile-time level filtering, ESP32 `esp32-hal-log` backend, host pluggable sink, and `MIOT_LOG_*` aliases. Host tests in `tests/log/test.cpp`. |
| Shared counters implemented | `Device` tracks `packetsEnqueued`, `packetsDropped`, `queueHighWaterMark`, and `missedTicks`. `DiagnosticsSnapshot` struct returned by `Device::diagnostics()` includes heap, stack, and queue metrics. Host tests in `tests/diagnostics/test.cpp`. |
| Sensor failures report health events | `DhtSensor` (NaN readings), `LoadCellAmplifier` (100 ms ready timeout), `UltrasonicSensor` (echo timeout), and `DigitalLightIntensitySensor` (I²C read failure) all call `reportReadFailure()`. Valid reads call `reportReadSuccess()`. Fault events (ID 22) and recovery events (ID 23) are emitted based on configurable thresholds. |
| Runtime health is invisible | Nothing reports free heap, task stack high-water marks, uptime, reset reason, or Wi-Fi signal strength |
| Silent resource failures | Queue full, packet allocation failure, MQTT publish false: each is a `bool` that the examples ignore |
| Sampler hang is undetected | The sampler task blocks on `ulTaskNotifyTake(..., portMAX_DELAY)`; if a handler hangs, nothing resets the device |

Consequence: in the field, a Modest IoT device that fails looks healthy, and during development the only debugging tool is the developer's own `Serial.println`.

## 2. Goals and non-goals

**Goals**
1. One small logging facility with levels and tags, compiled out when unused.
2. Sensors report faults and recoveries as framework events, so the mediator can decide what to do.
3. A shared, cheap counter mechanism that the scheduler, queue and gateways all use.
4. Optional runtime health snapshot (heap, stacks, uptime) that can also be sent as telemetry.
5. Optional watchdog coverage for the sampler.
6. Zero behaviour change for existing sketches unless they opt in.

**Non-goals**
- Remote log shipping, log storage in flash, or a tracing framework.
- A full state machine for sensor recovery (retry and reset policy belongs to the application).
- Replacing ESP-IDF logging.

## 3. Design options

### 3.1 Logging

| Option | Description | Pros | Cons |
|---|---|---|---|
| **L1: wrap `log_e/log_w/log_i/log_d`** | Framework macros `MODEST_LOG_E(tag, fmt, ...)` that expand to the Arduino-ESP32 `log_*` macros | No new infrastructure; reuses ESP32 color and level handling | Level is set by the IDE's *Core Debug Level* board option, not by the sketch; the ESP32 macros are unavailable on the host |
| **L2: own logger with a sink interface** | `Log::setSink(LogSink*)`, runtime level, default sink writes to `Serial` | Works on the host (fake sink), sketch-controlled level, can route to MQTT or a ring buffer | More code; two logging systems in one binary |
| **L3: macros mapping to either** | `MODEST_LOG_*` macros default to L1 on ESP32 and to a user-provided sink when `MODEST_IOT_LOG_SINK` is defined | Best of both, optional | Slightly more indirection |

**Recommendation: L3**, starting with L1 behaviour. Rules:
- Compile-time level `MODEST_IOT_LOG_LEVEL` (default: warnings and errors) so that disabled levels cost no code and no strings.
- Log only from non-ISR contexts. Never log from the timer ISR or from an interrupt handler (see the interrupt document).
- Each message carries a tag (`"Device"`, `"Scheduler"`, `"MQTT"`, a sensor name). Use `printf`-style formatting, with no `String` allocation.
- A single line per call, so output from the sampler and network tasks does not interleave mid-line (the underlying `log_*` call writes one buffered line; verify on target).
- Host tests provide a capturing sink so log output can be asserted.

### 3.2 Sensor health events

Add framework-wide identifiers next to `DATA_READ_EVENT_IDENTIFIER = 20` and `MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER = 21` in `Sensor.h`:

```cpp
static constexpr int SENSOR_FAULT_EVENT_IDENTIFIER     = 22;  // sourceId = pin or logical id
static constexpr int SENSOR_RECOVERED_EVENT_IDENTIFIER = 23;
```

Add protected helpers to the `Sensor` base class:

```cpp
void reportReadFailure(Event& event);   // counts consecutive failures; emits FAULT once on threshold
void reportReadSuccess(Event& event);   // resets the count; emits RECOVERED once after a fault
void setFaultThreshold(uint8_t consecutiveFailures);   // 0 = disabled (default)
```

Behaviour:
- Failure paths in `DhtSensor`, `LoadCellAmplifier`, `UltrasonicSensor` and the light sensor call `reportReadFailure` instead of silently setting `-1`.
- A fault event is emitted **once** when the threshold is crossed, and a recovery event once afterwards, not on every failed poll. This avoids flooding the mediator and the network.
- **Default disabled** (`threshold = 0`), so existing mediators never receive unknown events. Examples that benefit opt in explicitly.
- Counters per sensor: `readCount`, `failureCount`, `consecutiveFailures`, `lastFailureTimestamp`. Memory: about 16 bytes per sensor.
- The mediator decides policy. Examples: turn the LED red on fault; publish a health message; restart the I²C bus (a `Wire` recovery helper is a possible follow-up).

Compatibility: adding two event identifiers and non-virtual protected helpers does not change existing derived classes.

### 3.3 Shared counters

A tiny counter type used by every subsystem:

```cpp
struct Counter { volatile uint32_t value = 0; void increment() { value = value + 1; } uint32_t get() const { return value; } };
```

- **Single-writer rule:** each counter is written by exactly one task, so no lock is needed; readers in other tasks get a possibly slightly stale but never torn value (32-bit aligned reads and writes are atomic on ESP32). If a counter ever needs two writers, switch it to `std::atomic<uint32_t>` (lock-free on ESP32).
- Aggregated into a plain struct returned by value:

```cpp
struct DiagnosticsSnapshot {
    uint32_t missedTicks;          // sampling
    uint32_t worstCaseWakeMicros;  // sampling (high-water mark)
    uint32_t packetsEnqueued;      // telemetry
    uint32_t packetsDropped;       // telemetry
    uint8_t  queueHighWaterMark;   // telemetry
    uint32_t sendsSucceeded;       // gateway
    uint32_t sendsFailed;          // gateway
    uint32_t freeHeapBytes;        // runtime
    uint32_t minFreeHeapBytes;     // runtime
    uint16_t samplerStackFreeWords;   // runtime
    uint16_t networkStackFreeWords;   // runtime
    uint32_t uptimeSeconds;        // runtime
};
DiagnosticsSnapshot Device::diagnostics() const;
```

- A `Device::enableDiagnosticsLog(periodMs)` (opt-in) logs the snapshot periodically at info level from the network task (not the sampler, to avoid adding to its budget).
- Gateways report through the same type via a small `GatewayStatistics` member so that `sendsSucceeded/Failed` and the last `SendStatus` come from one place.

### 3.4 Runtime health

Sources, all available in the Arduino-ESP32 core:
- Heap: `ESP.getFreeHeap()`, `ESP.getMinFreeHeap()`, `ESP.getMaxAllocHeap()` (largest block, the fragmentation signal that matters for the telemetry pool decision).
- Stack margin: `uxTaskGetStackHighWaterMark(handle)` for each worker. Directly answers the open question of whether the 4096-byte sampler stack is enough once several handlers run per wake.
- Uptime: `millis()` (wraps at about 49.7 days; use a 64-bit tick count if long uptime matters) or `esp_timer_get_time()`.
- Reset reason: `esp_reset_reason()` read once at boot, so a watchdog or brownout reset is visible after the fact.
- Wi-Fi: `WiFi.RSSI()` through the connectivity driver (an optional `signalStrength()` on `ConnectivityDriver`).

Offer an optional ready-made `DeviceHealthPackage` (a `TelemetryPackage` subclass) so a device can publish health on a slow cadence with no custom code. Depends on the telemetry document's pool and writer decisions only for efficiency; it works with today's design.

### 3.5 Watchdog

- `HardwareWorker` blocks without timeout, so a hung handler cannot be detected. Change the wait to a bounded timeout (a small multiple of the tick) and feed the task watchdog each loop, so a stuck handler resets the device.
- Use `esp_task_wdt_add(nullptr)` from the task and `esp_task_wdt_reset()` on each iteration. The Arduino core only watches the `loop()` task by default, so this is an explicit, opt-in addition.
- Trade-off: a legitimately long handler (a 100 ms HX711 read, a slow display update) must stay under the watchdog timeout; choose the timeout from the handler budget in the sampling document (4.5), and make it configurable.
- Verify the API differences between core 3.0 and later (the task watchdog initialization signature changed in IDF 5.x).

## 4. Phases

**Phase A: visibility (small, no behaviour change)**
1. `[DONE]` `MODEST_LOG_*` macros (L3), host capture sink, tests. Implemented in `src/ModestLog.h` with `MIOT_LOG_*` aliases. Host tests in `tests/log/test.cpp`.
2. `[DONE]` Counters in `Device` (missed ticks, enqueued, dropped, high-water), the snapshot struct, `diagnostics()`. `DiagnosticsSnapshot` struct in `src/Device.h` with 12 fields. `Device::diagnostics()` queries FreeRTOS high-water marks and ESP heap stats. Host tests in `tests/diagnostics/test.cpp`.
3. `[DONE]` Runtime health fields and `uxTaskGetStackHighWaterMark` reporting. `diagnostics()` returns `freeHeapBytes`, `minFreeHeapBytes`, `samplerStackFreeWords`, `networkStackFreeWords`, and `uptimeSeconds`.
4. `[DONE]` Log the first drop and the first send failure at warning level. `Device::enqueueTelemetryPayload()` logs once when `packetsDroppedCount` transitions from 0 to 1. `MqttGatewayClient::sendTelemetryRecord()` logs once on the first failure with a descriptive message. `GatewayStatistics` struct in `GatewayClient.h` with `getStatistics()` virtual method; `MqttGatewayClient` tracks `sendsSucceeded`/`sendsFailed` and reports them via `diagnostics()`. Host tests in `tests/mqtt_gateway/test.cpp`.

**Phase B: sensor health**
5. `[DONE]` `Sensor` fault and recovery events, helpers, per-sensor counters (default off). Implemented in `src/Sensor.h` and `src/Sensor.cpp`: `SENSOR_FAULT_EVENT_IDENTIFIER` (22) and `SENSOR_RECOVERED_EVENT_IDENTIFIER` (23), with `reportReadFailure()`, `reportReadSuccess()`, and `setFaultThreshold()` helpers. The debounced threshold state machine emits fault events exactly once when the threshold is reached, and recovery events exactly once on the next successful read. Default threshold is 0 (disabled) for backward compatibility. Host tests in `tests/sensor_fault/test.cpp`.
6. `[DONE]` Convert `DhtSensor`, `LoadCellAmplifier`, `UltrasonicSensor`, `DigitalLightIntensitySensor`. Each sensor's `processEvent()` calls `reportReadFailure()` on read errors (NaN, timeout, I²C failure) and `reportReadSuccess()` on valid reads (including unchanged values). Suppressions for unchanged values still call `reportReadSuccess()` so the failure counter resets.
7. `[DONE]` One example opts in. `OutdoorSecurityLightController` sets `setFaultThreshold(3)` on both the light sensor and PIR sensor. When either sensor faults, the light turns off as a safe default. When the sensor recovers, normal operation resumes.

**Phase C: resilience**
8. Optional `DeviceHealthPackage` and periodic diagnostics log.
9. Sampler watchdog with a bounded wait.
10. Wi-Fi signal strength on the driver interface.

## 5. Interaction with the other documents

| Document | What it needs from here |
|---|---|
| Sampling | `missedTickCount`, worst-case wake duration, clamp reports, handler budget checks |
| Telemetry | `packetsDropped`, `queueHighWaterMark`, heap and largest-block data to justify the pool |
| Connectivity | send status counters, link up/down events (shares the identifier block with sensor health; see the identifier registry in the threading document) |
| Interrupt and power | ISR event counters (increments only, no logging in ISR), wake reasons |
| Library readiness | Host tests with the capture sink; CI sanitizers |

## 6. Risks

| Risk | Mitigation |
|---|---|
| Logging cost on the sampler task | Compile-time level, `printf`-style with no heap, never in ISRs, info-level periodic report from the network task |
| Event floods | Threshold with hysteresis, one fault and one recovery event per episode |
| Unknown events reach existing mediators | Default off; document the identifiers |
| Counter races | Single-writer rule; atomics where two writers exist |
| Watchdog false resets | Configurable timeout derived from the handler budget; opt-in |
| `Serial` contention with user code | Sink abstraction; users can route or disable |
| Stack growth from logging | Measure high-water marks; keep format buffers small |

## 7. Open decisions

| # | Decision | Recommended |
|---|---|---|
| 1 | Logger approach | L3, with L1 behaviour first |
| 2 | Default log level | Warning and above |
| 3 | Fault events default | Off, examples opt in |
| 4 | Fault threshold default when enabled | 3 consecutive failures |
| 5 | Counter type | Plain `volatile uint32_t` with single-writer rule |
| 6 | Watchdog | Opt-in in Phase C, after handler budgets are measured |
| 7 | Health telemetry | Optional ready-made package, off by default |
| 8 | Event identifiers 22 and 23 | Reserve in `Sensor.h` and in the identifier registry |

## 8. Acceptance criteria

- [ ] A logger exists with compile-time level control, tags, and a host capture sink; nothing logs from an ISR.
- [ ] `Device::diagnostics()` returns missed ticks, worst-case wake, queue counters, heap, minimum heap and both stack margins.
- [ ] DHT, HX711, ultrasonic and light failures can raise `SENSOR_FAULT` once per episode and `SENSOR_RECOVERED` after; default behaviour is unchanged.
- [ ] Host tests cover threshold, hysteresis, recovery and counter snapshots.
- [ ] At least one example reacts to a sensor fault.
- [ ] Sampler watchdog is available as opt-in with a documented timeout rule.
- [ ] README, class diagram, `keywords.txt` and Doxygen updated.
