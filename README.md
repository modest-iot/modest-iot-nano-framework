# Modest IoT Nano Framework (C++ Edition)

**Version**: 1.0.0  
**Author**: Angel Velasquez  
**Date created**: 2025-03-23  
**Last updated**: 2026-11-12

## Overview

Modest IoT Nano Framework (C++ Edition) is a lightweight, object-oriented, event-driven, and CQRS-inspired Arduino library for resource-constrained IoT projects on ESP32-based devices. It provides reusable abstractions for events, commands, sensors, actuators, connectivity drivers, gateway clients, and complete devices — all orchestrated by an asynchronous FreeRTOS engine with per-slot sampling intervals, zero-cost logging, and runtime diagnostics.

The framework uses a **Mediator pattern** where a `Device` subclass owns and coordinates its sensors and actuators. Sensors emit events that propagate synchronously through the mediator, which issues commands to actuators. Telemetry flows through a queue-based pipeline from the sampler task to the network task, keeping sensor polling and network transmission isolated.

This repository contains the C++ edition of the framework. A MicroPython edition is planned as a separate distribution. For a practical usage example, see [`ToggableLedDevice-Example-Cpp`](https://github.com/avelasquezn/ToggableLedDevice-Example-Cpp).

## Key Characteristics

- **Object-Oriented**: Built with C++ classes, inheritance, and encapsulation for maintainable code.
- **Event-Driven**: Uses an `Event`/`EventHandler` system for asynchronous, reactive behavior.
- **CQRS-Inspired**: Separates `Command` (control) and `Event` (notification) pathways.
- **Asynchronous Engine**: Two FreeRTOS tasks (sampler and network worker) with configurable stack sizes, priorities, and core pinning via `EngineConfig`.
- **Per-Slot Scheduling**: `SamplingScheduler` supports independent polling intervals per sensor, independent of slot count.
- **Zero-Cost Logging**: `MODEST_LOG_*` macros with compile-time level filtering; `MIOT_LOG_*` shorthand aliases available.
- **Runtime Diagnostics**: `DiagnosticsSnapshot` with 12 fields: stack high-water marks, heap stats, queue counters, send statistics, and uptime.
- **Sensor Health**: Debounced fault and recovery events (IDs 22/23) with configurable failure thresholds.
- **Telemetry Pipeline**: `unique_ptr` ownership, drop-oldest queue policy, bounded serialization buffer, and envelope metadata (timestamp/sequence).

## Components

### Core

| Class | Description |
|---|---|
| `Event` | Lightweight event container with identifier and payload fields. |
| `EventHandler` | Base interface for event-driven components (`on(Event)`). |
| `Command` | Lightweight command container with identifier and optional `int32_t` payload for parameterized commands. |
| `CommandHandler` | Base interface for command-driven components (`handle(Command)`). |
| `Device` | Base class combining event and command handling; orchestrates the asynchronous engine, scheduler, and telemetry pipeline. |
| `EngineConfig` | Configuration struct for FreeRTOS task stack sizes, priorities, core pinning, and queue depth. |
| `ConfigurationStore` | Persistent key-value configuration using ESP32 NVS with namespace isolation, type-safe access, and schema validation. |
| `TimeSynchronizationDriver` | NTP-based time synchronization with RTC fallback for deep-sleep devices; replaces `millis()` timestamps with Unix epoch seconds. |
| `DiagnosticsSnapshot` | 12-field runtime health snapshot (stack, heap, queue, timing). |
| `SamplingScheduler` | Per-slot countdown scheduler with phase-aligned re-arming and minimum period enforcement. |
| `TelemetryPackage` | Base class for telemetry payloads with envelope metadata (timestamp, sequence). |

### Sensors

| Class | Description |
|---|---|
| `Sensor` | Abstract base class with fault state machine, sampling interval attributes, and virtual getters. |
| `Button` | GPIO button press detection with debounce. |
| `DhtSensor` | Temperature and humidity from DHT11/DHT22/DHT21/AM2301 modules. |
| `DigitalLightIntensitySensor` | I²C BH1750 light intensity sensor with threshold-based light/dark events. |
| `GeolocationSensor` | GPS module parser for latitude, longitude, and altitude. |
| `GasSensor` | MQ-series gas sensor with custom calibration support. |
| `LoadCellAmplifier` | HX711 load cell amplifier for weight measurement. |
| `PassiveInfraredSensor` | PIR motion detection with motion started/stopped events. |
| `PulseSensor` | Heart rate or pulse detection from analog signal. |
| `OneWireTemperatureSensor` | DS18B20 1-Wire temperature sensor with native bit-banging protocol. |
| `PthSensor` | I²C BME280 sensor for temperature, humidity, and pressure with altitude calculation. |
| `EnvironmentalGasSensor` | I²C BME680 sensor for temperature, humidity, pressure, and gas resistance (VOC detection). |
| `BmeEnvironmentalSensor` | Abstract base for Bosch BME280/BME680 compensation formulas and calibration data. |
| `RfidSensor` | SPI RFID card reader with CRC calculation and card communication. |
| `SoilMoistureSensor` | Analog soil moisture sensor with epsilon-based change detection. |
| `UltrasonicSensor` | HC-SR04 ultrasonic distance measurement. |
| `WaterFlowSensor` | YF-S201 water flow sensor with hybrid interrupt/periodic scheduling. |
| `WaterLeakSensor` | Digital water leak detection (YL-83/FR-101) with interrupt-driven operation. |
| `DoorWindowSensor` | Digital door/window open/close detection (A313/MY-103LY) with interrupt-driven operation. |

### Actuators

| Class | Description |
|---|---|
| `Actuator` | Abstract base class with command propagation. |
| `Buzzer` | PWM-driven buzzer with tone frequency and duration. |
| `Led` | Digital LED with on/off/toggle commands. |
| `RgbLed` | PWM-driven RGB LED with color mixing, common-anode support, and predefined colors. |
| `RelayActuator` | Digital relay with on/off/toggle commands for AC/DC load control. |
| `ServoMotor` | PWM servo motor with angle positioning (native LEDC driver). |
| `CharacterLcdDisplay` | I²C 16x2 character LCD with text, cursor, and backlight control (native HD44780 driver). |
| `OledDisplay` | I²C SSD1306 OLED display (128x64) with frame buffer, text rendering, and drawing primitives. |

### Connectivity

| Class | Description |
|---|---|
| `ConnectivityDriver` | Abstract base interface for network transports (`connect()`, `isConnected()`, `transmit()`). |
| `WiFiConnectivityDriver` | Wi-Fi STA driver with reconnection retry logic and configurable reconnect intervals. |
| `BleConnectivityDriver` | Bluetooth Low Energy driver using Nordic UART Service (NUS) GATT profile; supports all ESP32 variants. |

### Gateway Clients

| Class | Description |
|---|---|
| `GatewayClient` | Abstract base for telemetry gateways with `sendTelemetryRecord()`, `service()`, and `getStatistics()`. |
| `MqttGatewayClient` | MQTT gateway with keep-alive servicing, reconnection backoff, and bounded serialization buffer. |
| `HttpGatewayClient` | HTTP gateway with authentication, configurable methods (GET/POST/PUT/PATCH/DELETE), statistics tracking, response status code reporting, `sendRequest()` for general HTTP requests, and `getResponseBody()` for response capture. |
| `BleGatewayClient` | BLE NUS gateway client for serialized telemetry payloads with JSON lines framing, fixed-size buffer, and statistics tracking. |

### Logging

| Macro | Description |
|---|---|
| `MODEST_LOG_E(tag, fmt, ...)` | Log error-level message. |
| `MODEST_LOG_W(tag, fmt, ...)` | Log warning-level message. |
| `MODEST_LOG_I(tag, fmt, ...)` | Log info-level message. |
| `MODEST_LOG_D(tag, fmt, ...)` | Log debug-level message. |
| `MIOT_LOG_*` | Shorthand aliases for `MODEST_LOG_*` (enabled via `MODEST_USE_SHORT_LOG_MACROS`). |

Log level is controlled by `MODEST_LOG_LEVEL` (0=NONE, 1=ERROR, 2=WARN, 3=INFO, 4=DEBUG, 5=VERBOSE). When compiled out, logging incurs zero Flash and RAM overhead.

## Threading Model

The framework runs on two FreeRTOS tasks plus a hardware timer ISR. Understanding what runs where is essential for correct usage.

| Context | What Runs There |
|---|---|
| **Timer ISR** | `Device::handleHardwareTimerInterrupt`: signals the sampler task via `vTaskNotifyGiveFromISR`. |
| **Sampler Task** (default: priority 3, 4096 bytes, core 1) | `SamplingScheduler` dispatch, `Sensor::on()` and `processEvent()`, mediator `Device::on()`, actuator `handle(Command)`, `TelemetryPackage` creation, and `enqueueTelemetryPayload()`. |
| **Network Task** (default: priority 1, 8192 bytes, core 1) | `processQueuedTelemetryData()`, gateway `sendTelemetryRecord()`, JSON serialization, and packet deletion. Also runs `maintainNetworkServices()` during idle periods. |
| **Arduino `loop()`** | `setup()` (all constructors, `pinMode`, `Wire.begin`, `Device` construction, `initializeAsynchronousEngine()`), then user `loop()` (examples typically call `vTaskDelay`). |

### Threading Rules

1. **Sensor, mediator, and actuator code runs on the sampler task** and must not block for long periods. Slow actuators (e.g., I²C LCD updates) consume the sensor tick budget.
2. **Do not call actuators, sensors, or `Wire` from `loop()` or other tasks.** Use the mediator pattern: let the sampler task drive all hardware interactions.
3. **Telemetry packets are immutable snapshots** passed by ownership transfer through the queue. The network task owns the packet after enqueue succeeds.
4. **Interrupts only set flags** and signal the sampler task; they do not access hardware directly.
5. **The two workers share no mutable framework state** except the queue, so no locks are needed in the core framework.

### Engine Configuration

The `EngineConfig` struct allows tuning stack sizes, priorities, and core pinning:

```cpp
EngineConfig config;
config.samplerStackBytes = 4096;
config.networkStackBytes = 8192;
config.samplerPriority = 3;
config.networkPriority = 1;
config.samplerCore = 1;
config.networkCore = 1;
config.queueLength = 10;
device.initializeAsynchronousEngine(config);
```

Call `device.stopAsynchronousEngine()` to gracefully shut down both tasks, drain the queue, and clean up the timer. Derived classes must call this in their destructor **before** their own destructor body runs.

### Identifier Registry

Event and command identifiers are plain integers. The framework reserves ranges to avoid collisions:

| Space | Range | Owner |
|---|---|---|
| Events | 10 | `Button::BUTTON_PRESSED` |
| Events | 20–21 | `Sensor::DATA_READ`, `MEASURE_DATA_REQUESTED` |
| Events | 22–23 | `Sensor::SENSOR_FAULT`, `SENSOR_RECOVERED` |
| Events | 30–31 | `PassiveInfraredSensor` motion detected/stopped |
| Events | 40 | `GasSensor` digital gas detected |
| Events | 50–51 | `DigitalLightIntensitySensor` light detected/absent |
| Events | 60–61 | `RfidSensor` card detected/removed |
| Events | 70–71 | `WaterLeakSensor` leak detected/cleared |
| Events | 72–73 | `DoorWindowSensor` door opened/closed |
| Commands | 100–102 | `Led` |
| Commands | 150–153 | `ServoMotor` (153: `DRIVE_TO_ANGLE_COMMAND` with payload angle) |
| Commands | 200–202 | `CharacterLcdDisplay` |
| Commands | 250–253 | `Buzzer` (253: `PLAY_FREQUENCY_COMMAND` with payload frequency) |
| Commands | 300–302 | `RelayActuator` |
| Commands | 304–306 | `RgbLed` (304: `ADJUST_BRIGHTNESS_COMMAND` with payload brightness factor) |
| Commands | 350–352 | `OledDisplay` |
| **User range** | **1000+** | **Reserved for application-defined identifiers** |

## Sampling Scheduler

The `SamplingScheduler` replaces fixed-tick round-robin dispatch with per-slot countdown timers. Each registered sensor can have an independent polling period:

```cpp
// Automatic: uses sensor's declared default interval
appendSensorToScheduler(&dhtSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);

// Explicit: custom period with minimum enforcement
appendSensorToScheduler(&button, Button::BUTTON_PRESSED_EVENT_IDENTIFIER, 100, 50);
```

Sensors declare `DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS` and `MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS` constants. The scheduler enforces minimums and reports clamping via `MODEST_LOG_W`.

## Logging

The framework provides zero-cost compile-time filtered logging:

```cpp
MODEST_LOG_I("SmartBand", "Heart rate: %d bpm", heartRate);
MODEST_LOG_W("DhtSensor", "Checksum error on pin %d", pin);
MODEST_LOG_E("MqttGateway", "Connection failed to %s", brokerIp);
```

Set `MODEST_LOG_LEVEL` in your build flags or sketch to control verbosity. At level 0, all logging compiles to `((void)0)` with zero overhead.

## Diagnostics

Call `device.diagnostics()` to get a `DiagnosticsSnapshot` with 12 runtime health fields:

```cpp
DiagnosticsSnapshot health = device.diagnostics();
Serial.printf("Free heap: %u bytes\n", health.freeHeapBytes);
Serial.printf("Packets dropped: %u\n", health.packetsDropped);
Serial.printf("Sampler stack free: %u words\n", health.samplerStackFreeWords);
```

### Sensor Fault Events

Opt into sensor fault detection by calling `setFaultThreshold()` on any sensor:

```cpp
dhtSensor.setFaultThreshold(3); // Emit fault event after 3 consecutive failures
```

When a sensor faults, it emits event ID 22 (`SENSOR_FAULT_EVENT_IDENTIFIER`). When it recovers, it emits event ID 23 (`SENSOR_RECOVERED_EVENT_IDENTIFIER`). The mediator can react by turning on an alert LED, publishing health telemetry, or switching to a safe default state.

## Connectivity

### Wi-Fi

```cpp
WiFiConnectivityDriver wifiDriver("ssid", "password");
wifiDriver.connect();
```

Supports reconnection retry with configurable intervals. Automatically re-arms after transient access point outages.

### Bluetooth Low Energy (BLE)

```cpp
BleConnectivityDriver bleDriver("MyDevice");
bleDriver.connect();
```

Uses the Nordic UART Service (NUS) GATT profile. Compatible with all ESP32 variants (ESP32, S3, C3, C6, H2). Supports GATT notifications for transmission and write callbacks for reception.

### MQTT

```cpp
MqttGatewayClient mqttGateway(wifiDriver, wifiClient, "192.168.1.100", 1883, "device/telemetry", "device-001");
device.setTelemetryGateway(&mqttGateway);
```

Provides keep-alive servicing via `service()`, reconnection backoff, and bounded serialization (512-byte buffer). Register with `device.setTelemetryGateway()` so diagnostics can aggregate send statistics.

### Secure Communication

Both `MqttGatewayClient` and `HttpGatewayClient` accept a `Client&` reference, making `WiFiClientSecure` a drop-in replacement for `WiFiClient`. This enables TLS 1.2/1.3 encrypted communication with no framework code changes.

**MQTT over TLS** (port 8883):

```cpp
#include <WiFiClientSecure.h>

WiFiClientSecure secureClient;
secureClient.setInsecure(); // For testing; use setCACert() in production

MqttGatewayClient mqttGateway(wifiDriver, secureClient, "broker.example.com", 8883, "device/telemetry", "device-001");
```

**HTTP over TLS** (HTTPS):

```cpp
WiFiClientSecure secureClient;
secureClient.setInsecure(); // For testing; use setCACert() in production

HttpGatewayClient httpGateway(secureClient, "https://api.example.com/device/telemetry");
```

**Certificate options**:

| Method | Use Case | Security Level |
|--------|----------|----------------|
| `setInsecure()` | Testing and development only | None — skips certificate verification |
| `setCACert(pemCert)` | Production with CA-signed certificates | High — verifies broker/server certificate chain |
| `setCertificate(pemCert)` | Mutual TLS (client authentication) | Highest — both server and client authenticate |

**Memory overhead**: `WiFiClientSecure` uses approximately 4-6KB more RAM than `WiFiClient` due to TLS state and buffer requirements. This is a trade-off for encrypted communication.

**See also**: [`MqttTlsDevice`](examples/MqttTlsDevice/) example for a complete MQTT over TLS implementation.

## Examples

The framework ships with 22 example sketches demonstrating various use cases:

| Example | Description |
|---|---|
| `StatusIndicator` | Minimal status indicator with periodic LED toggle; demonstrates timer-based scheduling and loop-less execution. |
| `BleVitalsTransmitter` | Wearable fitness band sending heart rate and temperature via BLE NUS; demonstrates `BleGatewayClient` and `BleConnectivityDriver`. |
| `RemoteLedController` | MQTT-connected RGB smart lamp controlled by cloud commands; demonstrates `enqueueCommand()`, `RgbLed`, and cloud-to-device CQRS flow. |
| `IndoorAirQualityDisplay` | Desktop BME680 air quality monitor with OLED display; demonstrates `EnvironmentalGasSensor`, `OledDisplay`, and drawing primitives. |
| `PipeTemperatureMonitor` | DS18B20 pipe freeze alarm with RGB LED and buzzer; demonstrates `OneWireTemperatureSensor` and fault detection. |
| `EquipmentHealthMonitor` | Industrial equipment health dashboard; demonstrates `ModestLog`, `Device::diagnostics()`, and `EngineConfiguration`. |
| `SmartBandMqtt` | Wearable device with pulse, DHT temperature/humidity, and button; publishes via MQTT. |
| `AirConditioningDevice` | HVAC monitor with DHT sensor, buttons, and LCD display; demonstrates I²C actuator on sampler task. |
| `AirQualityMonitor` | Gas sensor with MQTT telemetry; demonstrates keep-alive and reconnection. |
| `ClimateDataUploader` | DHT22 sensor with HTTP telemetry, Bearer token acquisition via GET request, response body parsing, and authenticated POST telemetry. |
| `CropMonitoringStation` | Multi-sensor agricultural monitoring with tickless scheduling and MQTT telemetry. |
| `DriverAssistantDevice` | GPS and ultrasonic distance monitoring; demonstrates multi-sensor coordination. |
| `HomeSecurityDevice` | Water leak, door/window, PIR, and buzzer; demonstrates interrupt-driven home security. |
| `OutdoorSecurityLightController` | Light sensor and PIR with fault detection; demonstrates sensor health events and safe-state fallback. |
| `PerimeterMonitor` | Four PIR sensors for zone-based motion detection; demonstrates high-frequency polling. |
| `RfidAccessController` | SPI RFID card reader with access control; demonstrates CRC calculation and card communication. |
| `SmartGreenhouseDevice` | Soil moisture, DHT, relay pump, and servo vent; demonstrates event-driven irrigation control. |
| `SmartScaleDevice` | Four HX711 load cells for precision weighing; demonstrates multi-sensor averaging. |
| `ToggableLedDevice` | Button-controlled LED with MQTT telemetry; demonstrates CQRS command/event separation. |
| `WaterFlowMonitor` | YF-S201 water flow sensor with hybrid interrupt/periodic scheduling. |
| `MqttTlsDevice` | Secure environmental monitor with encrypted MQTT (TLS); demonstrates `WiFiClientSecure` as a drop-in replacement for `WiFiClient`. |
| `WeatherStation` | Battery-powered weather station with deep sleep, DHT22, BH1750, and PthSensor. |

## Testing

The framework includes 31 host unit test suites that run natively without hardware:

| Suite | What It Tests |
|---|---|
| `tests/DigitalLightIntensitySensor/` | I²C sensor polling, Wire mock, event emission |
| `tests/RgbLed/` | PWM output, color mixing, analogWrite mock |
| `tests/RfidSensor/` | CRC calculation, SPI mock, card communication |
| `tests/BleConnectivityDriver/` | BLE NUS GATT service, notifications, RX callbacks |
| `tests/BleGatewayClient/` | BLE NUS telemetry serialization, JSON lines framing, statistics |
| `tests/ModestLog/` | `MODEST_LOG_*` level filtering, sink redirection, `MIOT_LOG_*` aliases |
| `tests/MqttGatewayClient/` | MQTT keep-alive, reconnect backoff, client state |
| `tests/EngineConfig/` | `EngineConfig` defaults, custom values, aggregate init |
| `tests/DiagnosticsSnapshot/` | `DiagnosticsSnapshot`, shared counters, enqueue edge cases |
| `tests/SensorFault/` | Fault threshold, latching, recovery events, counter reset |
| `tests/SamplingScheduler/` | Per-slot periods, phase alignment, minimum enforcement |
| `tests/TelemetryPackage/` | Golden JSON serialization, envelope metadata, bounded buffer overflow |
| `tests/TriggeringScheduler/` | Interrupt-driven sensor registration, trigger table, ISR handler |
| `tests/CommandMarshaling/` | Command queue, cross-task marshaling, enqueue edge cases |
| `tests/WaterFlowSensor/` | Hybrid scheduling, pulse counting, flow rate change detection |
| `tests/SoilMoistureSensor/` | AnalogRead moisture, epsilon-based change detection, event emission |
| `tests/DeepSleep/` | PowerMode enum, wake causes, RTC_DATA_ATTR, deep sleep API mocks |
| `tests/ServoMotor/` | Native LEDC PWM, angle commands, state tracking, parent propagation |
| `tests/CharacterLcdDisplay/` | HD44780 4-bit I2C protocol, cursor positioning, backlight control |
| `tests/RelayActuator/` | Relay on/off/toggle commands, state tracking, parent propagation |
| `tests/PthSensor/` | BME280 I²C sensor, compensation formulas, altitude calculation |
| `tests/WaterLeakSensor/` | Digital input, leak detection, propagation suppression |
| `tests/DoorWindowSensor/` | Digital input, open/close detection, propagation suppression |
| `tests/OneWireTemperatureSensor/` | 1-Wire protocol, DS18B20 temperature, change detection |
| `tests/OledDisplay/` | SSD1306 I²C protocol, frame buffer, text rendering, drawing |
| `tests/EnvironmentalGasSensor/` | BME680 I²C sensor, gas resistance, compensation formulas |
| `tests/HttpGatewayClient/` | HTTP methods, auth headers, response body handling, statistics tracking |
| `tests/Buzzer/` | All commands (TURN_ON, TURN_OFF, PLAY_TONE, PLAY_FREQUENCY) with tone/noTone mock |
| `tests/ConfigurationStore/` | Namespace isolation, type-safe getters/setters, default fallbacks, read-only mode, schema validation |
| `tests/TimeSynchronizationDriver/` | NTP initialization, timestamp fallback, sync status, RTC save/restore |

Run all tests locally:

```sh
./tests/run_all.sh
```

Tests compile with strict warnings (`-Wall -Wextra -Werror`) and Address/Undefined sanitizers. A GitHub Actions workflow runs the full suite on Ubuntu and macOS for every pull request.

## Documentation

Documentation is split between architecture artifacts in `docs/` and code-level API documentation in `src/`.

### Design and Architecture (`docs/`)

| Artifact | Description |
|---|---|
| [`docs/class-diagram.puml`](docs/class-diagram.puml) | PlantUML class diagram of the framework structure. |
| [`docs/user-stories.md`](docs/user-stories.md) | Product-level user stories and acceptance criteria. |
| [`docs/improvements.md`](docs/improvements.md) | Master index, implementation status, and detailed plans for all framework improvements. |

### API and Code-Level Reference (`src/`)

The source files in `src/` include Doxygen comments in both headers and implementation files. To generate browsable API docs locally:

```sh
doxygen docs/Doxyfile
```

Then open `docs/build/html/index.html` in your browser.

## Getting Started

### Prerequisites

- **Software**: Arduino IDE 2.x or `arduino-cli` with ESP32 board support (Arduino-ESP32 core 3.x recommended).
- **Hardware**: ESP32 development board (original ESP32, ESP32-S3, ESP32-C3, ESP32-C6, or ESP32-H2).

### Installation

1. **Download**: Clone or download this repository from [modest-iot/modest-iot-nano-framework](https://github.com/modest-iot/modest-iot-nano-framework).
2. **Install as Library**:
   - Copy the repository to your Arduino libraries directory (e.g., `~/Arduino/libraries/ModestIoT/` on macOS/Linux or `Documents\Arduino\libraries\ModestIoT\` on Windows).
   - Alternatively, in the Arduino IDE: Sketch > Include Library > Add .ZIP Library.
3. **Install Dependencies**: The framework requires the following Arduino libraries (install via Library Manager):
   - `ArduinoJson (>=7.0.0)` (for telemetry serialization)
   - `MQTT (>=2.5.3)` (for `MqttGatewayClient`)
4. **Verify Structure**: The library should be organized as:
   ```plaintext
   Modest-IoT-Nano-Framework/
   # Note: The above directory name uses hyphens as a file system convention.
   # The display name is "Modest IoT Nano Framework" (with spaces).
   ├── README.md
   ├── library.properties
   ├── src/
   │   ├── ModestIoT.h
   │   ├── ModestLog.h
   │   ├── EventHandler.h
   │   ├── CommandHandler.h
   │   ├── Sensor.h / Sensor.cpp
   │   ├── Actuator.h / Actuator.cpp
   │   ├── Device.h / Device.cpp
   │   ├── SamplingScheduler.h / SamplingScheduler.cpp
   │   ├── TelemetryPackage.h / TelemetryPackage.cpp
   │   ├── GatewayClient.h
   │   ├── MqttGatewayClient.h / MqttGatewayClient.cpp
   │   ├── HttpGatewayClient.h / HttpGatewayClient.cpp
   │   ├── WiFiConnectivityDriver.h / WiFiConnectivityDriver.cpp
   │   ├── BleConnectivityDriver.h / BleConnectivityDriver.cpp
   │   └── ... (sensors and actuators)
   ├── examples/
   └── tests/
   ```
5. **Integration**: Include the library from your sketch with `#include <ModestIoT.h>`.

### Usage

```cpp
#include <ModestIoT.h>

class MyDevice : public Device {
public:
    MyDevice() : Device(1000) { /* 1-second tick interval */ }

    void on(Event event) override {
        // React to sensor events
    }

    void processQueuedTelemetryData(const TelemetryPackage* packet) const override {
        // Send telemetry via gateway
    }
};

MyDevice device;

void setup() {
    device.initializeAsynchronousEngine();
}

void loop() {
    // Framework handles everything asynchronously
}
```

## Notes

- The library targets Arduino-compatible ESP32 projects.
- BLE connectivity (`BleConnectivityDriver`) supports all ESP32 variants (ESP32, S3, C3, C6, H2).
- Some example components depend on external Arduino libraries (listed in `library.properties`).
- Doxygen comments in `src/*.h` and `src/*.cpp` provide the canonical code-level reference.

## License

This project is licensed under the **Apache License 2.0**.

- **Allowed**: Use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the framework, with attribution.
- **Full License**: See [`LICENSE.md`](LICENSE.md) for the complete terms.

## Contributing

Contributions, improvements, and extensions are welcome under the Apache License 2.0. You're welcome to:

- Use the framework in your projects.
- Submit feedback, bug reports, or feature suggestions via GitHub.
- Fork and adapt the framework for your own needs (with attribution).

## Future Directions

- **v1.1.0**: Telemetry packet pool (W18), telemetry buffering with LittleFS persistence, power manager with battery monitoring, and firmware version in telemetry.
- **v1.2.0**: Over-the-air (OTA) updates and event aggregation for composite sensor rules.
- **Edge Framework Phase 1**: Python-based edge server with MQTT broker, device registry, and telemetry ingestion.
- **MicroPython Edition**: A planned port to MicroPython in a separate repository.

## Contact

For questions, feedback, or support, contact Angel Velasquez via GitHub:

- **GitHub**: [avelasquezn](https://github.com/avelasquezn)

---
Modest IoT Nano Framework (C++ Edition): A simple, modular foundation for your IoT projects.
