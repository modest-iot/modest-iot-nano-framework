<div style="text-align: center;">

![Modest IoT Nano Framework logo](assets/branding/logo.png)

</div>

# Modest IoT Nano Framework (C++ Edition)

[![Host Unit Tests](https://github.com/modest-iot/modest-iot-nano-framework/actions/workflows/host-tests.yml/badge.svg)](https://github.com/modest-iot/modest-iot-nano-framework/actions/workflows/host-tests.yml)
[![Compile Examples](https://github.com/modest-iot/modest-iot-nano-framework/actions/workflows/compile-examples.yml/badge.svg)](https://github.com/modest-iot/modest-iot-nano-framework/actions/workflows/compile-examples.yml)
[![Lint](https://github.com/modest-iot/modest-iot-nano-framework/actions/workflows/lint.yml/badge.svg)](https://github.com/modest-iot/modest-iot-nano-framework/actions/workflows/lint.yml)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![Platform](https://img.shields.io/badge/Platform-ESP32-orange.svg)](https://espressif.com/en/products/socs/esp32)
[![Version](https://img.shields.io/github/v/tag/modest-iot/modest-iot-nano-framework?sort=semver)](https://github.com/modest-iot/modest-iot-nano-framework/releases)
[![Arduino Library Manager](https://img.shields.io/badge/Arduino-Library%20Manager-blue.svg)](https://librarymanager/all#Modest-IoT-Nano-Framework)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/modest-iot/library/Modest-IoT-Nano-Framework.svg)](https://registry.platformio.org/libraries/modest-iot/Modest-IoT-Nano-Framework)
[![MQTT](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/modest-iot/modest-iot-nano-framework/develop/.badges/mqtt.json)](https://github.com/256dpi/arduino-mqtt)
[![ESP32](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/modest-iot/modest-iot-nano-framework/develop/.badges/esp32.json)](https://github.com/espressif/arduino-esp32)

**Version**: 1.3.0<br>
**Author**: Angel Velasquez  
**Date created**: 2025-03-23  
**Last updated**: 2026-11-27

> **Git Flow**: This project uses Git Flow. `main` holds release versions (tagged `v1.0.0`, etc.), `develop` is the integration branch, and feature branches (e.g., `feature/W27-firmware-version`) merge into `develop`.

## Overview

Modest IoT Nano Framework (C++ Edition) is a lightweight, object-oriented, event-driven, and CQRS-inspired Arduino library for resource-constrained IoT projects on ESP32-based devices. It provides reusable abstractions for events, commands, sensors, actuators, connectivity drivers, gateway clients, and complete devices — all orchestrated by an asynchronous FreeRTOS engine with per-slot sampling intervals, zero-cost logging, and runtime diagnostics.

**Approved by Arduino** — Available in the [Arduino Library Manager](https://github.com/modest-iot/modest-iot-nano-framework) for easy installation directly from the Arduino IDE.

The framework uses a **Mediator pattern** where a `Device` subclass owns and coordinates its sensors and actuators. Sensors emit events that propagate synchronously through the mediator, which issues commands to actuators. Telemetry flows through a queue-based pipeline from the sampler task to the network task, keeping sensor polling and network transmission isolated.

This repository contains the C++ edition of the framework. A MicroPython edition is planned as a separate distribution. The [`examples/`](examples/) folder includes 28 grounded real-world devices; start with [`StatusIndicator`](examples/StatusIndicator/) for the simplest usage demonstration.

## Key Characteristics

- **Object-Oriented**: Built with C++ classes, inheritance, and encapsulation for maintainable code.
- **Event-Driven**: Uses an `Event`/`EventHandler` system for asynchronous, reactive behavior.
- **CQRS-Inspired**: Separates `Command` (control) and `Event` (notification) pathways.
- **Asynchronous Engine**: Two FreeRTOS tasks (sampler and network worker) with configurable stack sizes, priorities, and core pinning via `EngineConfiguration`.
- **Per-Slot Scheduling**: `SamplingScheduler` supports independent polling intervals per sensor, independent of slot count.
- **Zero-Cost Logging**: `MODEST_LOG_*` macros with compile-time level filtering; `MIOT_LOG_*` shorthand aliases available.
- **Runtime Diagnostics**: `DiagnosticsSnapshot` with 16 fields: stack high-water marks, heap stats, queue counters, send statistics, and uptime.
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
| `I2cBus` / `ArduinoI2cBus` | Shared I²C transaction contract and Arduino `Wire` backend with transfer results and configurable timeout. |
| `EngineConfiguration` | Configuration struct for FreeRTOS task stack sizes, priorities, core pinning, and queue depth. |
| `ConfigurationStore` | Persistent key-value configuration using ESP32 NVS with namespace isolation, type-safe access, and schema validation. |
| `TimeSynchronizationDriver` | NTP-based time synchronization with RTC fallback for deep-sleep devices; replaces `millis()` timestamps with Unix epoch seconds. |
| `DiagnosticsSnapshot` | 16-field runtime health snapshot (stack, heap, queue, timing). |
| `SamplingScheduler` | Per-slot countdown scheduler with phase-aligned re-arming and minimum period enforcement. |
| `TelemetryPackage` | Base class for telemetry payloads with envelope metadata (timestamp, sequence). |
| `TelemetryBuffer` | Persistent telemetry buffer using LittleFS with monotonic indices for offline storage and FIFO replay — no file shifting overhead. |
| `PowerManager` | Battery voltage sensing via ADC with configurable voltage divider, level calculation, and low-battery alerts. |
| `OtaUpdateHandler` | Over-The-Air firmware update service with hostname configuration, password protection, and progress callbacks. |
| `EventAggregator` | Composite event pattern matching with AND/OR logic, source-aware matching, conditional patterns, nested aggregators, and dynamic pattern management. |
| `RulesEngine` | On-device automation rules. Maps composite events from EventAggregator to automated actuator Commands. Fixed-size rule array (4 rules, 4 actions per rule) — zero heap allocation. |
| `ChangeFilter` | Template class for deadband-based telemetry suppression. Per-slot tracking with configurable thresholds, supporting float and int sensor values. |
| `AiMicroModelController` | Abstract base class for ML inference on microcontrollers. Uniform interface for classification and regression. |
| `EmlearnController` | Optional emlearn adapter for tree, neural-network, regression, and generated C models; generated-model use does not require the emlearn library. |

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
| `ImuSensor` | MPU6050 6-axis inertial measurement unit (accelerometer + gyroscope) with I²C communication, factory calibration, and windowed statistics (RMS, peak-to-peak). |
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
| **Arduino `setup()`/`loop()`** | `setup()` constructs the device composition; injected I²C devices explicitly start the bus and peripherals before registering sampling and starting the engine. The user `loop()` typically calls `vTaskDelay`. |

For injected I²C peripherals, constructors store configuration and a non-owning bus reference without performing hardware I/O. The concrete `Device` owns one `ArduinoI2cBus` declared before its I²C members, then starts the bus and required peripherals before activating sampling and starting the asynchronous engine. If a required startup fails, the engine remains stopped. Existing Arduino constructor overloads using the legacy `Wire` path remain available for compatibility.

### Threading Rules

1. **Sensor, mediator, and actuator code runs on the sampler task** and must not block for long periods. Slow actuators (e.g., I²C LCD updates) consume the sensor tick budget.
2. **Do not call sensors, actuators, or perform I²C operations from `loop()` or unrelated tasks.** Use the mediator pattern so the sampler task drives runtime hardware interactions; legacy paths using `Wire` follow the same restriction.
3. **Telemetry packets are immutable snapshots** passed by ownership transfer through the queue. The network task owns the packet after enqueue succeeds.
4. **Interrupts only set flags** and signal the sampler task; they do not access hardware directly.
5. **The two workers share no mutable framework state** except the queue, so no locks are needed in the core framework.

### Engine Configuration

The `EngineConfiguration` struct allows tuning stack sizes, priorities, and core pinning:

```cpp
EngineConfiguration config;
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

Call `device.collectDiagnostics()` to get a `DiagnosticsSnapshot` with 16 runtime health fields:

```cpp
DiagnosticsSnapshot health = device.collectDiagnostics();
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

### Firmware Version in Telemetry

Include your firmware version in every telemetry packet for fleet management and diagnostics:

```cpp
Device::setFirmwareVersion("1.0.0");
```

The `firmwareVersion` field appears in the JSON envelope of every telemetry record. Optionally set a schema version to track telemetry format changes:

```cpp
Device::setTelemetrySchemaVersion("1.0");
```

When neither is set (the default), both fields are omitted from the JSON output with zero overhead.

### Battery Monitoring

Monitor battery voltage and level for battery-powered devices:

```cpp
// Configure battery monitoring on ADC pin 34 with a 2:1 voltage divider
PowerManager::begin(34, 2.0f);

// Read battery voltage and level
uint32_t voltage = PowerManager::getBatteryVoltageInMillivolts(); // in millivolts
float level = PowerManager::getBatteryLevelInPercentage();        // 0.0 to 100.0

// Check low battery status
if (PowerManager::isLowBattery()) {
    // Take action: reduce sampling, enter deep sleep, etc.
}
```

Supports configurable voltage divider ratios, custom battery voltage ranges for different chemistries, and adjustable low-battery thresholds. Default range is 3.0V–4.2V for Li-ion batteries.

### Telemetry Buffering

Persist telemetry packets when the network is unavailable and replay them when connectivity is restored:

```cpp
// Initialize the buffer in setup()
TelemetryBuffer::begin();

// Optional: monitor eviction events
TelemetryBuffer::onEviction([]() {
    Serial.println("Warning: telemetry packet evicted due to buffer capacity");
});

// In processQueuedTelemetryData(), if send fails:
if (!gatewayClient.sendTelemetryRecord(*packet)) {
    TelemetryBuffer::store(packet->toJsonString());
}

// When connectivity is restored, replay buffered packets:
String buffered;
while (TelemetryBuffer::retrieveNext(buffered)) {
    // Parse and resend the buffered JSON
}
```

Uses LittleFS for persistence with configurable max buffer size (default 50 packets). Buffered packets survive unexpected reboots and are replayed in FIFO order.

### OTA Firmware Updates

Enable Over-The-Air firmware updates for deployed devices:

```cpp
// Configure OTA updates in setup()
OtaUpdateHandler::begin("my-device-name");
OtaUpdateHandler::setPassword("secret123");

// Optional: track progress
OtaUpdateHandler::onProgress([](uint32_t progressInNumberOfBytesWritten, uint32_t totalNumberOfBytes) {
    float percent = (float)progressInNumberOfBytesWritten / (float)totalNumberOfBytes * 100.0f;
    Serial.printf("OTA Progress: %.1f%%\r", percent);
});

// In your Device subclass, call service() from maintainNetworkServices()
void maintainNetworkServices() const override {
    gatewayClient.service();
    OtaUpdateHandler::service();
}
```

Supports hostname configuration for device discovery, password protection, and progress/error callbacks. Upload firmware directly from the Arduino IDE by selecting the OTA port listed under Tools → Port. The `service()` method is called automatically by the network dispatcher task via `maintainNetworkServices()` — no `loop()` required.

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

Uses the Nordic UART Service (NUS) GATT profile. Compatible with all ESP32 variants (ESP32, S3, C3, C6, H2). Supports GATT notifications for transmission and write callbacks for reception. The BLE hardware abstraction layer supports both ESP32 Arduino core 3.x and 4.x APIs.

### MQTT

```cpp
MqttGatewayClient mqttGateway(wifiDriver, wifiClient, "192.168.1.100", 1883, "device/telemetry", "device-001");
device.setTelemetryGateway(&mqttGateway);
```

Provides keep-alive servicing via `service()`, reconnection backoff, and bounded serialization (512-byte buffer). Register with `device.setTelemetryGateway()` so diagnostics can aggregate send statistics.

### Secure Communication

Both `MqttGatewayClient` and `HttpGatewayClient` accept a `Client&` reference for the network socket, making `WiFiClientSecure` a drop-in replacement for `WiFiClient`. This enables TLS 1.2/1.3 encrypted communication with no framework code changes.

**Recommended: Use the `createSecureSocket()` builder method** for concise TLS setup:

```cpp
// Create and configure a TLS socket in one call
WiFiClientSecure* secureSocket = WiFiConnectivityDriver::createSecureSocket(nullptr, true); // insecure mode for testing
// For production:
// WiFiClientSecure* secureSocket = WiFiConnectivityDriver::createSecureSocket(caCertificate, false);

MqttGatewayClient mqttGateway(wifiDriver, *secureSocket, "broker.example.com", 8883, "device/telemetry", "device-001");
```

**Alternative: Manual `WiFiClientSecure` construction** for advanced use cases:

```cpp
WiFiClientSecure secureClient;
secureClient.setInsecure(); // For testing; use setCACert() in production

MqttGatewayClient mqttGateway(wifiDriver, secureClient, "broker.example.com", 8883, "device/telemetry", "device-001");
```

**HTTP over TLS** (HTTPS) follows the same pattern:

```cpp
WiFiClientSecure* secureSocket = WiFiConnectivityDriver::createSecureSocket(nullptr, true);
HttpGatewayClient httpGateway(wifiDriver, "https://api.example.com/device/telemetry");
```

**Certificate options**:

| Method | Use Case | Security Level |
|--------|----------|----------------|
| `setInsecure()` | Testing and development only | None — skips certificate verification |
| `setCACert(pemCert)` | Production with CA-signed certificates | High — verifies broker/server certificate chain |
| `setCertificate(pemCert)` | Mutual TLS (client authentication) | Highest — both server and client authenticate |

**Memory overhead**: `WiFiClientSecure` uses approximately 4-6KB more RAM than `WiFiClient` due to TLS state and buffer requirements. This is a trade-off for encrypted communication.

**See also**: [`MqttTlsDevice`](examples/MqttTlsDevice/) example for a complete MQTT over TLS implementation using `createSecureSocket()`.

## Examples

The framework ships with 28 example sketches demonstrating various use cases:

| Example | Description |
|---|---|
| `StatusIndicator` | Minimal status indicator with periodic LED toggle; demonstrates timer-based scheduling and loop-less execution. |
| `BleVitalsTransmitter` | Wearable fitness band sending heart rate and temperature via BLE NUS; demonstrates `BleGatewayClient` and `BleConnectivityDriver`. |
| `RemoteLedController` | MQTT-connected RGB smart lamp controlled by cloud commands; demonstrates `enqueueCommand()`, `RgbLed`, and cloud-to-device CQRS flow. |
| `IndoorAirQualityDisplay` | Desktop BME680 air quality monitor with OLED display; demonstrates `EnvironmentalGasSensor`, `OledDisplay`, and drawing primitives. |
| `PipeTemperatureMonitor` | DS18B20 pipe freeze alarm with RGB LED and buzzer; demonstrates `OneWireTemperatureSensor` and fault detection. |
| `EquipmentHealthMonitor` | Industrial equipment health dashboard; demonstrates `ModestLog`, `Device::collectDiagnostics()`, and `EngineConfiguration`. |
| `SmartBandMqtt` | Wearable device with pulse, DHT temperature/humidity, and button; publishes via MQTT. |
| `AirConditioningDevice` | HVAC monitor with DHT sensor, buttons, and LCD display; demonstrates I²C actuator on sampler task. |
| `AirQualityMonitor` | Gas sensor with MQTT telemetry; demonstrates keep-alive and reconnection. |
| `ClimateDataUploader` | DHT22 sensor with HTTP telemetry, Bearer token acquisition, and offline buffering; demonstrates `TelemetryBuffer` for LittleFS persistence when the network is unavailable, with FIFO eviction and replay on reconnection, and `BearerTokenParser` utility class for isolated token extraction. |
| `CropMonitoringStation` | Multi-sensor agricultural monitoring with tickless scheduling and MQTT telemetry. |
| `DriverAssistantDevice` | GPS and ultrasonic distance monitoring; demonstrates multi-sensor coordination. |
| `HomeSecurityDevice` | Water leak, door/window, PIR, and buzzer; demonstrates interrupt-driven home security. |
| `OutdoorSecurityLightController` | Light sensor and PIR with fault detection; demonstrates sensor health events and safe-state fallback. |
| `PerimeterMonitor` | Four PIR sensors for zone-based motion detection; demonstrates high-frequency polling. |
| `RfidAccessController` | SPI RFID card reader with access control; demonstrates CRC calculation and card communication. |
| `SmartGreenhouseDevice` | Soil moisture, DHT, relay pump, and servo vent; demonstrates event-driven irrigation control. |
| `SmartScaleDevice` | Four HX711 load cells for precision weighing; demonstrates multi-sensor averaging. |
| `TogglableLedDevice` | Button-controlled LED with MQTT telemetry; demonstrates CQRS command/event separation. |
| `WaterFlowMonitor` | YF-S201 water flow sensor with hybrid interrupt/periodic scheduling. |
| `MqttTlsDevice` | Secure environmental monitor with encrypted MQTT (TLS); demonstrates `WiFiConnectivityDriver::createSecureSocket()` builder method for TLS socket creation. |
| `WeatherStation` | Battery-powered weather station with deep sleep, DHT22, BH1750, and PthSensor; demonstrates `PowerManager` for battery voltage sensing, adaptive power mode switching (Periodic → DeepSleep on low battery), and RTC-persisted low-battery state. |
| `OtaUpdateDevice` | Minimal device with DHT22 and OTA firmware updates; demonstrates `OtaUpdateHandler` with hostname configuration, password protection, progress/error/start/end callbacks, and `service()` in `maintainNetworkServices()`. |
| `IntrusionDetector` | Motion, door, and water leak sensors with composite event patterns; demonstrates `EventAggregator` standard AND patterns for home security. |
| `ColdChainMonitor` | Refrigerated truck cargo bay monitoring; demonstrates `EventAggregator` conditional patterns with `PatternCondition` callbacks for IIoT cold chain logistics. |
| `PharmaceuticalCleanRoom` | Clean room environmental monitoring; demonstrates `EventAggregator` nested 3-level aggregator chains, OR patterns, and `DEVICE_SOURCE_ID` for IIoT pharmaceutical manufacturing. |
| `SmartFactoryAlarm` | Factory alarm system with automated rules; demonstrates `EventAggregator` AND pattern + `RulesEngine` for automated buzzer alert on water leak + door intrusion. |
| `VibrationMonitor` | Industrial vibration monitoring with anomaly detection; demonstrates `ImuSensor` with windowed statistics, `EmlearnController` for on-device ML inference, and `Buzzer` actuator for audible alerts. |

## Testing

The framework includes 54 host unit test suites that run natively without hardware:

| Suite | What It Tests |
|---|---|
| `tests/DigitalLightIntensitySensor/` | I²C sensor polling, injected and legacy bus paths, event emission |
| `tests/I2cBus/` | Bus startup/configuration, transaction results, combined reads, timeout and serialization behavior |
| `tests/RgbLed/` | PWM output, color mixing, analogWrite mock |
| `tests/RfidSensor/` | CRC calculation, SPI mock, card communication |
| `tests/BleConnectivityDriver/` | BLE NUS GATT service, notifications, RX callbacks |
| `tests/BleGatewayClient/` | BLE NUS telemetry serialization, JSON lines framing, statistics |
| `tests/ModestLog/` | `MODEST_LOG_*` level filtering, sink redirection, `MIOT_LOG_*` aliases |
| `tests/MqttGatewayClient/` | MQTT keep-alive, reconnect backoff, client state |
| `tests/EngineConfiguration/` | `EngineConfiguration` defaults, custom values, aggregate init |
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
| `tests/GasSensor/` | Analog gas detection, calibration modes, Rs/R0 ratio, PPM estimation, gas types |
| `tests/Button/` | Debounce logic, press detection, timing, event emission |
| `tests/DhtSensor/` | DHT single-wire protocol, temperature/humidity, CRC validation |
| `tests/PowerManager/` | Battery voltage sensing, level calculation, low-battery threshold, custom ranges |
| `tests/TelemetryBuffer/` | LittleFS persistence, FIFO store/retrieve, eviction callback, monotonic indices |
| `tests/OtaUpdateHandler/` | OTA status tracking, all callback type aliases, hostname, re-initialization safety |
| `tests/Led/` | Digital LED on/off/toggle commands, state tracking, parent propagation |
| `tests/PassiveInfraredSensor/` | PIR motion detection, motion started/stopped events, propagation suppression |
| `tests/PulseSensor/` | Heart rate detection from analog signal, moving average filter, threshold events |
| `tests/Hd44780Driver/` | HD44780 4-bit I²C protocol, initialization sequence, command/data writes |
| `tests/Ssd1306Driver/` | SSD1306 I²C protocol, display initialization, frame buffer commands |
| `tests/WiFiConnectivityDriver/` | Wi-Fi STA connection, reconnection retry, status enumeration, signal strength |
| `tests/BmeEnvironmentalSensor/` | Bosch BME280/BME680 compensation formulas, calibration data parsing, shared base class |
| `tests/UltrasonicSensor/` | HC-SR04 distance measurement, trigger/echo timing, change detection |
| `tests/LoadCellAmplifier/` | HX711 load cell amplifier, weight calculation, tare, power down/up cycle |
| `tests/GeolocationSensor/` | GPS NMEA parsing, GGA/RMC sentences, coordinate conversion, location change detection |
| `tests/Device/` | Device base class, engine initialization, scheduler integration, telemetry pipeline |
| `tests/EventAggregator/` | Composite event patterns, AND/OR logic, source-aware matching, condition functions, nested aggregators, dynamic pattern management |
| `tests/RulesEngine/` | Rules engine, rule creation, multiple actions, rule removal, cleanup, full capacity, payload, unknown event |
| `tests/JsonParser/` | JSON parsing utilities, token extraction, Bearer token parsing |
| `tests/ChangeFilter/` | Deadband-based telemetry suppression, first reading passthrough, multi-slot independence, float/int types, NaN/Inf protection |
| `tests/AiMicroModelController/` | ML inference interface, tree/net prediction, regression, model switching, invalid features, NaN/Inf handling |
| `tests/ImuSensor/` | MPU6050 I²C IMU sensor, compensation formulas, sample buffer statistics, event emission, calibration data |

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
| [`docs/quick-start.md`](https://github.com/modest-iot/modest-iot-nano-framework/blob/release/v1.3.0/docs/quick-start.md) | Step-by-step guide to go from zero to a working device in under 10 minutes. |
| [`docs/troubleshooting.md`](https://github.com/modest-iot/modest-iot-nano-framework/blob/release/v1.3.0/docs/troubleshooting.md) | Common issues and solutions: serial output, board detection, Wi-Fi, BLE, MQTT, memory, and sensors. |
| [`docs/migration.md`](https://github.com/modest-iot/modest-iot-nano-framework/blob/release/v1.3.0/docs/migration.md) | How to convert traditional `loop()` sketches to the framework's event-driven pattern with before/after examples. |
| [`docs/adrs.md`](docs/adrs.md) | 34 Architecture Decision Records with rationale, trade-offs, and consequences. |
| [`docs/user-stories.md`](docs/user-stories.md) | 53 user stories covering functional and non-functional requirements. |
| [`docs/improvements.md`](docs/improvements.md) | Active work items, comparison tables, and future work. |
| [`docs/evolution.md`](https://github.com/modest-iot/modest-iot-nano-framework/blob/release/v1.3.0/docs/evolution.md) | Historical record of completed work items through W47 with implementation details. |
| [`docs/class-diagram.puml`](docs/class-diagram.puml) | PlantUML class diagram of framework classes and their relationships. |
| [`docs/doxygen-mainpage.md`](docs/doxygen-mainpage.md) | Doxygen API reference main page with module navigation and generation instructions. |
| [`docs/benchmarks.md`](https://github.com/modest-iot/modest-iot-nano-framework/blob/release/v1.3.0/docs/benchmarks.md) | Performance comparison table with 9 metrics: Flash, RAM, latency, throughput, queue depth, stack, heap, serialization, and power. |
| [`docs/coverage.md`](https://github.com/modest-iot/modest-iot-nano-framework/blob/release/v1.3.0/docs/coverage.md) | Code coverage guide: local execution with gcov/lcov, CI integration, threshold configuration, and report interpretation. |
| [`docs/dependencies.md`](https://github.com/modest-iot/modest-iot-nano-framework/blob/release/v1.3.0/docs/dependencies.md) | External dependencies list with update processes: Dependabot for GitHub Actions, weekly status badges for MQTT library and ESP32 core. |
| [`docs/release.md`](https://github.com/modest-iot/modest-iot-nano-framework/blob/release/v1.3.0/docs/release.md) | Complete release process: Git Flow Helper Plugin steps, automated release workflow, version bump, registry publishing, and troubleshooting. |

### Project Standards

| Artifact | Description |
|---|---|
| [`ACCESSIBILITY.md`](https://github.com/modest-iot/modest-iot-nano-framework/blob/release/v1.3.0/ACCESSIBILITY.md) | Accessibility statement, reporting guidance, and known limitations. |
| [`CODE_OF_CONDUCT.md`](https://github.com/modest-iot/modest-iot-nano-framework/blob/release/v1.3.0/CODE_OF_CONDUCT.md) | Contributor Covenant Code of Conduct. |
| [`CONTRIBUTING.md`](https://github.com/modest-iot/modest-iot-nano-framework/blob/release/v1.3.0/CONTRIBUTING.md) | Contribution guidelines, Git Flow workflow, and coding standards. |
| [`SECURITY.md`](https://github.com/modest-iot/modest-iot-nano-framework/blob/release/v1.3.0/SECURITY.md) | Security policy and vulnerability reporting process. |

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

**Option 1: Arduino Library Manager (Recommended)**

1. Open the Arduino IDE.
2. Go to **Tools > Manage Libraries...** (or press `Ctrl+Shift+I` / `Cmd+Shift+I`).
3. Search for **Modest IoT Nano Framework**.
4. Click **Install** to install the latest version.
5. The required dependency (`MQTT`) will be installed automatically.

**Option 2: Manual Installation**

1. **Download**: Clone or download this repository from [modest-iot/modest-iot-nano-framework](https://github.com/modest-iot/modest-iot-nano-framework).
2. **Install as Library**:
   - Copy the repository to your Arduino libraries directory (e.g., `~/Arduino/libraries/ModestIoT/` on macOS/Linux or `Documents\Arduino\libraries\ModestIoT\` on Windows).
   - Alternatively, in the Arduino IDE: Sketch > Include Library > Add .ZIP Library.
3. **Install Dependencies**: The framework requires the following Arduino library (install via Library Manager):
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

Every device in the Modest-IoT Nano-Framework extends the `Device` base class, which manages the asynchronous engine, sensor scheduling, and event dispatch. The framework is loop-less: all execution is event-driven, with sensor readings dispatched as `Event` objects to your device's `on()` handler.

The example below demonstrates a minimal status indicator that toggles an LED at a fixed interval using a hardware timer event:

```cpp
#include <ModestIoT.h>

class StatusIndicator : public Device {
    private:
    static constexpr int STATUS_LED_PIN = 2;
    static constexpr unsigned long BLINK_INTERVAL_IN_MILLISECONDS = 500;
    static constexpr int TIMER_EVENT_IDENTIFIER = 300;

    Led statusLed;

    StatusIndicator()
    : Device(BLINK_INTERVAL_IN_MILLISECONDS),
    statusLed(STATUS_LED_PIN, false, this)
    {
        initializeAsynchronousEngine(5);
        appendSensorToScheduler(this, TIMER_EVENT_IDENTIFIER,
                                 BLINK_INTERVAL_IN_MILLISECONDS, 0);
    }

    void on(Event event) override {
        if (event.identifier == TIMER_EVENT_IDENTIFIER) {
            statusLed.handle(Led::TOGGLE_LED_COMMAND);
        }
    }

    void start() {
        Device::setFirmwareVersion("STI-909 FW 1.0.0");
    }

    ~StatusIndicator() override {
        stopAsynchronousEngine();
    }
};

StatusIndicator* indicator;

void setup() {
    Serial.begin(115200);
    indicator = new StatusIndicator();
    indicator->start();
}

void loop() {
    // Framework handles everything asynchronously
    vTaskDelay(portTICK_PERIOD_MS);
}
```

For complete, grounded examples with connectivity, telemetry, and cloud integration, browse the [example sketches](https://github.com/modest-iot/modest-iot-nano-framework/tree/release/v1.3.0/examples) (28 sketches covering MQTT, HTTP, BLE, TLS, deep sleep, battery monitoring, offline buffering, OTA updates, vibration monitoring, and composite-event automation).

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

## Release Highlights and Future Directions

- **v1.3.0**: Adds on-device ML inference, MPU6050 vibration sensing, change-only telemetry, and composite-event automation; BLE supports ESP32 Arduino core 3.x and 4.x.
- **Benchmark Publication**: See [docs/benchmarks.md](https://github.com/modest-iot/modest-iot-nano-framework/blob/release/v1.3.0/docs/benchmarks.md) for the performance comparison table.
- **Edge Framework Phase 1**: Python-based edge server with MQTT broker, device registry, and telemetry ingestion.
- **Edge Framework Phase 2.5**: Matter Bridge to expose Nano devices via Matter ecosystem.
- **MicroPython Edition**: A planned port to MicroPython in a separate repository.

## Contact

For questions, feedback, or support, contact Angel Velasquez via GitHub:

- **GitHub**: [avelasquezn](https://github.com/avelasquezn)

---
Modest IoT Nano Framework (C++ Edition): A simple, modular foundation for your IoT projects.
