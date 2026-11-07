# Modest IoT Improvements

Master index, implementation status, and detailed plans for all framework improvements.

All 8 improvement briefs and the roadmap are consolidated in this single document to eliminate cross-document maintenance overhead. Architecture decisions are recorded separately in [adrs.md](adrs.md).

---

## Status Overview

| Tier                                    | Items                                                  | Goal                                                                                                | Exit criterion                                                                                                       | Status       |
|-----------------------------------------|--------------------------------------------------------|-----------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------|--------------|
| 0: Make it correct and shippable        | W1, W2, W3, W4                                         | Everything compiles, repo is clean, rules are written down                                          | All examples compile cleanly; clean repo layout                                                                      | **Complete** |
| 1: Safety net and visibility            | W5                                                     | Regressions caught automatically via host tests & CI                                                | CI green on PRs/pushes; all 11 host test suites pass; examples compile in CI; lint passes                            | **Complete** |
| 2: Reliability, diagnostics & footprint | W6 (logging/high-water/sensor conversion), W7, W9, W10 | Reconnection robust, tasks tunable, BLE modern, zero-cost logs, sensor fault events with conversion | Soak test reconnects; logs available behind macro; BLE tests pass; diagnostics snapshot works; sensors report faults | **Complete** |
| 3: Core performance and DX              | W8, then W11                                           | Per-slot intervals, bounded memory use for telemetry                                                | Scheduler host tests pass; all examples use per-sensor intervals; telemetry ownership and queue policy complete      | **Complete** |
| 4: Capabilities                         | W12, W13, W14, W15, rest of W6                         | Edge-driven sensing, remote commands, fault events                                                  | Latency measured for polling versus interrupt                                                                        | Queued       |
| 5: Optimization and power               | W16, W17, W18, W19                                     | Battery operation and further tuning                                                                | Decide per item from measured data                                                                                   | Queued       |

## Work Item Registry

Prioritization method: each work item is scored 0 to 5 on three benefits, and sized by cost. Scores reflect architectural value; revisit them after diagnostics data is captured.

- **Q** quality: correctness, safety, testability, release credibility.
- **P** performance: latency, CPU, memory, energy.
- **D** developer experience: first-run success, API clarity, debuggability.
- **Cost**: XS under half a day, S about a day, M several days, L a week or more.
- **Score** = 2Q + P + 2D, divided by cost weight (XS 1, S 1.5, M 2.5, L 4), rounded.

| ID  | Work item                                                                               | Doc Section | Q | P | D | Cost | Score | Status                                                                                                                                      |
|-----|-----------------------------------------------------------------------------------------|-------------|---|---|---|------|-------|---------------------------------------------------------------------------------------------------------------------------------------------|
| W1  | Fix abstract `WiFiConnectivityDriver` (D1); examples must compile                       | Tier 0      | 5 | 0 | 5 | XS   | 20    | **Completed**                                                                                                                               |
| W2  | Delete dead `LoadCellController`, unused deps, unused includes, pin ArduinoJson         | Tier 0      | 4 | 1 | 4 | XS   | 17    | **Completed**                                                                                                                               |
| W3  | Repo cleanup (`main.cpp`, empty `.ino`, `.gitignore`), `library.properties` fixes       | Tier 0      | 4 | 0 | 4 | XS   | 16    | **Completed**                                                                                                                               |
| W4  | Threading rules, "Runs on" tags, ID registry (docs only)                                | Tier 0      | 3 | 0 | 4 | XS   | 14    | **Completed**                                                                                                                               |
| W5  | Host test harness plus CI (host tests, compile-examples, lint)                          | Tier 1      | 5 | 0 | 4 | M    | 7     | **Completed**                                                                                                                               |
| W6  | Logging macros, stack high-water, sensor fault events, sensor conversion                | Tier 2      | 4 | 1 | 5 | S    | 13    | **Completed**                                                                                                                               |
| W7  | Connectivity reconnect policy, `mqttClient.loop()` every cycle, link events             | Tier 2      | 5 | 2 | 4 | M    | 8     | **Completed**                                                                                                                               |
| W8  | Per-slot sampling intervals, pure scheduler, elapsed-tick accounting, example migration | Tier 3      | 4 | 5 | 5 | M    | 9     | **Completed**                                                                                                                               |
| W9  | `EngineConfig` and `stopAsynchronousEngine()`                                           | Tier 2      | 4 | 2 | 3 | S    | 11    | **Completed**                                                                                                                               |
| W10 | Optional Bluetooth, BLE driver addition, optional MQTT footprint                        | Tier 2      | 3 | 2 | 4 | S    | 11    | **Partially Complete** (BLE NUS added, BT Classic isolated)                                                                                 |
| W11 | Telemetry ownership and queue policy (Phases A and B)                                   | Tier 3      | 4 | 3 | 4 | M    | 8     | **Partially Complete** (Phase A done, `unique_ptr` overload, drop-oldest, bounded buffer, envelope metadata, golden tests, T14 name hiding) |
| W12 | `Device::post` command marshalling                                                      | Tier 4      | 3 | 0 | 4 | M    | 6     | Pending                                                                                                                                     |
| W13 | Interrupt triggers for Button and PIR                                                   | Tier 4      | 3 | 4 | 3 | M    | 6     | Pending                                                                                                                                     |
| W14 | HX711 data-ready and ultrasonic echo capture (remove blocking)                          | Tier 4      | 2 | 4 | 2 | M    | 5     | Pending                                                                                                                                     |
| W15 | Cloud-to-device commands (MQTT subscribe)                                               | Tier 4      | 2 | 0 | 4 | M    | 5     | Pending                                                                                                                                     |
| W16 | Tickless scheduling and light sleep                                                     | Tier 5      | 2 | 4 | 2 | L    | 3     | Pending                                                                                                                                     |
| W17 | Deep-sleep cycle and battery example                                                    | Tier 5      | 1 | 4 | 2 | L    | 3     | Pending                                                                                                                                     |
| W18 | Telemetry pool and writer interface (Phase C)                                           | Tier 5      | 2 | 3 | 1 | L    | 2     | Pending                                                                                                                                     |
| W19 | Native servo and LCD drivers                                                            | Tier 5      | 2 | 0 | 1 | M    | 2     | Pending                                                                                                                                     |

## Dependencies between items

- Interrupt triggers and `Device::post` share one notification redesign (bit flags plus an atomic tick counter) and require the pure scheduler.
- Tickless sleep requires `ticksUntilNextDue()` from the scheduler.
- Cloud-to-device commands (connectivity) require `Device::post` (threading).
- Stack-size defaults should come from diagnostics measurements.
- Telemetry writer and pool work should follow the host test harness.

## Deliberately out of scope

- Replacing DHT, PubSubClient, ArduinoJson or TLS.
- Targeted (per-instance) commands until a use case appears.
- Switchable sensor power rails and ADC/DMA sampling.
- Non-ESP32 targets.

## Claims verified and remaining verification gaps

- **Verified**: Local `arduino-cli compile` cleanly builds all 10 examples across ESP32 targets with `--library .` configured.
- **Verified**: `library.properties` `depends=` syntax formatted for Arduino Library Manager compliance.
- **Verified**: Bluetooth Classic fails on modern ESP32 variants (S2, S3, C3, C6, H2); resolved via target guards and `BleConnectivityDriver`.
- **Verified**: Host test harness executes 11 unit test suites (`tests/digital_light`, `tests/rgb_led`, `tests/rfid_sensor`, `tests/ble_connectivity`, `tests/log`, `tests/mqtt_gateway`, `tests/engine_config`, `tests/diagnostics`, `tests/sensor_fault`, `tests/scheduler`, `tests/telemetry`) under Address and Undefined sanitizers.
- **Verified**: Logging facility (`MODEST_LOG_*`) works with host capture sink and `MIOT_LOG_*` aliases.
- **Verified**: MQTT gateway reconnection and keep-alive verified with host tests.
- **Remaining**: Performance, latency, memory and power figures (reasoned, to be measured on hardware).
- **Remaining**: `Wire` thread-safety on core 3.x; `loop()` core affinity.
- **Remaining**: Task notification array availability; `CONFIG_PM_ENABLE` in the precompiled core.
- **Remaining**: Datasheet values behind the per-sensor interval constants.

---

## Tier 0: Compilation & Code Hygiene (Complete)

### W1: Fix abstract `WiFiConnectivityDriver` (D1)

`WiFiConnectivityDriver` did not implement the pure-virtual `transmit()` inherited from `ConnectivityDriver.h`, making it an abstract class that could not be instantiated.

**Resolution**: Implemented `WiFiConnectivityDriver::transmit()` to resolve the compilation blocker. All Wi-Fi-backed devices and gateways now instantiate properly.

**Source**: `src/WiFiConnectivityDriver.h`, `src/WiFiConnectivityDriver.cpp`

---

### W2: Delete dead `LoadCellController`, unused deps, unused includes, pin ArduinoJson

**Inventory** (verified from `src/` and `library.properties`):

| Dependency                | Used by                       | Declared in `depends=`   | Notes                                                                                |
|---------------------------|-------------------------------|--------------------------|--------------------------------------------------------------------------------------|
| `LiquidCrystal_I2C`       | `CharacterLcdDisplay`         | yes                      | Many forks with differing APIs                                                       |
| `DHT sensor library`      | `DhtSensor`                   | yes                      | Pulls in Adafruit Unified Sensor transitively                                        |
| `ESP32Servo`              | `ServoMotor`                  | yes                      |                                                                                      |
| `PubSubClient`            | `MqttGatewayClient`           | yes                      |                                                                                      |
| `ArduinoJson`             | `TelemetryPackage` (needed)   | yes, pinned to `>=7.0.0` | Code uses the v7 `JsonDocument` type                                                 |
| `HX711`                   | ~~`LoadCellController` only~~ | **Removed**              | `LoadCellController` deleted; `LoadCellAmplifier` implements HX711 protocol natively |
| `EspSoftwareSerial`       | nothing                       | **Removed**              | Not referenced anywhere                                                              |
| `Adafruit_Unified_Sensor` | nothing directly              | **Retained**             | Transitive via DHT library; kept for safety                                          |

**Resolution**:
- Deleted `LoadCellController.h/.cpp` and its `keywords.txt` entry.
- Removed `HX711_Arduino_Library` and `EspSoftwareSerial` from `library.properties`.
- Pinned `ArduinoJson (>=7.0.0)` in `library.properties`.
- Corrected dependency names for Arduino Library Manager compliance (`DHT sensor library`, `Adafruit Unified Sensor`).
- Removed unused `#include <ArduinoJson.h>` from `GatewayClient.h` and `MqttGatewayClient.cpp`.
- Cleaned drafting comments from `HttpGatewayClient.cpp`.

**Principles**:
1. Remove what is unused first. It costs nothing and cuts install friction.
2. Replace only simple, well-specified protocols, where a native implementation is a few dozen lines.
3. Never replace protocols with timing risk or large state machines (DHT, MQTT, TLS, JSON).
4. Always wrap before replacing. Introduce a small internal interface first.
5. Library Manager friction counts. Each extra `depends=` entry is auto-installed for every user.

**Source**: `library.properties`, `keywords.txt`

---

### W3: Repository cleanup

**Resolution**:
- Removed root clutter (`main.cpp`, `Modest-IoT-Nano-Framework.ino`).
- Updated `.gitignore` rules (`.DS_Store`, `/cmake-build-arduino/`, `/build/`, `*.o`).
- Aligned `CMakeLists.txt` `SKETCH_DIR` to `examples/ClassicBlinker` with cache variable.

**Source**: `.gitignore`, `CMakeLists.txt`

---

### W4: Threading rules, "Runs on" tags, ID registry (docs only)

**Resolution**:
- Added threading model section to README with context table, 5 threading rules, Engine Configuration example, and Identifier Registry.
- Added `@runs_on` Doxygen tags to public hooks in `Device.h`, `Sensor.h`, `Actuator.h`, and `GatewayClient.h`.
- Published identifier registry with reserved ranges (framework 0-999, user 1000+).

**Identifier Registry**:

| Space    | Range in use | Owner                                                |
|----------|--------------|------------------------------------------------------|
| Events   | 10           | `Button::BUTTON_PRESSED`                             |
| Events   | 20, 21       | `Sensor::DATA_READ`, `MEASURE_DATA_REQUESTED`        |
| Events   | 22, 23       | `Sensor::SENSOR_FAULT`, `SENSOR_RECOVERED`           |
| Events   | 30, 31       | `PassiveInfraredSensor` motion detected, stopped     |
| Events   | 40           | `GasSensor` digital gas detected                     |
| Events   | 50, 51       | `DigitalLightIntensitySensor` light detected, absent |
| Commands | 100 to 102   | `Led`                                                |
| Commands | 150 to 152   | `ServoMotor`                                         |
| Commands | 200 to 202   | `CharacterLcdDisplay`                                |
| Commands | 250 to 252   | `Buzzer`                                             |
| Commands | 300 to 303   | `RgbLed`                                             |

Reserved blocks: sensor-health 22-23, connectivity link events 900-909, power events 910-919. User identifiers from 1000.

**Source**: `README.md`, `src/Device.h`, `src/Sensor.h`, `src/Actuator.h`, `src/GatewayClient.h`

---

## Tier 1: Test Consolidation & CI (Complete)

### W5: Host test harness plus CI

**Shared Mock Harness**:
- Created centralized `tests/fakes/` with `Arduino.h`, `Wire.h`, `SPI.h`, `BLEDevice.h`, `BLEServer.h`, `BLEUtils.h`, `PubSubClient.h`, `WiFi.h`, `ArduinoJson.h`, `Client.h`.
- Eliminated duplicate per-suite fake directories.
- Resolved all Clang-Tidy warnings: deprecated C headers to C++ headers, unused includes, const/static qualifications.

**Test Runner & CI**:
- Implemented `tests/run_all.sh` with strict warnings (`-Wall -Wextra -Werror`) and Address/Undefined sanitizers.
- Added GitHub Actions workflows:
  - `host-tests.yml`: runs all 11 test suites on Ubuntu and macOS.
  - `compile-examples.yml`: compiles all 10 examples across ESP32 and ESP32-S3 boards.
  - `lint.yml`: validates `library.properties` with Arduino Library Manager compliance.
- Registered all test targets in `CMakeLists.txt` with CTest integration.

**Test Suites (11 total)**:

| #  | Suite                     | What It Tests                                                          |
|----|---------------------------|------------------------------------------------------------------------|
| 1  | `tests/DigitalLightIntensitySensor/` | I²C sensor polling, Wire mock, event emission                          |
| 2  | `tests/RgbLed/`          | PWM output, color mixing, analogWrite mock                             |
| 3  | `tests/RfidSensor/`      | CRC calculation, SPI mock, card communication                          |
| 4  | `tests/BleConnectivityDriver/` | BLE NUS GATT service, notifications, RX callbacks                      |
| 5  | `tests/ModestLog/`              | `MODEST_LOG_*` level filtering, sink redirection, `MIOT_LOG_*` aliases |
| 6  | `tests/MqttGatewayClient/`     | MQTT keep-alive, reconnect backoff, client state                       |
| 7  | `tests/DiagnosticsSnapshot/`      | `DiagnosticsSnapshot`, shared counters, enqueue edge cases             |
| 8  | `tests/SensorFault/`     | Fault threshold, latching, recovery events, counter reset              |
| 9  | `tests/SamplingScheduler/`        | Per-slot periods, phase alignment, minimum enforcement                 |
| 10 | `tests/EngineConfig/`    | `EngineConfig` defaults, custom values, aggregate init                 |
| 11 | `tests/TelemetryPackage/`        | Golden JSON serialization, envelope metadata, bounded buffer           |

**Source**: `tests/fakes/`, `tests/run_all.sh`, `.github/workflows/`, `CMakeLists.txt`

---

## Tier 2: Reliability, Diagnostics & Footprint (Complete)

### W6: Logging macros, stack high-water, sensor fault events

#### Logging Facility (`ModestLog.h`)

Zero-cost compile-time level filtering (`MODEST_LOG_LEVEL` 0-5). ESP32 `esp32-hal-log` backend binding. Host pluggable sink via `modest_iot::setLogSink()` for test capture. `MIOT_LOG_*` shorthand aliases for ergonomic call sites.

```cpp
#if MODEST_LOG_LEVEL >= 1
  #define MODEST_LOG_E(tag, fmt, ...) ModestLog::write(1, tag, fmt, ##__VA_ARGS__)
#else
  #define MODEST_LOG_E(tag, fmt, ...) ((void)0)
#endif
```

**Source**: `src/ModestLog.h`, `tests/ModestLog/test.cpp`

#### Diagnostics Snapshot

12-field `DiagnosticsSnapshot` struct: missedTicks, worstCaseWakeMicros, packetsEnqueued, packetsDropped, queueHighWaterMark, sendsSucceeded, sendsFailed, freeHeapBytes, minFreeHeapBytes, samplerStackFreeWords, networkStackFreeWords, uptimeSeconds.

`Device::diagnostics()` queries FreeRTOS `uxTaskGetStackHighWaterMark()`, `ESP.getFreeHeap()`, `ESP.getMinFreeHeap()`, and `millis()`.

Gateway statistics via `GatewayStatistics` struct and `MqttGatewayClient::getStatistics()`.

First-drop and first-failure warning logging with one-time transition guards.

**Source**: `src/Device.h`, `src/Device.cpp`, `src/GatewayClient.h`, `tests/DiagnosticsSnapshot/test.cpp`

#### Sensor Fault Events

Reserved event IDs 22 (`SENSOR_FAULT_EVENT_IDENTIFIER`) and 23 (`SENSOR_RECOVERED_EVENT_IDENTIFIER`).

Debounced threshold state machine in `Sensor` base class with `reportReadFailure()`, `reportReadSuccess()`, `setFaultThreshold()` helpers. Fault events emit exactly once when consecutive failures reach threshold; recovery events emit exactly once on next successful read. Default threshold is 0 (disabled) for backward compatibility.

Converted sensors: `DhtSensor`, `LoadCellAmplifier`, `UltrasonicSensor`, `DigitalLightIntensitySensor`.

`OutdoorSecurityLightController` example opts in with `setFaultThreshold(3)`.

**Source**: `src/Sensor.h`, `src/Sensor.cpp`, `tests/SensorFault/test.cpp`

---

### W7: Connectivity reconnect policy

**MqttGatewayClient**:
- Non-blocking reconnection with configurable backoff (`reconnectIntervalMilliseconds`).
- Continuous keep-alive servicing via `service()` method.
- Client state inquiry via `isConnected()`.
- Zero-cost `MODEST_LOG_*` diagnostic output for connection state changes.

**WiFiConnectivityDriver**:
- Re-arming retry logic with configurable reconnect intervals.
- Prevents permanent link lockouts after transient access point outages.

**Device Network Worker**:
- Polls telemetry queues with bounded timeout (`pdMS_TO_TICKS(100)`).
- Executes periodic background network maintenance via `maintainNetworkServices()`.

**Source**: `src/MqttGatewayClient.h`, `src/MqttGatewayClient.cpp`, `src/WiFiConnectivityDriver.cpp`, `src/Device.cpp`

---

### W9: EngineConfig and stopAsynchronousEngine

**EngineConfig Struct**:
- Configurable stack sizes, priorities, core pinning, and queue depth.
- Defaults match original hard-coded values (4096/8192 bytes, priority 3/1, core 1/1, queue 10).
- `initializeAsynchronousEngine(const EngineConfig&)` overload; legacy form delegates for backward compatibility.

**stopAsynchronousEngine**:
- Cooperative task termination via notification bits (bit 1 = stop).
- Timer cleanup, queue drain with packet deletion.
- Idempotent double-stop guard.
- `~Device()` calls `stopAsynchronousEngine()` as safety net.

**Source**: `src/Device.h`, `src/Device.cpp`, `tests/EngineConfig/test.cpp`

---

### W10: BLE driver (Partially Complete)

**BleConnectivityDriver**:
- Implemented Nordic UART Service (NUS) GATT profile with 128-bit UUIDs.
- TX characteristic with GATT notifications, RX characteristic with write callbacks.
- Automatic advertising lifecycle recovery on disconnect.
- Host unit tests covering connection state, transmission, and RX events.

**BluetoothConnectivityDriver**:
- Fully removed. The legacy driver was initially isolated behind preprocessor guards and marked `[[deprecated]]`, then deleted as Arduino-ESP32 Core 4.0.0 drops `BluetoothSerial` support and modern ESP32 variants (S3, C3, C6, H2) lack Bluetooth Classic hardware.

**Source**: `src/BleConnectivityDriver.h`, `src/BleConnectivityDriver.cpp`, `tests/BleConnectivityDriver/test.cpp`

---

## Tier 3: Core Scheduling & Telemetry (Complete)

### W8: Per-slot sampling intervals

**SamplingScheduler Class**:
- Pure C++ scheduler with per-slot periods, staggered initial phases, elapsed tick handling.
- Phase-aligned re-arming and minimum period enforcement with clamping reports.
- Wired into `Device::executeHardwareSamplingContext` replacing `activeSequencePointer` round-robin.
- 15 host unit tests covering all design scenarios.

**Sensor Interval Integration (Hybrid Approach)**:
- Two `protected` attributes in `Sensor` for runtime tuning (`samplingIntervalInMilliseconds`, `minimalSamplingIntervalInMilliseconds`).
- Two pure virtual methods (`getDefaultSamplingIntervalInMilliseconds`, `getMinimalSamplingIntervalInMilliseconds`).
- Four non-virtual getter/setter methods combining overrides with virtual defaults.
- Virtual method implementations in all 10 sensor subclasses.
- `Device::appendSensorToScheduler(Sensor*, int)` overload queries effective period polymorphically.
- `[[deprecated]]` removed from `EventHandler*` two-argument overload — valid fallback for non-Sensor handlers.

**Per-class interval constants** added to all polled sensor headers:

| Class                         | Default (ms) | Minimum (ms) |
|-------------------------------|--------------|--------------|
| `Button`                      | 100          | 10           |
| `PassiveInfraredSensor`       | 100          | 20           |
| `DigitalLightIntensitySensor` | 1000         | 180          |
| `DhtSensor`                   | per model    | per model    |
| `UltrasonicSensor`            | 100          | 60           |
| `PulseSensor`                 | 1000         | 10           |
| `GasSensor`                   | 1000         | 100          |
| `LoadCellAmplifier`           | 1000         | 100          |
| `GeolocationSensor`           | 1000         | 100          |
| `RfidSensor`                  | 100          | 10           |

**Example Migration**: All 10 examples migrated. `ClassicBlinker` uses explicit four-argument `EventHandler*` overload. 9 other examples automatically use `Sensor*` overload.

**Handler time budget** (worst-case blocking per handler):

| Handler                       | Worst-case blocking                 | Source                                  |
|-------------------------------|-------------------------------------|-----------------------------------------|
| `LoadCellAmplifier`           | up to 100 ms busy-wait              | `READ_TIMEOUT_MS = 100`                 |
| `UltrasonicSensor`            | up to 30 ms inside `pulseIn`        | `TIMEOUT_LIMIT_IN_MICROSECONDS = 30000` |
| `DhtSensor`                   | roughly 20 to 25 ms bit-banged read | DHT library                             |
| `DigitalLightIntensitySensor` | sub-millisecond to a few ms         | `Wire` reads                            |
| `GeolocationSensor`           | grows with serial backlog           | `updateInternalBuffer()`                |

**Source**: `src/SamplingScheduler.h`, `src/SamplingScheduler.cpp`, `src/Device.cpp`, `src/Sensor.h`, `tests/SamplingScheduler/test.cpp`

---

### W11: Telemetry ownership and queue policy (Partially Complete)

**Completed**:
- `unique_ptr` enqueue overload: on success, ownership transfers to queue; on failure, automatic deletion.
- Migrated `SmartBandMqtt` and `AirQualityMonitor` examples from raw `new`/`delete` to `std::make_unique`.
- Drop-oldest queue policy: when queue is full, oldest packet is dropped and new one inserted.
- `packetsDroppedOldest` counter in `DiagnosticsSnapshot`.
- Bounded 512-byte serialization buffer in `MqttGatewayClient` (`MAX_TELEMETRY_PAYLOAD_SIZE`), eliminating intermediate `String` allocation.
- Envelope metadata: `captureTimestamp` (from `millis()`) and `sequenceNumber` (monotonically increasing) in every `TelemetryPackage`. Sequence assigned before enqueue so drops are visible as gaps.
- Renamed `serialize()` convenience overloads to `toDocument()` and `toJsonString()` to prevent name hiding (T14).
- Golden host tests in `tests/TelemetryPackage/test.cpp` (10 test cases).

**Deferred**:
- Packet pool with placement-new (requires heap measurements).
- `TelemetryWriter` format abstraction (breaks all package subclasses).
- Alarm lane priority queue.

**Current Telemetry Pipeline**:

```
Sampler Task (Core 1)                    Network Task (Core 1)
┌──────────────────────┐                ┌─────────────────────────┐
│ Sensor reads data    │                │ xQueueReceive()         │
│                      │                │                         │
│ make_unique<Package> │──enqueue──►│ TelemetryPackage*       │
│ (heap allocation)    │  (unique_ptr)│                         │
│ if enqueue fails:    │                │ processQueuedTelemetry()│
│   unique_ptr deletes │                │ delete packet           │
└──────────────────────┘                │ serializeJson()         │
                                        │   → 512-byte buffer     │
                                        │   → mqtt.publish()      │
                                        └─────────────────────────┘
```

**Source**: `src/Device.h`, `src/Device.cpp`, `src/TelemetryPackage.h`, `src/MqttGatewayClient.h`, `src/MqttGatewayClient.cpp`, `tests/TelemetryPackage/test.cpp`

---

## Tier 4: Capabilities (Pending)

### W12: `Device::post` command marshalling

Give other tasks a safe way to act on the sampler's objects.

**Problem**: The threading model is single-threaded on the sampler. Cloud-to-device commands (MQTT subscribe) arrive on the network worker. Calling actuators from the network worker breaks the single-context guarantee.

**Recommendation**: `Device::post(CommandHandler*, Command)` — a command queue consumed by the sampler task. The sampler wakes through notification bit flags, drains the queue, and calls `handle` on its own context. Keeps every actuator single-threaded; no locks in actuators.

**Prerequisites**: Notification redesign (N1 from Tier 5), pure scheduler (done).

**Optional**: Argument-carrying `Command` (`Command(id, int32_t argument = 0)`) to remove the buffered-setter pattern.

**Source**: `src/Device.h`, `src/Command.h`

---

### W13: Interrupt triggers for Button and PIR

**Problem**: Edge-type sensors are polled, introducing latency (up to one full period) and wasting CPU on idle reads.

**Mechanism**: The ISR does minimal work: sets a bit and wakes the sampler. The sampler runs the sensor's normal `processEvent` (existing logic, including debounce) and forwards the event to the mediator.

```cpp
TriggerHandle addTriggered(EventHandler* handler, int eventIdentifier);
void IRAM_ATTR trigger(TriggerHandle handle);   // ISR-safe
```

**Notification redesign (N1)**: Replace `vTaskNotifyGiveFromISR` with `xTaskNotifyFromISR(handle, bit, eSetBits)` for all sources. Keep elapsed ticks in an ISR-incremented `std::atomic<uint32_t> pendingTicks` that the task exchanges to zero.

**ISR Rules**:
- Mark handlers `IRAM_ATTR`; keep them tiny; no heap, no `Serial`, no logging.
- Use `portYIELD_FROM_ISR` when a higher-priority task was woken.
- Rate-limit contact bounce by disabling the pin interrupt during debounce.
- Never call `digitalRead` from the ISR.

**Candidates**:
- `Button`: Excellent fit. GPIO edge interrupt plus debounce in the task.
- `PassiveInfraredSensor`: Excellent fit. Rising and falling edge.
- `LoadCellAmplifier` (HX711): Good. Data-ready line (DOUT) falls when conversion is ready.
- `UltrasonicSensor`: Good, harder. RMT/PCNT peripheral or edge interrupt pair.

**Source**: `src/Device.h`, `src/Button.h`, `src/PassiveInfraredSensor.h`

---

### W14: HX711 data-ready and ultrasonic echo capture

**Problem**: `LoadCellAmplifier` busy-waits up to 100 ms; `UltrasonicSensor` blocks up to 30 ms in `pulseIn`. Both consume the sensor tick budget on the sampler task.

**Resolution**: Convert to interrupt-driven operation so the sampler is only woken when data is ready, rather than polling and blocking.

**Source**: `src/LoadCellAmplifier.cpp`, `src/UltrasonicSensor.cpp`

---

### W15: Cloud-to-device commands (MQTT subscribe)

**Problem**: The framework only supports publish (telemetry outbound). No path from MQTT topic to `Command`.

**Resolution**: Add `subscribe(topic, callback)` to MQTT gateway and map messages to `Command`s delivered via `Device::post`. Requires W12 (command marshalling) first.

**Source**: `src/MqttGatewayClient.h`

---

## Tier 5: Optimization and Power (Pending)

### W16: Tickless scheduling and light sleep

**Problem**: The hardware timer wakes the system at the tick rate forever, preventing sleep.

**Resolution**: With the pure scheduler, the next due time is known. Program the single timer alarm for that moment instead of a fixed period:
- `SamplingScheduler::ticksUntilNextDue()` returns the smallest `ticksUntilDue`.
- After each dispatch, the sampler re-arms the one-shot alarm for that interval.

**Light Sleep** (`esp_light_sleep_start`): Timer and GPIO wake sources. RAM and CPU state preserved. Tasks resume. Wi-Fi association lost unless modem-sleep is configured.

**PowerPolicy for Device**:
```cpp
enum class PowerMode : uint8_t { AlwaysOn, LightSleepWhenIdle, DeepSleepCycle };
void setPowerMode(PowerMode mode, uint32_t maximumSleepMilliseconds);
```

Rules before sleeping:
1. Telemetry queue is empty and network worker is idle.
2. No slot is due within minimum useful sleep time.
3. Next wake sources are armed: timer for next due slot plus GPIO wake pins.

**Source**: `src/SamplingScheduler.h`, `src/Device.h`

---

### W17: Deep-sleep cycle and battery example

**Deep Sleep** (`esp_deep_sleep_start`): Timer and `ext0`/`ext1` GPIO wake. RAM lost except RTC memory (`RTC_DATA_ATTR`).

**Additions**:
- Application state survives only in RTC memory.
- Boot path must re-create devices and sensors quickly.
- Report wake reason (`esp_sleep_get_wakeup_cause()`) to mediator as event.
- Wi-Fi reconnection costs most of a battery budget; recommend batching telemetry.

**Source**: `src/Device.h`

---

### W18: Telemetry pool and writer interface

**Packet Pool** (A3): Placement-new into a pool with typed `acquire<T>()`. Keeps polymorphism and type safety with no heap.

```cpp
template <size_t SlotBytes, uint8_t SlotCount>
class TelemetryPool {
public:
    template <typename T, typename... Args>
    TelemetryHandle<T> acquire(Args&&... args);
};
```

**TelemetryWriter Interface**: Format-neutral output so a package writes to a "sink" rather than to a `JsonDocument`.

```cpp
class TelemetryWriter {
public:
    virtual void field(const char* name, int32_t v) = 0;
    virtual void field(const char* name, float v) = 0;
    virtual void field(const char* name, bool v) = 0;
    virtual void field(const char* name, const char* v) = 0;
};
```

**Breaking change**: Every `TelemetryPackage` subclass must change from `serialize(JsonDocument&)` to `write(TelemetryWriter&)`. Keep `serialize(JsonDocument&)` as a deprecated adapter path for one release.

**Source**: `src/TelemetryPackage.h`

---

### W19: Native servo and LCD drivers

**ESP32Servo Replacement**: About 60-100 lines using `ledcAttach` and `ledcWrite` at 50 Hz with a 500-2500 microsecond pulse range. Risk: LEDC API differs between core 2.x and 3.x.

**LiquidCrystal_I2C Replacement**: About 100 lines for an HD44780 through a PCF8574 backpack (4-bit mode, init sequence, timing). Risk: backpack pin maps and 3.3V vs 5V behavior vary.

**Recommendation**: Wrap first behind small internal interfaces, decide with data.

**Source**: `src/ServoMotor.h`, `src/CharacterLcdDisplay.h`

---

## Comparison with traditional development

Compares three points: a traditional Arduino `loop()` app, Modest IoT today, and Modest IoT once Tiers 0 to 4 are done.

### Developer experience

| Aspect          | Traditional `loop()`                         | Modest IoT today                                                  | Modest IoT after Tier 4                                  |
|-----------------|----------------------------------------------|-------------------------------------------------------------------|----------------------------------------------------------|
| First build     | Works with the board package                 | All examples compile in CI                                        | All examples compile in CI; only used libraries declared |
| Structure       | One growing `loop()` with `millis()` flags   | Sensors emit events, a `Device` mediates, actuators take commands | Same, plus documented threading rules and an ID registry |
| Adding a sensor | New globals, new timing variable, new branch | Subclass or reuse, register in the scheduler                      | One line with a per-sensor default period                |
| Sampling rates  | Hand-written per-sensor timers               | Per-slot periods, minimums enforced, staggering handled           | Same                                                     |
| Timing mistakes | Easy: a blocking call stalls everything      | Handler budgets documented; blocking HX711/ultrasonic removed     | Same, plus interrupt-driven sensors                      |
| Debugging       | `Serial.println` scattered                   | `MODEST_LOG_*`, stack high-water marks, sensor fault events       | Same                                                     |
| Networking      | Blocking connect/publish in `loop()`         | Separate network task, reconnect policy, keep-alive               | Same, plus cloud-to-device commands                      |
| Cloud-to-device | Hand-rolled callback                         | Not supported                                                     | Mediator-routed commands through `Device::post`          |
| Testing         | Rarely possible off-device                   | 11 host test suites with CI                                       | Same                                                     |

### Performance

| Aspect                     | Traditional `loop()`              | Modest IoT today                              | Modest IoT after Tier 4                               |
|----------------------------|-----------------------------------|-----------------------------------------------|-------------------------------------------------------|
| Timing model               | Cooperative polling; jitter grows | Timer ISR plus sampler task; per-slot periods | Same engine; interrupt-driven sensors                 |
| Sample period accuracy     | Depends on `loop()` duration      | Per-slot periods independent of slot count    | Same                                                  |
| Worst-case stall           | Any blocking call                 | HX711 up to 100 ms, ultrasonic up to 30 ms    | Interrupt-driven HX711/ultrasonic; documented budgets |
| Edge latency (button, PIR) | Up to one `loop()` pass           | Up to one polling period                      | Interrupt mode: task wake-up latency                  |
| Network isolation          | A slow publish blocks sensing     | Isolated in the network task                  | Isolated, with bounded queue policy                   |
| Heap behaviour             | Usually static                    | `unique_ptr` ownership, drop-oldest policy    | Same; pool only if measurement justifies it           |
| CPU when idle              | Spins in `loop()`                 | Sleeps between ticks                          | Fewer idle wake-ups; sleep modes in Tier 5            |
| Energy                     | High unless hand-tuned            | Same as traditional                           | Modestly better; large savings with Tier 5            |

### Resource consumption

| Resource                 | Traditional `loop()` | Modest IoT today                                                                | Modest IoT after Tier 4                            |
|--------------------------|----------------------|---------------------------------------------------------------------------------|----------------------------------------------------|
| Task stacks              | `loop()` task only   | 4096 B sampler + 8192 B network, configurable                                   | Same defaults, configurable through `EngineConfig` |
| Queue and packets        | None                 | Queue of pointers + one heap packet per reading                                 | Same; bounded by queue policy                      |
| Flash                    | Smallest             | Framework, Wi-Fi, BLE driver                                                    | Smaller than today if Bluetooth/MQTT optional      |
| Static RAM               | Small                | Slot table, per-object state                                                    | Slot table gains a few words per slot              |
| Heap fragmentation       | Low                  | Lower with explicit ownership                                                   | Same; pool in Tier 5 if measured                   |
| Third-party dependencies | As needed            | Only those used (DHT, LiquidCrystal_I2C, ESP32Servo, PubSubClient, ArduinoJson) | Same                                               |
| Logging overhead         | User-controlled      | Zero when compiled out; UART cost when on                                       | Same                                               |

---

## How to validate the comparison

Run the same three reference applications on the same ESP32 board, once per variant (plain `loop()`, current Modest IoT, post-Tier-4 Modest IoT):

1. **Multi-sensor monitor:** DHT, BH1750 and a button, with a display and Wi-Fi/MQTT telemetry.
2. **Blocking-sensor stress:** the monitor plus an HX711 and an ultrasonic sensor.
3. **Network fault:** the monitor with the broker unreachable for 60 seconds, then restored.

| Metric                                  | Method                                                                     |
|-----------------------------------------|----------------------------------------------------------------------------|
| Sample period jitter                    | GPIO toggle per sample on a logic analyzer, or `esp_timer_get_time` deltas |
| Button and PIR edge-to-action latency   | Logic analyzer: input edge to actuator output edge                         |
| Sensing continuity during network fault | Count missed samples while the broker is down                              |
| Reconnect time                          | Time from broker restore to first successful publish                       |
| Stack use                               | `uxTaskGetStackHighWaterMark` for each task                                |
| Heap                                    | `ESP.getFreeHeap()` and `getMinFreeHeap()` after 24 hours                  |
| Flash and static RAM                    | Size report from the build for each variant                                |
| Current draw                            | Inline current meter (Wokwi cannot measure it)                             |
| Lines of application code               | Count in the reference sketches                                            |

Publish the table in the README with the board, core version and commit hashes. Capture measurements after each tier, not only at the end.
