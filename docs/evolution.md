# Modest IoT Framework Evolution

Historical record of completed work items W1–W28 with implementation details.

> This document records completed work. For current status and planned improvements, see [improvements.md](improvements.md).

---

## Tier 0: Compilation & Code Hygiene (Complete)

### W1: Fix abstract `WiFiConnectivityDriver` (D1)

`WiFiConnectivityDriver` did not implement the pure-virtual `transmit()` inherited from `ConnectivityDriver.h`, making it an abstract class that could not be instantiated.

**Resolution**: Implemented `WiFiConnectivityDriver::transmit()` to resolve the compilation blocker. All Wi-Fi-backed devices and gateways now instantiate properly.

**Source**: `src/WiFiConnectivityDriver.h`, `src/WiFiConnectivityDriver.cpp`

---

### W2: Delete dead `LoadCellController`, unused deps, unused includes, pin ArduinoJson

**Resolution**:
- Deleted `LoadCellController.h/.cpp` and its `keywords.txt` entry.
- Removed unused dependencies from `library.properties` (HX711, EspSoftwareSerial, Adafruit Unified Sensor, LiquidCrystal_I2C, DHT sensor library, ESP32Servo, PubSubClient).
- Pinned `ArduinoJson (>=7.0.0)` in `library.properties`.
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
  > **Note**: The `.ino` file name uses hyphens as a file system convention.
- Updated `.gitignore` rules (`.DS_Store`, `/cmake-build-arduino/`, `/build/`, `*.o`).
- Aligned `CMakeLists.txt` `SKETCH_DIR` to `examples/StatusIndicator` with cache variable.

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
  - `host-tests.yml`: runs all 36 test suites on Ubuntu and macOS.
  - `compile-examples.yml`: compiles all 23 examples across ESP32 and ESP32-S3 boards.
  - `lint.yml`: validates `library.properties` with Arduino Library Manager compliance.
- Registered all test targets in `CMakeLists.txt` with CTest integration.

**Test Suites (36 total)**:

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
| 11 | `tests/EngineConfig/`                | `EngineConfiguration` defaults, custom values, aggregate init                 |
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
| 28 | `tests/HttpGatewayClient/`           | HTTP methods, auth headers, response body, statistics tracking        |
| 29 | `tests/Buzzer/`                      | Buzzer commands (TURN_ON, TURN_OFF, PLAY_TONE, PLAY_FREQUENCY), tone/noTone mock |
| 30 | `tests/ConfigurationStore/`          | Namespace isolation, type-safe getters/setters, default fallbacks, read-only mode, schema validation |
| 31 | `tests/TimeSynchronizationDriver/`   | NTP initialization, timestamp fallback, sync status, RTC save/restore |
| 32 | `tests/GasSensor/`                   | Analog gas detection, calibration modes, Rs/R0 ratio, PPM estimation  |
| 33 | `tests/Button/`                      | Debounce logic, press detection, timing, event emission               |
| 34 | `tests/PowerManager/`                | Battery voltage sensing, level calculation, low-battery threshold     |
| 35 | `tests/TelemetryBuffer/`             | LittleFS persistence, FIFO store/retrieve, eviction callback          |
| 36 | `tests/OtaUpdateHandler/`            | OTA status tracking, callback type aliases, hostname, re-initialization |

**Component Test Coverage**:

| Category | Total Headers | Dedicated Test | Indirect Coverage | No Test Coverage |
|----------|--------------|---------------|-------------------|-----------------|
| Core | 16 | 10 | 6 (base classes, umbrella) | 0 |
| Sensors | 18 | 12 | 1 (BmeEnvironmentalSensor) | **5** |
| Actuators | 7 | 6 | 0 | **1** |
| Connectivity | 3 | 1 | 2 (base class, Wi-Fi via gateways) | 0 |
| Gateway Clients | 4 | 3 | 1 (base class) | 0 |
| Drivers | 2 | 0 | 2 (via display tests) | 0 |
| **Total** | **51** | **31** | **12** | **6** |

**Components with No Dedicated Test Coverage**:

| Header | Complexity | Risk | Recommendation |
|--------|-----------|------|----------------|
| **PassiveInfraredSensor.h** | Low (digital input) | Low | Optional — simple GPIO |
| **UltrasonicSensor.h** | Medium (timing, distance calc) | Low | Optional — timing hard to mock |
| **GeolocationSensor.h** | Medium (NMEA parsing) | Low | Optional — UART parsing |
| **LoadCellAmplifier.h** | Medium (HX711 protocol) | Low | Optional — custom protocol |
| **PulseSensor.h** | Low (analog read) | Low | Optional — simple ADC |
| **Led.h** | Low (digital write) | Low | Optional — simplest actuator |

**Previously Added Tests**:
- **GasSensor** (32 tests, W23): Calibration modes, Rs/R0 ratio, PPM estimation, digital gas detection.
- **Button** (14 tests, W23): Debounce logic, press detection, timing, event emission.

**Notes**:
- `LocationData.h` is a plain data struct — no test needed.
- Base classes (`Sensor`, `Actuator`, `ConnectivityDriver`, `GatewayClient`) are tested via their subclasses.
- `ModestIoT.h` is an umbrella header — no test needed.
- The 6 untested components are simple GPIO/ADC wrappers well-demonstrated in examples.

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

### W10: BLE driver (Complete)

**BleConnectivityDriver**:
- Implemented Nordic UART Service (NUS) GATT profile with 128-bit UUIDs.
- TX characteristic with GATT notifications, RX characteristic with write callbacks.
- Automatic advertising lifecycle recovery on disconnect.
- Host unit tests covering connection state, transmission, and RX events.

**BleGatewayClient**:
- JSON lines framing over BLE NUS with 512-byte transmit buffer.
- Connection monitoring with automatic reconnection on disconnect.
- Gateway statistics tracking (packets sent, bytes transmitted, errors).
- Host unit tests covering transmission, connection state, and statistics.

**BluetoothConnectivityDriver**:
- Fully removed. The legacy driver was initially isolated behind preprocessor guards and marked `[[deprecated]]`, then deleted as Arduino-ESP32 Core 4.0.0 drops `BluetoothSerial` support and modern ESP32 variants (S3, C3, C6, H2) lack Bluetooth Classic hardware.

**Optional MQTT footprint**:
- Achieved: only 2 dependencies (ArduinoJson ≥7.0.0, MQTT ≥2.5.3). All other external dependencies removed via native implementations (ESP32Servo → native LEDC, LiquidCrystal_I2C → Hd44780Driver, PubSubClient → MqttGatewayClient, DHT → DhtSensor native, HX711 → LoadCellAmplifier native).

**Source**: `src/BleConnectivityDriver.h`, `src/BleConnectivityDriver.cpp`, `src/BleGatewayClient.h`, `src/BleGatewayClient.cpp`, `tests/BleConnectivityDriver/test.cpp`, `tests/BleGatewayClient/test.cpp`

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

**Example Migration**: All 22 examples migrated. `StatusIndicator` uses explicit four-argument `EventHandler*` overload. 21 other examples automatically use `Sensor*` overload.

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
- **Examples migrated**: `TogglableLedDevice`, `SmartBandMqtt`, `AirConditioningDevice` (Button → interrupt), `PerimeterMonitor`, `OutdoorSecurityLightController` (PIR → interrupt).
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

**ClimateDataUploader example**: Demonstrates Bearer token authentication flow, Wi-Fi connection verification, and HTTP POST telemetry with a DHT22 sensor.

**Source**: `src/HttpGatewayClient.h`, `src/HttpGatewayClient.cpp`, `examples/ClimateDataUploader/ClimateDataUploader.ino`

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

**Superseded by W46** (ArduinoJson removal, see [improvements.md](improvements.md)).

The `TelemetryWriter` format abstraction portion has been reprioritized as W46. The packet pool with placement-new remains deferred pending heap measurements.

**Source**: `src/TelemetryPackage.h`

---

### W19: Native servo and LCD drivers — **Complete** ✓

**ESP32Servo Replacement**: ServoMotor now uses native `ledcAttach()`/`ledcWrite()` at 50 Hz with 13-bit LEDC resolution. ESP32 Arduino core 3.x API (pin-based, not channel-based). 10 host tests in `tests/ServoMotor/test.cpp`.

**Hd44780Driver (new component)**: Extracted HD44780 4-bit mode I2C protocol into a standalone driver class. Implements full initialization sequence, enable pulse timing, PCF8574 backpack communication, cursor positioning via DDRAM addresses, and backlight control. CharacterLcdDisplay composes `Hd44780Driver` instead of implementing the protocol directly. 10 host tests in `tests/CharacterLcdDisplay/test.cpp`.

**library.properties**: Removed `ESP32Servo`, `LiquidCrystal_I2C`, `PubSubClient`, and `DHT sensor library` from the `depends=` field. Framework now has only 2 external dependencies: ArduinoJson (>=7.0.0) and MQTT (>=2.5.3).

**DHT Native Implementation**: Replaced `DHT sensor library` with native single-wire protocol in `DhtSensor`. Implemented `sendStartSignal()`, `readBit()`, `readData()`, and `validateChecksum()` methods. All timing magic numbers replaced with named constants (e.g., `DHT22_START_SIGNAL_LOW_TIME_IN_MICROSECONDS`). DHT11 and DHT22 model-specific start signal durations supported. CRC validation (8-bit checksum) via `validateChecksum()`. 11 host tests in `tests/DhtSensor/test.cpp`.

**Source**: `src/ServoMotor.h`, `src/ServoMotor.cpp`, `src/Hd44780Driver.h`, `src/Hd44780Driver.cpp`, `src/CharacterLcdDisplay.h`, `src/CharacterLcdDisplay.cpp`, `src/DhtSensor.h`, `src/DhtSensor.cpp`

---

## Pre-Release Prerequisites (Complete, v1.0.0)

### W20: Persistent configuration store (ESP32 NVS) — **Complete**

`ConfigurationStore` with namespace isolation, type-safe getters/setters, default fallbacks, read-only mode, and schema validation. 66 host tests.

**Source**: `src/ConfigurationStore.h`, `src/ConfigurationStore.cpp`, `tests/ConfigurationStore/test.cpp`

### W21: Time synchronization (NTP + RTC fallback) — **Complete**

`TimeSynchronizationDriver` with NTP support over Wi-Fi and RTC fallback for deep-sleep devices. Replaces `millis()` timestamps with Unix epoch seconds. 8 host tests.

**Source**: `src/TimeSynchronizationDriver.h`, `src/TimeSynchronizationDriver.cpp`, `tests/TimeSynchronizationDriver/test.cpp`

### W22: TLS/SSL examples & documentation — **Complete**

`MqttTlsDevice` example with `WiFiClientSecure`, Secure Communication section in README documenting both MQTT and HTTP TLS.

**Source**: `examples/MqttTlsDevice/MqttTlsDevice.ino`, `README.md`

### W23: Command payload support — **Complete**

`Command(int, int32_t)` constructor, `DRIVE_TO_ANGLE_COMMAND`, `ADJUST_BRIGHTNESS_COMMAND`, `adjustColorSetIntensity()`, `PLAY_FREQUENCY_COMMAND`.

**Source**: `src/Command.h`, `src/ServoMotor.h`, `src/RgbLed.h`, `src/Buzzer.h`

### W24: WiFiConnectivityDriver secure socket builder — **Complete**

`createSecureSocket()` builder method with optional CA certificate pinning.

**Source**: `src/WiFiConnectivityDriver.h`, `src/WiFiConnectivityDriver.cpp`

---

## Post-Release Improvements (Complete, v1.1.0)

### W25: Telemetry buffering with LittleFS persistence — **Complete** ✓

`TelemetryBuffer` with LittleFS persistence, monotonic indices, FIFO eviction, eviction callback, `getMaxPacketsCount()`, 22 host tests.

**Source**: `src/TelemetryBuffer.h`, `src/TelemetryBuffer.cpp`, `tests/TelemetryBuffer/test.cpp`

### W26: Power manager & battery monitoring — **Complete** ✓

`PowerManager` with battery voltage sensing, level calculation, low-battery threshold, 18 host tests.

**Source**: `src/PowerManager.h`, `src/PowerManager.cpp`, `tests/PowerManager/test.cpp`

### W27: Firmware version in telemetry envelope — **Complete** ✓

Optional `schemaVersion` and `firmwareVersion` fields in telemetry envelope metadata. Set via `Device::setFirmwareVersion()` and `Device::setTelemetrySchemaVersion()`. 5 new tests.

**Source**: `src/TelemetryPackage.h`, `src/Device.h`, `src/Device.cpp`, `tests/TelemetryPackage/test.cpp`

### W28: OTA update support — **Complete** ✓

`OtaUpdateHandler` with hostname, password, progress/error/start/end callbacks, 21 host tests.

**Source**: `src/OtaUpdateHandler.h`, `src/OtaUpdateHandler.cpp`, `tests/OtaUpdateHandler/test.cpp`

---

## Post-Release Improvements (Complete, v1.2.0)

### W46: Remove ArduinoJson via TelemetryWriter — **Complete** ✓

**Problem**: ArduinoJson consumed ~30-40 KB of Flash and allocated a `JsonDocument` on the heap for every telemetry serialization. The `JsonDocument` type appeared in the public API (`TelemetryPackage::serialize`), coupling all telemetry packages to an external library.

**Resolution**: Introduced `TelemetryWriter` abstract interface and `JsonTelemetryWriter` native implementation to eliminate ArduinoJson entirely.

**Implementation**:

- **TelemetryWriter interface** (`src/TelemetryWriter.h`): Abstract class with pure virtual `field()` overloads for `int32_t`, `float`, `bool`, and `const char*`. Decouples telemetry packages from the serialization format, enabling future format writers (CBOR, MessagePack, binary).
- **JsonTelemetryWriter** (`src/JsonTelemetryWriter.h/cpp`): `snprintf`-based JSON writer using a 512-byte fixed stack buffer. Zero heap allocation per serialization, automatic comma handling, buffer overflow detection, and string escaping for special characters.
- **TelemetryPackage migration**: Replaced `serialize(JsonDocument&)` with `write(TelemetryWriter&)` pure virtual method. Removed `toDocument()` method, simplified `toJsonString()` to use `JsonTelemetryWriter`. Added `serializeToBuffer()` single-call pipeline that encapsulates envelope metadata, data fields, finalization, overflow check, and memcpy.
- **Gateway client migration**: `MqttGatewayClient`, `BleGatewayClient`, and `HttpGatewayClient` now use `serializeToBuffer()` instead of manual `JsonTelemetryWriter` sequences. Removed `#include <ArduinoJson.h>` from all three.
- **Native JSON parser**: `MqttGatewayClient` uses `JsonParser::parseInt()` and `JsonParser::parseInt32()` for MQTT command parsing. The `JsonParser` static class provides framework-wide JSON parsing (int, int32, float, bool, string) with zero heap allocation, replacing the file-scope static functions.
- **Example migration**: 6 examples with TelemetryPackage subclasses migrated from `serialize(JsonDocument&)` to `write(TelemetryWriter&)`.
- **Test migration**: 7 TelemetryPackage test subclasses migrated, 15 golden tests rewritten for insertion-order JSON output. `tests/fakes/ArduinoJson.h` (253 lines) deleted, replaced with `tests/fakes/TelemetryWriter.h` simple mock.
- **GasSensor utilities**: Added `gasTypeName()` and `operatingModeName()` static methods for enum-to-string conversion, removing custom implementations from examples.

**Benefits**:
- ~30-40 KB Flash saved (2.5-5% of usable Flash on small MCUs)
- Zero heap allocation per serialization on the network task
- 253 lines of test mock code eliminated
- Dependency count reduced from 2 to 1 (only MQTT >=2.5.3 remains)
- No external types in the public API
- Format-neutral telemetry enables future compact binary protocols

**Source**: `src/TelemetryWriter.h`, `src/JsonTelemetryWriter.h`, `src/JsonTelemetryWriter.cpp`, `src/TelemetryPackage.h`, `src/MqttGatewayClient.cpp`, `src/BleGatewayClient.cpp`, `src/HttpGatewayClient.cpp`, `src/GasSensor.h`, `src/GasSensor.cpp`, `tests/fakes/TelemetryWriter.h`

---

### W32: High-Priority Telemetry Lane — **Complete** ✓

**Problem**: Critical alerts (water leak, fire, security breach) were enqueued in the standard telemetry queue alongside routine data. The network task processed packets in FIFO order, so alerts could be delayed behind routine telemetry.

**Resolution**: Added a separate high-priority queue (4 slots) that the network task checks first (non-blocking) before polling the standard queue. High-priority packets include `"priority": "high"` in the JSON envelope for edge-side routing.

**Implementation**:

- **Device::enqueueHighPriorityPayload()**: Enqueues to the high-priority queue with envelope metadata and priority marker. Bounded queue (4 slots) drops new alerts when full (no eviction, as alerts are too critical to replace).
- **TelemetryPackage::markAsHighPriority()**: Sets the high-priority flag, adding `"priority": "high"` to the envelope during serialization.
- **TelemetryPackage::isHighPriority()**: Accessor for the high-priority flag.
- **Network task modification**: Checks high-priority queue first (non-blocking `xQueueReceive`), then standard queue (100ms timeout). One alert per loop iteration prevents starvation of routine telemetry.
- **DiagnosticsSnapshot extension**: New fields `highPriorityEnqueued`, `highPriorityDropped`, `highPriorityQueueHighWaterMark` for monitoring queue pressure.
- **Device refactoring**: Extracted `assignEnvelopeMetadata()` helper (shared by both enqueue methods) and `drainTelemetryQueue()` static helper (shared cleanup in `stopAsynchronousEngine()`).
- **Example destructor safety**: 9 Device subclasses replaced `= default` destructors with proper `stopAsynchronousEngine()` calls to ensure the engine stops before member destruction.

**Benefits**:
- Critical alerts bypass routine telemetry queue, reducing latency from queue depth to near-zero
- Bounded queue prevents flooding while allowing burst of up to 4 simultaneous alerts
- `"priority": "high"` envelope field enables edge-side routing without transport-level changes
- Diagnostics track dropped alerts and queue pressure for fleet monitoring

**Source**: `src/Device.h`, `src/Device.cpp`, `src/TelemetryPackage.h`

---

### W32 Example: HomeSecurityDevice High-Priority Lane Demonstration — **Complete** ✓

**Problem**: The high-priority telemetry lane (W32) was implemented but had no example demonstrating both `enqueueTelemetryPayload()` and `enqueueHighPriorityPayload()` side-by-side.

**Resolution**: Extended `HomeSecurityDevice` with MQTT gateway integration and two TelemetryPackage subclasses. Routine status updates (door closed, motion stopped, leak cleared) use the standard queue via `enqueueTelemetryPayload()`. Critical alerts (water leak detected, door opened) use the high-priority queue via `enqueueHighPriorityPayload()`, with `"priority": "high"` in the JSON envelope.

**Implementation**:

- **SecurityStatusTelemetryPackage**: Routine telemetry with `doorOpen`, `motionDetected`, and `waterLeak` boolean fields. Enqueued via `enqueueTelemetryPayload()` for non-critical status updates.
- **SecurityAlertTelemetryPackage**: Critical alert telemetry with `alertType` (string) and `alertState` (boolean) fields. Enqueued via `enqueueHighPriorityPayload()` for time-sensitive alerts.
- **on(Event) handler**: Clearly comments which events go to which queue — water leak and door opened are CRITICAL (high-priority lane), while door closed, motion detected/stopped, and leak cleared are ROUTINE (standard queue).
- **3 golden tests**: SecurityStatus routine telemetry (no priority field), SecurityAlert with `markAsHighPriority()` (priority field present), SecurityAlert without `markAsHighPriority()` (no priority field).

**Benefits**:
- Developers can see both enqueue paths side-by-side in a realistic security monitoring scenario
- The example serves as a template for any application that needs to distinguish between routine telemetry and critical alerts
- Clear comments in the `on(Event)` handler explain the rationale for each enqueue decision

**Source**: `examples/HomeSecurityDevice/HomeSecurityDevice.ino`, `tests/TelemetryPackage/test.cpp`

---

### W33: Schema Registry — **Complete** ✓

**Problem**: All TelemetryPackage subclasses shared a single global schema version from `Device::setTelemetrySchemaVersion()`. When one telemetry package evolved its schema (adding/removing fields), the global version became ambiguous — the edge framework couldn't distinguish between different package formats.

**Resolution**: Added per-package schema versioning. Each TelemetryPackage subclass declares its own schema version via `schemaVersionValue`. The `setVersionMetadata()` method falls back to the global schema only when the per-package version is not set, enabling safe schema evolution per package type.

**Implementation**:

- **TelemetryPackage::setVersionMetadata()**: Now checks if `schemaVersionValue` is already set before applying the global schema. Per-package version takes priority, global is only a fallback.
- **DiagnosticsTelemetryPackage**: Declares `static constexpr const char* SCHEMA_VERSION = "1.0"` and sets `schemaVersionValue` in the constructor.
- **SmartBandTelemetryPackage** (SmartBandMqtt example): Declares per-package schema version "1.0".
- **SecurityStatusTelemetryPackage** and **SecurityAlertTelemetryPackage** (HomeSecurityDevice example): Both declare per-package schema version "1.0".
- **2 new host tests**: Per-package schema priority over global, and fallback to global when per-package is not set.
- **Golden test update**: DiagnosticsTelemetryPackage golden test now includes `"schemaVersion": "1.0"` in expected JSON output.

**Benefits**:
- Each telemetry package can evolve its schema independently without breaking other packages
- Edge framework can validate schema versions per package type, rejecting unknown formats
- Backward compatible: packages without per-package schema use the global value
- Safe fleet management: old and new devices can coexist with different schema versions

**Source**: `src/TelemetryPackage.h`, `src/DiagnosticsTelemetryPackage.h`, `src/DiagnosticsTelemetryPackage.cpp`, `examples/SmartBandMqtt/SmartBandMqtt.ino`, `examples/HomeSecurityDevice/HomeSecurityDevice.ino`, `tests/TelemetryPackage/test.cpp`

---

### W34: MQTT QoS and Retained Messages — **Complete** ✓

**Problem**: MqttGatewayClient hardcoded QoS 0 for all MQTT operations and had no retained message support. Production deployments often require QoS 1 (at least once delivery) for critical telemetry, and retained messages for last-known-good state that new subscribers can receive immediately.

**Resolution**: Added configurable QoS (0/1/2) for both publish and subscribe operations, and retained message support for last-known-good telemetry. All changes are backward-compatible with defaults matching previous behavior (QoS 0, non-retained).

**Implementation**:

- **Constructor parameters**: Both constructors accept optional `publishQos`, `subscribeQos`, and `retainedFlag` parameters with backward-compatible defaults.
- **Setter/getter methods**: `setPublishQoS()`, `getPublishQoS()`, `setSubscribeQoS()`, `getSubscribeQoS()`, `setRetained()`, `getRetained()` for runtime configuration.
- **publish() call**: Updated to pass `retained` and `publishQoS` to `mqttClient.publish()`.
- **subscribe() call**: Updated to use `subscribeQoS` member instead of hardcoded `DEFAULT_MQTT_QOS` constant.
- **Test mock**: Updated `tests/fakes/MQTT.h` to track `lastPublishQos` and `lastPublishRetained` values.
- **5 new host tests**: Default values, setter/getter round-trip, publish with QoS 1, publish with retained flag, subscribe with QoS 1.

**Benefits**:
- QoS 1 ensures at-least-once delivery for critical telemetry without code changes
- Retained messages allow new subscribers to receive the last-known-good state immediately
- Per-operation QoS configuration (publish vs subscribe) enables fine-grained control
- Backward-compatible defaults ensure existing code continues to work without modification

**Source**: `src/MqttGatewayClient.h`, `src/MqttGatewayClient.cpp`, `tests/fakes/MQTT.h`, `tests/MqttGatewayClient/test.cpp`

---

### JsonParser: Native JSON Parser — **Complete** ✓

**Problem**: The native JSON parser in MqttGatewayClient consisted of two static functions (`parseJsonInt`/`parseJsonInt32`) that were file-scope only. Developers building custom devices needed JSON parsing for custom MQTT commands, HTTP response extraction, and BLE notification parsing, but had to either copy the parser code or use ArduinoJson (defeating W46's purpose).

**Resolution**: Extracted the parser into a framework-wide `JsonParser` static class with 5 methods (int, int32, float, bool, string). Zero heap allocation, all operations use stack buffers and C string functions. Migrated MqttGatewayClient to use `JsonParser::parseInt()` and `JsonParser::parseInt32()`.

**Implementation**:

- **src/JsonParser.h**: Static class with `parseInt()`, `parseInt32()`, `parseFloat()`, `parseBool()`, `parseString()` methods. All methods return `bool` for error handling.
- **src/JsonParser.cpp**: Implementation using `strstr`, `strtol`, `strtod` for key-value extraction from flat JSON.
- **MqttGatewayClient.cpp**: Replaced static `parseJsonInt()`/`parseJsonInt32()` with `JsonParser::parseInt()`/`JsonParser::parseInt32()`.
- **ModestIoT.h**: Added `JsonParser.h` to umbrella header for framework-wide access.
- **20 host tests**: Covering all 5 methods with edge cases (null inputs, missing keys, buffer overflow, complex JSON).

**Benefits**:
- Developers can parse JSON in custom MQTT commands, HTTP responses, and BLE notifications without ArduinoJson
- Symmetric naming with `JsonTelemetryWriter` (output) ↔ `JsonParser` (input)
- Extensible: adding new types (e.g., uint64_t) is trivial
- Zero heap allocation matches the framework's philosophy
- Framework-wide visibility eliminates code duplication across examples

**Source**: `src/JsonParser.h`, `src/JsonParser.cpp`, `src/MqttGatewayClient.cpp`, `src/ModestIoT.h`, `tests/JsonParser/test.cpp`

---

### W41: Remote Diagnostics — **Complete** ✓

**Problem**: Fleet health monitoring required continuous telemetry overhead or manual serial access. There was no way to request on-demand diagnostics from a device via MQTT — developers had to either poll diagnostics continuously (wasting bandwidth) or connect via serial console (impractical for deployed devices).

**Resolution**: Added on-demand diagnostics via MQTT command (commandId = 300). When the cloud sends a diagnostics command, the device creates a `DiagnosticsTelemetryPackage` from `Device::collectDiagnostics()` and enqueues it via the standard telemetry queue. The JSON output contains all 14 health metrics plus envelope metadata.

**Implementation**:

- **DiagnosticsTelemetryPackage** (`src/DiagnosticsTelemetryPackage.h/cpp`): New TelemetryPackage subclass that wraps `DiagnosticsSnapshot`. All 14 fields (missedTicks, worstCaseWakeMicros, packetsEnqueued, packetsDropped, queueHighWaterMark, highPriorityEnqueued, highPriorityDropped, sendsSucceeded, sendsFailed, freeHeapBytes, minFreeHeapBytes, samplerStackFreeWords, networkStackFreeWords, uptimeSeconds) are serialized via `write(TelemetryWriter&)`.
- **Device::handleInternalCommand(Command)**: Private method that handles framework-internal commands (diagnostics, commandId = 300) before dispatching to `handle()`. Returns `true` if consumed, keeping `handle()` purely for developer-defined custom commands.
- **Device::handle(Command)**: Restored as a pure no-op override. Derived classes override this for their custom commands without needing to call the base class first.
- **Device::processPendingCommands()**: Calls `handleInternalCommand()` first for each command. If not consumed, dispatches to `targetHandler->handle()` for developer-defined custom commands.
- **Golden test**: Verifies all 14 fields are present with correct values in the JSON output.

**Benefits**:
- Fleet health monitoring without continuous telemetry overhead
- All 14 DiagnosticsSnapshot fields available on demand via a single MQTT command
- `handle()` is purely for developer-defined custom commands — no framework-internal logic pollutes the developer's API
- Command ID 300 in the reserved 300+ range avoids collision with component-specific IDs (100-253)

**Source**: `src/DiagnosticsTelemetryPackage.h`, `src/DiagnosticsTelemetryPackage.cpp`, `src/Device.h`, `src/Device.cpp`, `tests/TelemetryPackage/test.cpp`

---

### W31: Event Aggregator — **Complete** ✓

**Problem**: The framework's event system supported single-event handling via `EventHandler::on(Event)`, but had no way to detect composite event patterns (e.g., "motion AND door opened within 5 seconds"). Complex automation rules required the application to track event timing manually, leading to error-prone state machines in every Device subclass. Additionally, 9+ sensors share the same `DATA_READ_EVENT_IDENTIFIER = 20`, making it impossible to distinguish which sensor triggered a pattern without source-aware matching.

**Resolution**: Added `EventAggregator` as a framework component for composite event pattern matching with time-windowed detection. It implements `EventHandler` to intercept sensor events, captures each event with its timestamp and sourceId, and checks all registered patterns using AND or OR logic. When a pattern matches, an optional condition function can further filter based on sensor values. Composite events carry `DEVICE_SOURCE_ID` as their sourceId, enabling nested aggregator chains where downstream aggregators distinguish composite events from raw sensor events.

**Implementation**:

- **EventAggregator class** (`src/EventAggregator.h/cpp`): Implements `EventHandler` interface. Fixed-size circular buffer (32 captured events) for event history and fixed-size pattern array (4 patterns) — zero heap allocation.
- **`addAllMatchPattern()` (3 overloads)**: Registers AND-patterns where all required event identifiers must occur within the time window. Overloads support: basic (any source), source-aware (per-position source filtering with `EventAggregator::ANY_SOURCE` wildcards), and conditional (with `PatternCondition` callback receiving `compositeEventId` context).
- **`addAnyMatchPattern()` (2 overloads)**: Registers OR-patterns where any single event identifier triggers the composite event. Overloads support: basic (any source) and source-aware.
- **`PatternCondition` callback**: `bool (*)(int compositeEventId)` — allows the Device descendant to suppress composite events based on sensor values at the moment of the match (e.g., "only turn off cooler if cargo bay is empty").
- **`PatternLogic` enum**: Internal `And`/`Or` discriminator in `EventPattern` struct guiding the matching strategy in `checkForEventPatternMatch()`.
- **`EventAggregator::ANY_SOURCE`** (value -1): Public constant for wildcard source matching in source-aware patterns.
- **`EventAggregator::DEVICE_SOURCE_ID`** (value 0): Public constant used as the sourceId of every composite event, enabling nested aggregators to filter composite events from raw sensor events.
- **`removePattern(compositeEventId)`**: Dynamically removes a pattern by composite event ID, keeping the array compact. Enables dynamic rule management for W29 (Rules Engine).
- **`cleanupPatterns()`**: Removes all registered patterns in one call for proper housekeeping in destructors.
- **`setOutputHandler()`**: Sets the `EventHandler` that receives composite events when patterns match. For nested aggregators, the output handler is another `EventAggregator` instance.
- **`on(Event)`**: Captures the event with timestamp and sourceId, checks all registered patterns using the appropriate logic (AND/OR), evaluates condition functions, and emits composite events via the output handler.
- **Private `checkForEventPatternMatch()` helper**: Iterates all patterns, branches on `PatternLogic` (And/Or), checks both identifier and sourceId per position, and evaluates condition functions before emitting.
- **Private `removeExpiredCapturedEvents()` helper**: Removes captured events older than the maximum pattern time window to prevent buffer growth.
- **43 host tests**: Covering single event patterns, two-event AND patterns within/outside time windows, three-event patterns, multiple patterns, captured event cleanup, circular buffer overflow, no patterns registered, pattern array full, invalid parameters, no output handler, reverse event order, time window boundary conditions, duplicate event identifiers, repeated pattern matches, shared events across patterns, source-aware exact match, source-aware wrong source rejection, source-aware wildcard, mixed exact and wildcard sources, two patterns with different sources on the same identifier, backward compatibility with the original API, condition function receives compositeEventId (emits), condition function returns false (suppresses), nullptr condition (always emits), condition-based sensor value check, shared condition function handling multiple patterns via compositeEventId, remove existing pattern, remove non-existent pattern, remove and re-add pattern (slot reuse), remove middle pattern (array compaction), cleanup all patterns, OR pattern single event triggers, OR pattern neither event fires, OR pattern with source filtering, OR pattern time window expiry, mixed AND and OR patterns coexisting, OR pattern array full, OR pattern remove and cleanup, nested aggregators basic chain, nested aggregators source filtering by DEVICE_SOURCE_ID, and nested aggregators three-level chain.

**Examples**:

- **IntrusionDetector** (`examples/IntrusionDetector/IntrusionDetector.ino`): Standard AND patterns that always emit. Two composite patterns: intrusion alert (motion AND door opened within 10 seconds) and leak near entry (water leak AND door opened within 30 seconds). Demonstrates named pattern setup methods and `cleanupPatterns()` housekeeping.
- **ColdChainMonitor** (`examples/ColdChainMonitor/ColdChainMonitor.ino`): Conditional patterns with source-aware matching. Three composite patterns: energy save (door AND weight change, **only if** cargo bay is empty — uses `PatternCondition`), unauthorized access (door AND motion stopped), and loading/unloading active (weight AND door AND motion). Demonstrates `PatternCondition` with `compositeEventId` context, source-aware patterns, and the static pointer pattern for condition functions.
- **PharmaceuticalCleanRoom** (`examples/PharmaceuticalCleanRoom/PharmaceuticalCleanRoom.ino`): Nested EventAggregator chains with OR patterns. Three aggregator levels: Level 1 detects environmental deviations (AND + OR patterns), Level 2 correlates contamination risks, Level 3 triggers batch quarantine. Demonstrates `addAnyMatchPattern()` (OR logic), `EventAggregator::DEVICE_SOURCE_ID` for source filtering between levels, `removePattern()` for dynamic removal, and `cleanupPatterns()` on all aggregator levels.

**Benefits**:
- Composite event patterns enable complex automation rules on-device without manual state machines
- Source-aware matching resolves the shared `DATA_READ_EVENT_IDENTIFIER` problem (9+ sensors share ID 20)
- OR patterns complement AND logic, covering the two fundamental boolean combinations
- Condition functions bridge event correlation and value-based decisions without coupling the aggregator to sensor internals
- `DEVICE_SOURCE_ID` enables nested aggregator chains for multi-level pattern detection (faults → situations → escalation)
- Dynamic pattern management (`removePattern`, `cleanupPatterns`) enables W29 (Rules Engine) for declarative on-device automation.
- Zero heap allocation matches the framework's philosophy
- The three examples provide clear, non-overlapping demonstrations: standard patterns, conditional patterns, and nested chains

**Source**: `src/EventAggregator.h`, `src/EventAggregator.cpp`, `tests/EventAggregator/test.cpp`, `examples/IntrusionDetector/IntrusionDetector.ino`, `examples/ColdChainMonitor/ColdChainMonitor.ino`, `examples/PharmaceuticalCleanRoom/PharmaceuticalCleanRoom.ino`

---

### W29: Rules Engine — **Complete** ✓

**Problem**: EventAggregator (W31) provides composite event pattern matching, but the action layer required manual `on(Event)` switch statements in every Device subclass. Developers had to write boilerplate code to map composite events to actuator commands, making automation rules verbose and error-prone. There was no declarative way to say "when this pattern matches, execute these commands."

**Resolution**: Added `RulesEngine` as a framework component that maps composite events from EventAggregator to automated actions (actuator Commands). It implements `EventHandler` so EventAggregator calls `RulesEngine::on(Event)` when a pattern matches. RulesEngine looks up the rule for that composite event ID and dispatches the associated Commands to target CommandHandlers (e.g., actuators). All storage uses fixed-size arrays — zero heap allocation.

**Implementation**:

- **RulesEngine class** (`src/RulesEngine.h/cpp`): Implements `EventHandler` interface. Fixed-size rule array (4 rules) with up to 4 actions per rule — zero heap allocation.
- **`addRule()` (2 overloads)**: Registers a rule mapping a composite event ID to a CommandHandler target and command ID. Optional payload for parameterized commands (e.g., frequency, angle, brightness).
- **`addActionToRule()` (2 overloads)**: Adds an additional action to an existing rule. Enables one pattern to trigger multiple commands (e.g., sound buzzer AND send telemetry).
- **`removeRule(compositeEventId)`**: Dynamically removes a rule by composite event ID, keeping the array compact.
- **`cleanupRules()`**: Removes all registered rules in one call for proper housekeeping in destructors.
- **`on(Event)`**: Looks up the rule for the event's identifier and dispatches all actions via `target->handle(Command(commandId, payload))`. Unknown event IDs are silently ignored.
- **RuleAction struct**: Holds `CommandHandler* target`, `int commandId`, `int32_t payload`, and `bool active`.
- **Rule struct**: Holds `int compositeEventId`, `RuleAction actions[4]`, `uint8_t actionCount`, and `bool active`.
- **30 host tests**: Covering basic rule, multiple actions, rule removal, cleanup, full capacity, payload, unknown event, and edge cases.

**Examples**:

- **SmartFactoryAlarm** (`examples/SmartFactoryAlarm/SmartFactoryAlarm.ino`): Factory alarm system with automated rules. WaterLeakSensor + DoorWindowSensor → EventAggregator AND pattern → RulesEngine → Buzzer PLAY_TONE command. Demonstrates the complete pipeline: sensor events, composite pattern matching, and automated action dispatch without any manual `on(Event)` switch statement.

**Benefits**:
- Declarative rules replace manual `on(Event)` switch statements for common automation patterns
- One pattern can trigger multiple actions (e.g., sound buzzer AND send high-priority telemetry)
- Dynamic rule management (`removeRule`, `cleanupRules`) enables runtime rule changes
- Fixed-size arrays ensure zero heap allocation and predictable memory usage
- Complements EventAggregator: EventAggregator handles the "when" (pattern matching), RulesEngine handles the "then" (action dispatch)
- The SmartFactoryAlarm example provides a clear, grounded demonstration of the complete pipeline

**Source**: `src/RulesEngine.h`, `src/RulesEngine.cpp`, `tests/RulesEngine/test.cpp`, `examples/SmartFactoryAlarm/SmartFactoryAlarm.ino`

---
