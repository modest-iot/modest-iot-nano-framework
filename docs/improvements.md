# Modest IoT Improvements

Master index, implementation status, and detailed plans for all framework improvements.

All 8 improvement briefs and the roadmap are consolidated in this single document to eliminate cross-document maintenance overhead. Architecture decisions are recorded separately in [adrs.md](adrs.md).

---

## Status Overview

| Tier                                    | Items                                                  | Goal                                                                                                | Exit criterion                                                                                                       | Status       |
|-----------------------------------------|--------------------------------------------------------|-----------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------|--------------|
| 0: Make it correct and shippable        | W1, W2, W3, W4                                         | Everything compiles, repo is clean, rules are written down                                          | All examples compile cleanly; clean repo layout                                                                      | **Complete** |
| 1: Safety net and visibility            | W5                                                     | Regressions caught automatically via host tests & CI                                                | CI green on PRs/pushes; all 27 host test suites pass; examples compile in CI; lint passes                         | **Complete** |
| 2: Reliability, diagnostics & footprint | W6 (logging/high-water/sensor conversion), W7, W9, W10 | Reconnection robust, tasks tunable, BLE modern, zero-cost logs, sensor fault events with conversion | Soak test reconnects; logs available behind macro; BLE tests pass; diagnostics snapshot works; sensors report faults | **Complete** |
| 3: Core performance and DX              | W8, then W11                                           | Per-slot intervals, bounded memory use for telemetry                                                | Scheduler host tests pass; all examples use per-sensor intervals; telemetry ownership and queue policy complete      | **Complete** |
| 4: Capabilities                         | W12, W13, W14, W15, rest of W6                         | Edge-driven sensing, remote commands, fault events                                                  | Bidirectional MQTT verified; interrupt-driven sensors tested                                                         | **Complete** (W12 ✓, W13 ✓, W14 ✓, W15 ✓) |
| 5: Optimization and power               | W16, W17, W18, W19                                     | Battery operation and further tuning                                                                | Decide per item from measured data                                                                                   | **Complete** (W16 ✓, W17 ✓, W19 ✓, W18 deferred to v1.1.0) |

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
| W12 | `Device::enqueueCommand` command marshaling                                                      | Tier 4      | 3 | 0 | 4 | M    | 6     | **Complete** (`enqueueCommand`, `processPendingCommands`, `CommandEntry`, `CommandAvailable` bit, host tests) |
| W13 | Interrupt triggers for Button, PIR, and WaterFlowSensor                                 | Tier 4      | 3 | 4 | 3 | M    | 6     | **Complete** (Button/PIR interrupt-driven, WaterFlowSensor hybrid, examples migrated, host tests) |
| W14 | HX711 data-ready and ultrasonic echo capture (remove blocking)                          | Tier 4      | 2 | 4 | 2 | M    | 5     | **Complete** (LoadCellAmplifier hybrid scheduling, UltrasonicSensor interrupt echo, SmartScaleDevice migrated) |
| W15 | Cloud-to-device commands (MQTT subscribe)                                               | Tier 4      | 2 | 0 | 4 | M    | 5     | **Complete** (subscribe on connect, JSON parsing, Device::enqueueCommand, 8 host tests)           |
| W16 | Tickless scheduling and light sleep                                                     | Tier 5      | 2 | 4 | 2 | L    | 3     | **Complete** (ticksUntilNextDue(), PowerMode enum, rearmTimerForNextDue(), enterLightSleep(), ESP32 light sleep) |
| W17 | Deep-sleep cycle and battery example                                                    | Tier 5      | 1 | 4 | 2 | L    | 3     | **Complete** (PowerMode::DeepSleep, enterDeepSleep(), reportWakeReason(), RTC_DATA_ATTR, WeatherStation example, 10 host tests) |
| W18 | Telemetry pool and writer interface (Phase C)                                           | Tier 5      | 2 | 3 | 1 | L    | 2     | Pending                                                                                                                                     |
| W19 | Native servo and LCD drivers                                                            | Tier 5      | 2 | 0 | 1 | M    | 2     | **Complete** (ServoMotor native LEDC, Hd44780Driver + CharacterLcdDisplay native I2C, library.properties updated, 20 host tests) |

## Dependencies between items

- Interrupt triggers and `Device::post` share one notification redesign (bit flags plus an atomic tick counter) and require the pure scheduler.
- Tickless sleep requires `ticksUntilNextDue()` from the scheduler.
- Cloud-to-device commands (connectivity) require `Device::post` (threading).
- Stack-size defaults should come from diagnostics measurements.
- Telemetry writer and pool work should follow the host test harness.

## Deliberately out of scope

- Replacing ArduinoJson (no viable embedded alternative).
- TLS/SSL implementation (use ESP32's built-in NetworkClientSecure).
- Targeted (per-instance) commands until a use case appears.
- Switchable sensor power rails and ADC/DMA sampling.
- Non-ESP32 targets.

## Claims verified and remaining verification gaps

- **Verified**: Local `arduino-cli compile` cleanly builds all 16 examples across ESP32 targets with `--library .` configured.
- **Verified**: `library.properties` `depends=` syntax formatted for Arduino Library Manager compliance.
- **Verified**: Bluetooth Classic fails on modern ESP32 variants (S2, S3, C3, C6, H2); resolved via target guards and `BleConnectivityDriver`.
- **Verified**: Host test harness executes 27 unit test suites (`tests/digital_light`, `tests/rgb_led`, `tests/relay_actuator`, `tests/rfid_sensor`, `tests/ble_connectivity`, `tests/log`, `tests/mqtt_gateway`, `tests/engine_config`, `tests/diagnostics`, `tests/sensor_fault`, `tests/scheduler`, `tests/telemetry`, `tests/triggering_scheduler`, `tests/command_marshaling`, `tests/water_flow_sensor`, `tests/soil_moisture_sensor`, `tests/deep_sleep`, `tests/servo_motor`, `tests/character_lcd_display`, `tests/door_window_sensor`, `tests/water_leak_sensor`, `tests/pth_sensor`, `tests/one_wire_temperature_sensor`, `tests/oled_display`, `tests/environmental_gas_sensor`, `tests/dht_sensor`, `tests/ble_gateway_client`) under Address and Undefined sanitizers.
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
| `LiquidCrystal_I2C`       | ~~`CharacterLcdDisplay`~~     | **Removed**              | Replaced with native HD44780 I2C protocol via `Hd44780Driver`                       |
| `DHT sensor library`      | ~~`DhtSensor`~~         | **Removed**              | Replaced with native single-wire protocol in `DhtSensor`                  |
| `ESP32Servo`              | ~~`ServoMotor`~~              | **Removed**              | Replaced with native LEDC PWM via `ledcAttach()`/`ledcWrite()`                      |
| `MQTT (>=2.5.3)`          | `MqttGatewayClient`           | yes                      | Replaced deprecated PubSubClient with MQTT Things (256dpi/arduino-mqtt)             |
| `ArduinoJson`             | `TelemetryPackage` (needed)   | yes, pinned to `>=7.0.0` | Code uses the v7 `JsonDocument` type                                                 |
| `HX711`                   | ~~`LoadCellController` only~~ | **Removed**              | `LoadCellController` deleted; `LoadCellAmplifier` implements HX711 protocol natively |
| `EspSoftwareSerial`       | nothing                       | **Removed**              | Not referenced anywhere                                                              |
| `Adafruit_Unified_Sensor` | nothing directly              | **Removed**              | Not used anywhere; removed from library.properties                                   |

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
| Events   | 60, 61       | `RfidSensor` card detected, removed                  |
| Events   | 70, 71       | `WaterLeakSensor` leak detected, cleared             |
| Events   | 72, 73       | `DoorWindowSensor` door opened, closed               |
| Commands | 100 to 102   | `Led`                                                |
| Commands | 150 to 152   | `ServoMotor`                                         |
| Commands | 200 to 202   | `CharacterLcdDisplay`                                |
| Commands | 250 to 252   | `Buzzer`                                             |
| Commands | 300 to 302   | `RelayActuator`                                      |
| Commands | 304 to 306   | `RgbLed`                                             |
| Commands | 350 to 352   | `OledDisplay`                                        |

Reserved blocks: sensor-health 22-23, connectivity link events 900-909, power events 910-919. User identifiers from 1000.

**Source**: `README.md`, `src/Device.h`, `src/Sensor.h`, `src/Actuator.h`, `src/GatewayClient.h`

---

## Tier 1: Test Consolidation & CI (Complete)

### W5: Host test harness plus CI

**Shared Mock Harness**:
- Created centralized `tests/fakes/` with `Arduino.h`, `Wire.h`, `SPI.h`, `BLEDevice.h`, `BLEServer.h`, `BLEUtils.h`, `MQTT.h`, `WiFi.h`, `ArduinoJson.h`, `Client.h`.
- Eliminated duplicate per-suite fake directories.
- Resolved all Clang-Tidy warnings: deprecated C headers to C++ headers, unused includes, const/static qualifications.

**Test Runner & CI**:
- Implemented `tests/run_all.sh` with strict warnings (`-Wall -Wextra -Werror`) and Address/Undefined sanitizers.
- Added GitHub Actions workflows:
  - `host-tests.yml`: runs all 27 test suites on Ubuntu and macOS.
  - `compile-examples.yml`: compiles all 16 examples across ESP32 and ESP32-S3 boards.
  - `lint.yml`: validates `library.properties` with Arduino Library Manager compliance.
- Registered all test targets in `CMakeLists.txt` with CTest integration.

**Test Suites (27 total)**:

| #  | Suite                                | What It Tests                                                          |
|----|--------------------------------------|------------------------------------------------------------------------|
| 1  | `tests/DigitalLightIntensitySensor/` | I²C sensor polling, Wire mock, event emission                          |
| 2  | `tests/RgbLed/`                      | PWM output, color mixing, analogWrite mock                             |
| 3  | `tests/RelayActuator/`               | Relay on/off/toggle commands, state tracking, parent propagation       |
| 4  | `tests/RfidSensor/`                  | CRC calculation, SPI mock, card communication                          |
| 5  | `tests/BleConnectivityDriver/`       | BLE NUS GATT service, notifications, RX callbacks                      |
| 6  | `tests/ModestLog/`                   | `MODEST_LOG_*` level filtering, sink redirection, `MIOT_LOG_*` aliases |
| 7  | `tests/MqttGatewayClient/`           | MQTT keep-alive, reconnect backoff, MQTT Things client state           |
| 8  | `tests/DiagnosticsSnapshot/`         | `DiagnosticsSnapshot`, shared counters, enqueue edge cases             |
| 9  | `tests/SensorFault/`                 | Fault threshold, latching, recovery events, counter reset              |
| 10 | `tests/SamplingScheduler/`           | Per-slot periods, phase alignment, minimum enforcement                 |
| 11 | `tests/EngineConfig/`                | `EngineConfig` defaults, custom values, aggregate init                 |
| 12 | `tests/TelemetryPackage/`            | Golden JSON serialization, envelope metadata, bounded buffer           |
| 13 | `tests/TriggeringScheduler/`         | Interrupt-driven sensor registration, trigger table, ISR handler       |
| 14 | `tests/CommandMarshaling/`           | Command queue, cross-task marshaling, enqueue edge cases               |
| 15 | `tests/WaterFlowSensor/`             | Hybrid scheduling, pulse counting, flow rate change detection          |
| 16 | `tests/SoilMoistureSensor/`          | AnalogRead moisture, epsilon-based change detection, event emission    |
| 17 | `tests/DeepSleep/`                   | PowerMode enum, wake causes, RTC_DATA_ATTR, deep sleep API mocks      |
| 18 | `tests/ServoMotor/`                  | Native LEDC PWM, angle commands, state tracking, parent propagation   |
| 19 | `tests/CharacterLcdDisplay/`         | HD44780 4-bit I2C protocol, cursor positioning, backlight control     |
| 20 | `tests/PthSensor/`                   | BME280 I²C sensor, compensation formulas, altitude calculation        |
| 21 | `tests/WaterLeakSensor/`             | Digital input, leak detection, propagation suppression                |
| 22 | `tests/DoorWindowSensor/`            | Digital input, open/close detection, propagation suppression          |
| 23 | `tests/OneWireTemperatureSensor/`    | 1-Wire protocol, DS18B20 temperature, change detection                |
| 24 | `tests/OledDisplay/`                 | SSD1306 I²C protocol, frame buffer, text rendering, drawing           |
| 25 | `tests/EnvironmentalGasSensor/`      | BME680 I²C sensor, gas resistance, compensation formulas              |
| 26 | `tests/DhtSensor/`                   | DHT single-wire protocol, temperature/humidity, CRC validation        |
| 27 | `tests/BleGatewayClient/`            | BLE NUS telemetry, JSON lines framing, statistics tracking            |

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
- Virtual method implementations in all 18 sensor subclasses.
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
| `PthSensor`                   | 2000         | 250          |
| `WaterLeakSensor`             | 100          | 20           |
| `DoorWindowSensor`            | 100          | 20           |
| `OneWireTemperatureSensor`    | 1000         | 100          |
| `EnvironmentalGasSensor`      | 2000         | 500          |
| `DhtSensor`                   | per model    | per model    |

**Example Migration**: All 16 examples migrated. `ClassicBlinker` uses explicit four-argument `EventHandler*` overload. 15 other examples automatically use `Sensor*` overload.

**Handler time budget** (worst-case blocking per handler):

| Handler                       | Worst-case blocking                 | Source                                  |
|-------------------------------|-------------------------------------|-----------------------------------------|
| `LoadCellAmplifier`           | ~1.2 ms (24-bit read, no busy-wait) | `readRawValue()` non-blocking           |
| `UltrasonicSensor`            | microseconds (ISR-based timing)     | `onEchoInterrupt()` with esp_timer      |
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
│ make_unique<Package> │──enqueue──────►│ TelemetryPackage*       │
│ (heap allocation)    │  (unique_ptr)  │                         │
│ if enqueue fails:    │                │ processQueuedTelemetry()│
│   unique_ptr deletes │                │ delete packet           │
└──────────────────────┘                │ serializeJson()         │
                                        │   → 512-byte buffer     │
                                        │   → mqtt.publish()      │
                                        └─────────────────────────┘
```

**Source**: `src/Device.h`, `src/Device.cpp`, `src/TelemetryPackage.h`, `src/MqttGatewayClient.h`, `src/MqttGatewayClient.cpp`, `tests/TelemetryPackage/test.cpp`

---

## Tier 4: Capabilities (Complete)

### W12: `Device::enqueueCommand` command marshaling

Give other tasks a safe way to act on the sampler's objects.

**Status**: **Complete**

**Implementation**:
- `Device::CommandEntry` struct carries `CommandHandler*` and `Command` value.
- `Device::enqueueCommand(CommandHandler*, Command)` places entry on the command queue and wakes the sampler via `CommandAvailable` bit flag.
- `Device::processPendingCommands()` drains the queue calling `targetHandler->handle(command)` on the sampler task context.
- Command queue created in `initializeAsynchronousEngine()` with same length as telemetry queue.
- Command queue drained in `stopAsynchronousEngine()` — commands are value types, no cleanup needed.
- 16 host unit tests in `tests/CommandMarshaling/test.cpp`.

**Source**: `src/Device.h`, `src/Device.cpp`, `tests/CommandMarshaling/test.cpp`

**Problem**: The threading model is single-threaded on the sampler. Cloud-to-device commands (MQTT subscribe) arrive on the network worker. Calling actuators from the network worker breaks the single-context guarantee.

**Recommendation**: `Device::post(CommandHandler*, Command)` — a command queue consumed by the sampler task. The sampler wakes through notification bit flags, drains the queue, and calls `handle` on its own context. Keeps every actuator single-threaded; no locks in actuators.

**Prerequisites**: Notification redesign (N1 from Tier 5), pure scheduler (done).

**Optional**: Argument-carrying `Command` (`Command(id, int32_t argument = 0)`) to remove the buffered-setter pattern.

**Source**: `src/Device.h`, `src/Command.h`

---

### W13: Interrupt triggers for Button, PIR, and pulse-based sensors

**Status**: Complete — Button and PIR converted to interrupt-driven, WaterFlowSensor added as hybrid sensor, all examples migrated, host tests pass.

**Problem**: Edge-type sensors are polled, introducing latency (up to one full period) and wasting CPU on idle reads. Pulse-based sensors (e.g., YF-S201 water flow) cannot be accurately measured by polling alone — every pulse must be captured.

**Implementation**:

- **Notification redesign (N1)**: Replaced `vTaskNotifyGiveFromISR` with `xTaskNotifyFromISR(handle, bit, eSetBits)` for all sources. Elapsed ticks tracked in `std::atomic<uint32_t> pendingTicks` that the sampler exchanges to zero. Bit-flag `enum class NotificationBitFlag` with named constants: `TimerTickArrived` (0x1), `TerminationSignal` (0x2), `SensorTrigger` (0x4), `CommandAvailable` (0x8).
- **Triggering scheduler API**:
  - `appendSensorToTriggeringScheduler(Sensor*, int, int)` — registers interrupt-driven sensor, returns handle.
  - `removeSensorFromTriggeringScheduler(uint32_t)` — removes registration, detaches GPIO interrupt.
  - `appendToBothSchedulers(Sensor*, int, int)` — convenience for hybrid sensors (e.g., YF-S201).
- **Trigger table**: `MAX_TRIGGERED_SENSORS = 4` slots, each with `sensor`, `eventIdentifier`, `wasTriggered`, and `slotIndex`.
- **ISR handler**: Static `onSensorTrigger(uint32_t)` marks the sensor as triggered and wakes the sampler via `SensorTrigger` bit flag.
- **Sampler task**: Calls `processTriggeredSensors()` when `SensorTrigger` bit is set, iterating the trigger table and calling `processEvent()` + `on()` for each triggered sensor.
- **Button conversion**: Updated `Button` documentation to indicate interrupt-driven operation via `appendSensorToTriggeringScheduler()` with `FALLING` edge. Debouncing logic in `processEvent()` runs in the sampler task context after the GPIO interrupt fires.
- **PIR conversion**: Updated `PassiveInfraredSensor` documentation to indicate interrupt-driven operation via `appendSensorToTriggeringScheduler()` with `CHANGE` edge. Both motion detected (RISING) and motion stopped (FALLING) are captured.
- **WaterFlowSensor (new component)**: Hybrid sensor using `appendToBothSchedulers()`. Interrupt captures every Hall effect pulse (CHANGE edge), periodic sampling computes liters/minute from pulse delta. YF-S201 constant: 7 pulses/ml.
- **Examples migrated**: `ToggableLedDevice`, `SmartBandMqtt`, `AirConditioningDevice` (Button → interrupt), `PerimeterMonitor`, `OutdoorSecurityLightController` (PIR → interrupt).
- **Host tests**: 16 test cases in `tests/TriggeringScheduler/test.cpp` + 6 new tests in `tests/WaterFlowSensor/test.cpp`. All 22 test suites pass.

**API Design** (following `appendSensorToScheduler` naming pattern):

```cpp
// In Device.h:
uint32_t appendSensorToTriggeringScheduler(Sensor* target, int eventId, int interruptMode = CHANGE);
void removeSensorFromTriggeringScheduler(uint32_t triggerHandle);
bool appendToBothSchedulers(Sensor* target, int eventId, int interruptMode = CHANGE);
```

**Hybrid Sensor Pattern** (e.g., YF-S201):

```cpp
appendToBothSchedulers(&flowSensor,
                       Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER,
                       RISING);
```

**Source**: `src/Device.h`, `src/Device.cpp`, `src/Sensor.h`, `tests/TriggeringScheduler/test.cpp`

---

### W14: HX711 data-ready and ultrasonic echo capture — **Complete**

**Problem**: `LoadCellAmplifier` busy-waits up to 100 ms; `UltrasonicSensor` blocks up to 30 ms in `pulseIn`. Both consume the sensor tick budget on the sampler task.

**Resolution**: Converted both sensors to interrupt-driven operation to eliminate blocking calls.

**Implementation**:

- **LoadCellAmplifier hybrid scheduling**: Added `dataReady` volatile flag set by GPIO interrupt on the HX711 data pin (CHANGE edge, FALLING triggers the read). `processEvent()` checks `dataReady` first; if not ready, suppresses propagation. If ready, performs the 24-bit read (~1.2 ms) without the 100 ms busy-wait. `readRawValue()` is the non-blocking read; `readRawValueWithWait()` retains the busy-wait for calibration/tare operations. Register via `appendToBothSchedulers()` for hybrid operation.
- **UltrasonicSensor interrupt echo**: Replaced `pulseIn()` with GPIO interrupt on the echo pin (CHANGE edge). `onEchoInterrupt()` ISR uses `esp_timer_get_time()` for nanosecond-precision timing: rising edge starts the timer, falling edge stops it and records the duration. `processEvent()` triggers the pulse and polls for `echoComplete` flag set by the ISR. Destructor detaches the interrupt.
- **Examples migrated**: `SmartScaleDevice` now uses `appendToBothSchedulers()` for all 4 LoadCellAmplifiers with FALLING edge.

**Performance impact**:
- LoadCellAmplifier: Worst-case sampler blocking reduced from **100 ms to ~1.2 ms** (99% reduction).
- UltrasonicSensor: Worst-case sampler blocking reduced from **30 ms to microseconds** (ISR-based timing).

**Source**: `src/LoadCellAmplifier.h`, `src/LoadCellAmplifier.cpp`, `src/UltrasonicSensor.h`, `src/UltrasonicSensor.cpp`, `examples/SmartScaleDevice/SmartScaleDevice.ino`

---

### W15: Cloud-to-device commands (MQTT subscribe) — **Complete**

**Problem**: The framework only supports publish (telemetry outbound). No path from MQTT topic to `Command`.

**Resolution**: Added subscribe topic to `MqttGatewayClient` constructor. On successful broker connection, the client subscribes to the inbound command topic. Incoming MQTT messages are parsed as JSON to extract `commandId`, and the resulting `Command` is marshaled to the sampler task via `Device::enqueueCommand()`. Since `Device` extends `CommandHandler`, the Device itself is the command target, ensuring cross-task safety. A legacy constructor preserves backward compatibility for publish-only mode.

**Implementation**:
- New constructor accepts `subscribeTopic` and `Device*` parameters (Device extends CommandHandler).
- `subscribeToTopic()` called after successful `mqttClient.connect()`.
- Static `mqttMessageCallback()` parses JSON and marshals commands via `Device::enqueueCommand()`.
- Commands are delivered to the sampler task context, not the network task context.
- Fixed-size `subscribePayloadBuffer` (256 bytes) prevents heap allocation.
- Payload size guard rejects oversized messages.
- Invalid JSON and missing `commandId` fields are logged and ignored.
- `Device::enqueueCommand()` is public and virtual for testability.

**Source**: `src/MqttGatewayClient.h`, `src/MqttGatewayClient.cpp`, `src/Device.h`

**Tests**: 8 host tests covering subscribe topic storage, legacy constructor, subscribe on connect, command JSON parsing with enqueueCommand verification, multiple commands, missing commandId, invalid JSON, and publish-only mode.

---

## Component Expansion Tier 1 (Complete)

Added two zero-dependency components to enable the v1.0.0 library release with broader IoT coverage.

| Component | Type | Dependencies | Use Case |
|-----------|------|-------------|----------|
| RelayActuator | Actuator | `digitalWrite()` | AC/DC load control (smart home, greenhouse pumps, industrial relays) |
| SoilMoistureSensor | Sensor | `analogRead()` | Soil moisture monitoring (smart greenhouse, agriculture) |

### RelayActuator

**Commands**: `RELAY_ON_COMMAND` (300), `RELAY_OFF_COMMAND` (301), `TOGGLE_RELAY_COMMAND` (302)

**API**: Extends `Actuator` with `isActive()` getter. Follows the `Led` pattern exactly.

**Tests**: 8 host tests covering constructor, on/off/toggle commands, state tracking, unknown command handling, and parent propagation.

**Source**: `src/RelayActuator.h`, `src/RelayActuator.cpp`, `tests/RelayActuator/test.cpp`

### SoilMoistureSensor

**Readings**: Raw ADC value (0–1023) via `analogRead()`. Callers calibrate to percentage based on sensor and soil type.

**Change Detection**: Epsilon-based (`MOISTURE_CHANGE_EPSILON = 5` ADC units) suppresses spurious events from ADC noise.

**Sampling**: Default 1000ms, minimum 100ms.

**Tests**: 7 host tests covering constructor, moisture reading, change detection, propagation suppression, and event emission.

**Source**: `src/SoilMoistureSensor.h`, `src/SoilMoistureSensor.cpp`, `tests/SoilMoistureSensor/test.cpp`

### SmartGreenhouseDevice Example

Bundles `SoilMoistureSensor` + `DhtSensor` + `RelayActuator` (pump) + `ServoMotor` (vent). Demonstrates event-driven irrigation control (soil dry → pump on) and ventilation control (temperature high → vent open).

**Source**: `examples/SmartGreenhouseDevice/SmartGreenhouseDevice.ino`

### WaterFlowMonitor Example

Demonstrates `WaterFlowSensor` with `appendToBothSchedulers()` using CHANGE edge for hybrid interrupt-driven pulse counting plus periodic flow rate computation.

**Source**: `examples/WaterFlowMonitor/WaterFlowMonitor.ino`

---

## Component Expansion Tier 2 (Complete)

Added three components to broaden IoT coverage for weather stations, home security, and industrial monitoring.

| Component | Type | Dependencies | Use Case |
|-----------|------|-------------|----------|
| PthSensor | Sensor | I²C (`Wire`) | Temperature, humidity, pressure (weather stations, indoor air quality) |
| WaterLeakSensor | Sensor | `digitalRead()` | Water leak detection (home security, industrial monitoring) |
| DoorWindowSensor | Sensor | `digitalRead()` | Door/window open/close detection (home security, perimeter monitoring) |

### PthSensor

**Readings**: Temperature (°C), humidity (%), pressure (hPa) via I²C with Bosch BME280 compensation formulas.

**Change Detection**: Epsilon-based for all three values (0.1°C, 0.1%, 0.1 hPa).

**Sampling**: Default 2000ms, minimum 250ms.

**Tests**: 12 host tests covering initialization, reading, change detection, altitude calculation, and event emission.

**Source**: `src/PthSensor.h`, `src/PthSensor.cpp`, `tests/PthSensor/test.cpp`

### WaterLeakSensor

**Events**: `WATER_LEAK_DETECTED_EVENT` (70), `WATER_LEAK_CLEARED_EVENT` (71).

**API**: Extends `Sensor` with `isWaterLeakDetected()` getter. Uses `INPUT_PULLUP` — water bridges contacts to GND, reading LOW.

**Tests**: 6 host tests covering constructor, leak detection, leak cleared, and propagation suppression.

**Source**: `src/WaterLeakSensor.h`, `src/WaterLeakSensor.cpp`, `tests/WaterLeakSensor/test.cpp`

### DoorWindowSensor

**Events**: `DOOR_OPENED_EVENT` (72), `DOOR_CLOSED_EVENT` (73).

**API**: Extends `Sensor` with `isDoorOpen()` getter. Uses `INPUT` — reed switch connects GPIO to VCC when open, reading HIGH.

**Tests**: 6 host tests covering constructor, door opened, door closed, and propagation suppression.

**Source**: `src/DoorWindowSensor.h`, `src/DoorWindowSensor.cpp`, `tests/DoorWindowSensor/test.cpp`

### HomeSecurityDevice Example

Bundles `WaterLeakSensor` + `DoorWindowSensor` + `PassiveInfraredSensor` + `Buzzer` for home security monitoring. All sensors use interrupt-driven operation via `appendSensorToTriggeringScheduler()` for zero-latency event detection.

**Source**: `examples/HomeSecurityDevice/HomeSecurityDevice.ino`

### WeatherStation Example Update

Updated `WeatherStation` example to include `PthSensor` alongside `DhtSensor` and `DigitalLightIntensitySensor` for comprehensive environmental monitoring (temperature, humidity, pressure, and light).

**Source**: `examples/WeatherStation/WeatherStation.ino`

---

## Component Expansion Tier 3 (Complete)

Adding three popular IoT components to maximize v1.0.0 adoption: DS18B20 1-Wire temperature, SSD1306 OLED display, and BME680 environmental gas sensor.

| Component | Type | Dependencies | Use Case | Status |
|-----------|------|-------------|----------|--------|
| OneWireTemperatureSensor | Sensor | GPIO (1-Wire) | High-accuracy temperature (weather stations, aquariums) | **Complete** |
| OledDisplay | Actuator | I²C (`Wire`) | OLED display (portable devices, dashboards) | **Complete** |
| Ssd1306Driver | Driver | I²C (`Wire`) | SSD1306 protocol driver for OledDisplay | **Complete** |
| EnvironmentalGasSensor | Sensor | I²C (`Wire`) | Air quality monitoring (VOC, gas resistance) | **Complete** |

### OneWireTemperatureSensor

**Readings**: Temperature (°C) via native 1-Wire bit-banging protocol (DS18B20).

**Change Detection**: Epsilon-based (`TEMPERATURE_CHANGE_EPSILON = 0.1f` °C).

**Sampling**: Default 1000ms, minimum 100ms.

**Tests**: 8 host tests covering constructor, pin configuration, sampling intervals, cached value access, and change detection.

**Source**: `src/OneWireTemperatureSensor.h`, `src/OneWireTemperatureSensor.cpp`, `tests/OneWireTemperatureSensor/test.cpp`

### OledDisplay + Ssd1306Driver

**Ssd1306Driver**: Native SSD1306 I²C protocol driver with initialization, command/data methods, and display control. Follows the `Hd44780Driver` pattern.

**OledDisplay**: Actuator with 1KB frame buffer (128x64), 5x7 font bitmap, text rendering, and drawing primitives. Commands: `UPDATE_DISPLAY_COMMAND` (350), `CLEAR_DISPLAY_COMMAND` (351), `SET_BACKLIGHT_COMMAND` (352).

**Tests**: 14 host tests covering constructor, commands, text rendering, drawing primitives, and edge cases.

**Source**: `src/Ssd1306Driver.h`, `src/Ssd1306Driver.cpp`, `src/OledDisplay.h`, `src/OledDisplay.cpp`, `tests/OledDisplay/test.cpp`

### EnvironmentalGasSensor

**Readings**: Temperature (°C), humidity (%), pressure (hPa), and gas resistance (kΩ) via Bosch BME680 compensation formulas.

**Change Detection**: Separate epsilon thresholds per value type (0.1°C, 0.1%, 0.1 hPa, 1000 kΩ).

**Sampling**: Default 2000ms, minimum 500ms.

**Tests**: 11 host tests covering constructor, all four compensation formulas, change detection, and event emission.

**Source**: `src/EnvironmentalGasSensor.h`, `src/EnvironmentalGasSensor.cpp`, `tests/EnvironmentalGasSensor/test.cpp`

### BmeEnvironmentalSensor

**Purpose**: Abstract base class for Bosch BME280/BME680 I²C sensors, extracting shared compensation formulas and calibration data parsing. Eliminates ~200 lines of duplicated code between `PthSensor` (BME280) and `EnvironmentalGasSensor` (BME680).

**Design**: Template-based compensation with digital calibration coefficients. `PthSensor` and `EnvironmentalGasSensor` inherit and extend with model-specific registers and gas resistance (BME680 only).

**Source**: `src/BmeEnvironmentalSensor.h`, `src/BmeEnvironmentalSensor.cpp`

---

## Gateway Client Enhancements (Complete)

Enhanced the GatewayClient family with production-ready features for all three transport options.

### BleGatewayClient (New Component)

**BleGatewayClient**: BLE NUS gateway client extending `GatewayClient` with JSON lines framing, fixed-size 512-byte serialization buffer, statistics tracking (`sendsSucceededCount`/`sendsFailedCount`), `service()`/`loop()` for advertising restart, and `isConnected()` override. Completes the GatewayClient family alongside MqttGatewayClient and HttpGatewayClient.

**Tests**: 8 host tests covering constructor, send failure when not connected, successful send with JSON verification, multiple sends, service/loop advertising restart, connection state tracking, and mixed success/failure statistics.

**Source**: `src/BleGatewayClient.h`, `src/BleGatewayClient.cpp`, `tests/BleGatewayClient/test.cpp`

### HttpGatewayClient Enhancements

**HttpGatewayClient**: Enhanced from a basic HTTP POST stub to a production-ready HTTP telemetry client. Added statistics tracking, authentication support (`setAuthorizationHeader()`), configurable HTTP methods via `enum class HttpMethod` (GET/POST/PUT/PATCH/DELETE), response status code reporting (`getLastResponseStatusCode()`), response body capture (`getResponseBody()` with 512-byte fixed buffer), `sendRequest()` for general HTTP requests independent of TelemetryPackage, `isConnected()` override, and `getStatistics()` override. Refactored with `executeHttpRequest()` to eliminate duplication.

**HttpTelemetryDemo example**: Demonstrates Bearer token authentication flow, Wi-Fi connection verification, and HTTP POST telemetry with a DHT22 sensor.

**Source**: `src/HttpGatewayClient.h`, `src/HttpGatewayClient.cpp`, `examples/HttpTelemetryDemo/HttpTelemetryDemo.ino`

### WiFiConnectivityDriver Enhancements

Added 5 must-have features: `getWiFiStatus()` (returns `wl_status_t`), `disconnect()` (resets provisioning flag), `getLocalIpAddress()` (returns `IPAddress`), `getSignalStrengthInDbm()` (returns RSSI in dBm), and `waitForConnection()` (blocking with timeout, 100ms polling).

**Source**: `src/WiFiConnectivityDriver.h`, `src/WiFiConnectivityDriver.cpp`

### BleConnectivityDriver Enhancements

Added `disconnect()` method (stops advertising, resets connected flag) and `getSignalStrengthInDbm()` method (returns 0 with documentation that ESP32 BLE server API doesn't expose RSSI). Replaced raw `new`/`delete` with `std::unique_ptr` for callback handlers.

**Source**: `src/BleConnectivityDriver.h`, `src/BleConnectivityDriver.cpp`

---

## Tier 5: Optimization and Power (Complete)

### W16: Tickless scheduling and light sleep — **Complete** ✓

**Problem**: The hardware timer wakes the system at the tick rate forever, preventing sleep.

**Resolution**: With the pure scheduler, the next due time is known. Program the single timer alarm for that moment instead of a fixed period:
- `SamplingScheduler::ticksUntilNextDue()` returns the smallest `ticksUntilDue`.
- After each dispatch, the sampler re-arms the one-shot alarm for that interval.
- In tickless mode, the device enters ESP32 light sleep until the next alarm fires.

**Implementation**:
- `SamplingScheduler::ticksUntilNextDue()` — returns minimum ticks until any slot is due.
- `Device::PowerMode` enum — `Periodic` (default) or `Tickless` (one-shot timer + light sleep).
- `Device::rearmTimerForNextDue()` — calculates and sets one-shot alarm based on `ticksUntilNextDue()`.
- `Device::enterLightSleep()` — configures timer wake source and calls `esp_light_sleep_start()`.

**Source**: `src/SamplingScheduler.h`, `src/SamplingScheduler.cpp`, `src/Device.h`, `src/Device.cpp`

---

### W17: Deep-sleep cycle and battery example — **Complete** ✓

**Problem**: Battery-powered devices need true deep sleep (~10µA) rather than light sleep (~100mA). Deep sleep shuts down the CPU, Wi-Fi, and most peripherals, retaining only RTC memory.

**Resolution**: Added `PowerMode::DeepSleep` to the existing `PowerMode` enum. When set, the device enters deep sleep after each sampler dispatch via `esp_deep_sleep_start()`. On wake, the device reports the wake reason and re-initializes all sensors.

**Implementation**:
- `PowerMode::DeepSleep` enum value — triggers `enterDeepSleep()` after sampler dispatch.
- `Device::enterDeepSleep()` — configures timer wake source via `esp_sleep_enable_timer_wakeup()` and calls `esp_deep_sleep_start()`.
- `Device::reportWakeReason()` — uses `esp_sleep_get_wakeup_cause()` to report the wake source (timer, GPIO, reset).
- `RTC_DATA_ATTR` support — application state survives deep sleep using RTC slow memory.
- `WeatherStation` example — demonstrates deep sleep with DHT22 and BH1750 sensors, RTC boot count tracking, and wake-reason reporting.
- 10 host tests in `tests/DeepSleep/test.cpp` covering PowerMode enum, all wake causes, RTC_DATA_ATTR, and deep sleep API mocks.

**Energy Saving**: Deep sleep reduces power from ~100mA (active) to ~10µA (sleeping), a 10,000x reduction. With 5-minute sleep intervals, the device spends >99.9% of its time asleep, enabling months of operation on a coin cell battery.

**Source**: `src/Device.h`, `src/Device.cpp`, `examples/WeatherStation/WeatherStation.ino`, `tests/DeepSleep/test.cpp`

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

### W19: Native servo and LCD drivers — **Complete** ✓

**ESP32Servo Replacement**: ServoMotor now uses native `ledcAttach()`/`ledcWrite()` at 50 Hz with 13-bit LEDC resolution. ESP32 Arduino core 3.x API (pin-based, not channel-based). 10 host tests in `tests/ServoMotor/test.cpp`.

**Hd44780Driver (new component)**: Extracted HD44780 4-bit mode I2C protocol into a standalone driver class. Implements full initialization sequence, enable pulse timing, PCF8574 backpack communication, cursor positioning via DDRAM addresses, and backlight control. CharacterLcdDisplay composes `Hd44780Driver` instead of implementing the protocol directly. 10 host tests in `tests/CharacterLcdDisplay/test.cpp`.

**library.properties**: Removed `ESP32Servo`, `LiquidCrystal_I2C`, `PubSubClient`, and `DHT sensor library` from the `depends=` field. Framework now has only 2 external dependencies: ArduinoJson (>=7.0.0) and MQTT (>=2.5.3).

**DHT Native Implementation**: Replaced `DHT sensor library` with native single-wire protocol in `DhtSensor`. Implemented `sendStartSignal()`, `readBit()`, `readData()`, and `validateChecksum()` methods. All timing magic numbers replaced with named constants (e.g., `DHT22_START_SIGNAL_LOW_TIME_IN_MICROSECONDS`). DHT11 and DHT22 model-specific start signal durations supported. CRC validation (8-bit checksum) via `validateChecksum()`. 11 host tests in `tests/DhtSensor/test.cpp`.

**Source**: `src/ServoMotor.h`, `src/ServoMotor.cpp`, `src/Hd44780Driver.h`, `src/Hd44780Driver.cpp`, `src/CharacterLcdDisplay.h`, `src/CharacterLcdDisplay.cpp`, `src/DhtSensor.h`, `src/DhtSensor.cpp`

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
| Cloud-to-device | Hand-rolled callback                         | Not supported                                                     | Mediator-routed commands through `Device::enqueueCommand` |
| Testing         | Rarely possible off-device                   | 27 host test suites with CI                                       | Same                                                     |

### Performance

| Aspect                     | Traditional `loop()`              | Modest IoT today                              | Modest IoT after Tier 4                               |
|----------------------------|-----------------------------------|-----------------------------------------------|-------------------------------------------------------|
| Timing model               | Cooperative polling; jitter grows | Timer ISR plus sampler task; per-slot periods | Same engine; interrupt-driven sensors                 |
| Sample period accuracy     | Depends on `loop()` duration      | Per-slot periods independent of slot count    | Same                                                  |
| Worst-case stall           | Any blocking call                 | Interrupt-driven HX711/ultrasonic; documented budgets    | Interrupt-driven HX711/ultrasonic; documented budgets |
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
| Third-party dependencies | As needed            | Only those used (ArduinoJson, MQTT) — ESP32Servo, LiquidCrystal_I2C, PubSubClient, DHT, Adafruit, and HX711 removed | Same                                               |
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
