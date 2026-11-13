# Changelog

All notable changes to the Modest IoT Nano Framework will be documented in this file.

## [1.0.0] - 2026-11-10

### Added

#### Core Framework
- Event-driven asynchronous IoT framework for ESP32
- Hardware timer-driven sampling scheduler with round-robin dispatch
- Two-task architecture: HardwareWorker (sampler) and NetworkWorker (network)
- Thread-safe communication via FreeRTOS queues and notification bits
- DiagnosticsSnapshot for runtime monitoring (heap, stack, uptime)
- **ConfigurationStore**: Persistent key-value configuration using ESP32 NVS (`Preferences`) with namespace-based isolation, type-safe getters/setters (string, int, unsigned long, float, bool), default fallbacks, read-only mode, and schema validation (containsKey, removeKey, clearNamespace)

#### Sensors (18 total)
- **DhtSensor**: Native DHT11/DHT22 single-wire protocol implementation
- **UltrasonicSensor**: HC-SR04 with interrupt-driven echo capture (no pulseIn blocking)
- **LoadCellAmplifier**: HX711 with interrupt-driven data-ready detection
- **PulseSensor**: Heart rate monitoring with moving average filter
- **GasSensor**: MQ-series gas sensors with temperature compensation
- **DigitalLightIntensitySensor**: BH1750 I²C light sensor
- **PassiveInfraredSensor**: PIR motion detection with interrupt-driven operation
- **Button**: Interrupt-driven button with debouncing
- **RfidSensor**: MFRC522 RFID reader with SPI interface
- **GeolocationSensor**: GPS/NMEA parsing
- **PthSensor**: BME280 I²C sensor (temperature, humidity, pressure, altitude)
- **SoilMoistureSensor**: Analog soil moisture with epsilon-based change detection
- **WaterFlowSensor**: YF-S201 hybrid sensor (interrupt + periodic)
- **WaterLeakSensor**: YL-83/FR-101 water leak detection
- **DoorWindowSensor**: A313/MY-103LY magnetic reed switch detection
- **OneWireTemperatureSensor**: DS18B20 1-Wire digital temperature sensor
- **EnvironmentalGasSensor**: BME680 I²C sensor (temperature, humidity, pressure, gas resistance)
- **BmeEnvironmentalSensor**: Abstract base class for Bosch BME280/BME680 compensation formulas

#### Actuators (7 total)
- **Led**: Digital output with on/off/toggle commands
- **RgbLed**: RGB LED with color mixing
- **ServoMotor**: Native LEDC PWM servo control (no ESP32Servo dependency)
- **Buzzer**: Tone generation with frequency/duration buffering
- **CharacterLcdDisplay**: HD44780 I²C LCD via native Hd44780Driver
- **RelayActuator**: Digital relay control with on/off/toggle commands
- **OledDisplay**: SSD1306 OLED display (128x64) via native Ssd1306Driver

#### Connectivity
- **MqttGatewayClient**: MQTT client using MQTT Things (256dpi/arduino-mqtt), replaced deprecated PubSubClient
- **HttpGatewayClient**: HTTP telemetry client with authentication, statistics, enum-based HTTP methods (`HttpMethod::Get/Post/Put/Patch/Delete`) and headers (`HTTP_HEADER_CONTENT_TYPE`, `HTTP_HEADER_AUTHORIZATION`), fixed-size buffer, response status code reporting, `sendRequest()` for general HTTP requests, `getResponseBody()` for response body capture, and `setServiceEndpointUrl()` for dynamic endpoint switching
- **GatewayClient**: Added `setServiceEndpointUrl()` method to allow switching the target endpoint URL at runtime
- **BleConnectivityDriver**: BLE NUS connectivity driver with disconnect support; RSSI reporting returns 0 (ESP32 BLE server API does not expose RSSI)
- **BleGatewayClient**: BLE NUS gateway client for serialized telemetry payloads. JSON lines framing, fixed-size 512-byte buffer, statistics tracking, and service/loop for advertising restart.
- **WiFiConnectivityDriver**: Wi-Fi connection management with status enumeration, disconnect, IP address reporting, signal strength (RSSI), and connection timeout
- **Ssd1306Driver**: Native SSD1306 I²C protocol driver for OLED displays
- **Hd44780Driver**: Native HD44780 I²C protocol driver for LCD displays

#### Power Management
- **Tickless Scheduling**: One-shot timer with `ticksUntilNextDue()` for optimal sleep
- **Light Sleep**: ESP32 light sleep integration via `enterLightSleep()`
- **Deep Sleep**: ESP32 deep sleep with RTC memory survival and wake-reason reporting

#### Command System
- Cloud-to-device MQTT subscribe with JSON command parsing
- Thread-safe command queue for cross-task command delivery
- CommandHandler pattern for actuator command propagation
- **Command payload support** (W23): Added optional `int32_t payload` to `Command` value object for parameterized commands (e.g., angle, brightness, frequency)
- **ServoMotor**: New `DRIVE_TO_ANGLE_COMMAND` (153) drives servo to payload angle directly; `validateOrAdjustPhysicalAngle()` static method centralizes angle clamping (0–180°) used by `executeCommand`, `setTargetAngleBufferInDegrees`, and `writeAngle`
- **RgbLed**: New `ADJUST_BRIGHTNESS_COMMAND` (304) scales active color by payload/255; `adjustColorSetIntensity()` public method scales all channels via `adjustColorChannelIntensity()` static helper
- **Buzzer**: New `PLAY_FREQUENCY_COMMAND` (253) assigns payload as buffered frequency, then plays using buffered duration via `playBufferedTone()` — consistent with the assign-then-play pattern

### Changed

#### Dependency Updates
- **Removed**: PubSubClient (deprecated) → replaced with MQTT Things (>=2.5.3)
- **Removed**: ESP32Servo → native LEDC peripheral in ServoMotor
- **Removed**: LiquidCrystal_I2C → native HD44780 I²C protocol via Hd44780Driver
- **Removed**: DHT sensor library → native single-wire protocol in DhtSensor
- **Removed**: Adafruit Unified Sensor (unused)
- **Retained**: ArduinoJson (>=7.0.0) - no viable embedded alternative

#### Performance Improvements
- LoadCellAmplifier: Eliminated 100ms busy-wait via interrupt-driven data-ready (W14)
- UltrasonicSensor: Eliminated 30ms pulseIn() blocking via interrupt-driven echo (W14)
- Button: Zero-latency press detection via GPIO interrupt (W13)
- PassiveInfraredSensor: Zero-latency motion detection via GPIO interrupt (W13)
- WaterFlowSensor: Hybrid interrupt + periodic scheduling for YF-S201 pulse counting (W13)

#### Refactoring
- BmeEnvironmentalSensor: Extracted shared Bosch BME280/BME680 compensation formulas into abstract base class, eliminating ~200 lines of duplicated code between PthSensor and EnvironmentalGasSensor; moved constructor to protected to enforce abstract class semantics at compile time
- Hd44780Driver: Extracted HD44780 I²C protocol from CharacterLcdDisplay into standalone driver class
- Ssd1306Driver: Extracted SSD1306 I²C protocol from OledDisplay into standalone driver class

#### Examples
- **MqttTlsDevice**: MQTT over TLS example using `WiFiClientSecure` with `MqttGatewayClient`; demonstrates encrypted telemetry with certificate options (`setInsecure()`, `setCACert()`) and memory overhead guidance
- Replaced banner-style comments (`// ---`) with Doxygen `@name` groups across all 22 examples for consistency
- Eliminated magic numbers in all examples: added `SERIAL_BAUD_RATE` (115200) and `MAIN_LOOP_DELAY_IN_MILLISECONDS` (60000) named constants
- Replaced `#define` pin assignments with `static const` variables where appropriate (HomeSecurityDevice, RfidAccessController)
- Added named constants for sampling intervals, queue depths, debounce times, and thresholds (AirConditioningDevice, AirQualityMonitor, HomeSecurityDevice)
- Moved `#define` constants to proper locations with Doxygen comments (ClimateDataUploader)
- RemoteLedController: Updated MQTT callback to parse JSON payloads `{"command": id, "payload": value}` and demonstrate `ADJUST_BRIGHTNESS_COMMAND`

#### Documentation
- **ConfigurationStore** documented in CHANGELOG.md Core Framework section with full API description
- **Secure Communication** section in README.md: documents MQTT over TLS (port 8883) and HTTP over TLS with `WiFiClientSecure`, certificate options table, and memory overhead guidance
- Comprehensive documentation audit aligning all docs (README, vision, improvements, CHANGELOG) with actual codebase capabilities
- Added BmeEnvironmentalSensor documentation across all documentation files
- Added Gateway Client Enhancements section documenting BleGatewayClient, HttpGatewayClient, WiFiConnectivityDriver, and BleConnectivityDriver
- Corrected test suite count from 26 to 27, then to 28, and finally to 29 host test suites
- Updated Tier 4 status from "Pending" to "Complete" in improvements tracking
- Removed DHT sensor library from README dependencies (replaced with native implementation)
- Fixed source header count from 47 to 46
- Added Pre-Release Readiness Assessment (W20–W29) to vision.md and improvements.md with blocking and deferred items
- Updated improvements.md comparison table reflecting current capabilities and planned features
- Complete rewrite of keywords.txt (276 → 419 lines) with all 46 source headers, drivers, methods, and nested types
- Enhanced Doxygen documentation: mainpage with key features and architecture overview, 5 new subgroup sections, and wildcard build directory exclusion
- Example enhancement plan: renamed ClassicBlinker → StatusIndicator, HttpTelemetryDemo → ClimateDataUploader; added 5 new examples (BleVitalsTransmitter, RemoteLedController, IndoorAirQualityDisplay, PipeTemperatureMonitor, EquipmentHealthMonitor) with grounded real-world purposes and framework aspects illustrated sections
- Fixed class diagram: marked BmeEnvironmentalSensor as abstract; moved Ssd1306Driver and OledDisplay from Sensors to Actuators package
- Fixed ADR count in adrs.md overview from 29 to 30
- Added new command identifiers to keywords.txt: `DRIVE_TO_ANGLE_COMMAND_IDENTIFIER`, `ADJUST_BRIGHTNESS_COMMAND_IDENTIFIER`, `PLAY_FREQUENCY_COMMAND_IDENTIFIER`
- Updated README.md: Command description now mentions `int32_t` payload, Identifier Registry expanded with payload-based commands, test suites table added `HttpGatewayClient` and `Buzzer`, Future Directions marked W23 complete
- Updated vision.md: Pre-Release tier status shows W20, W22, W23 complete; source header count updated to 47, test suite count updated to 30
- Updated improvements.md: W20 marked complete with ConfigurationStore details, comparison table updated, test counts updated to 30
- Updated keywords.txt: Added ConfigurationStore class and all public methods
- Updated README.md: Added ConfigurationStore to Components section, updated Future Directions, updated last updated date
- Updated README.md: Added Secure Communication section with MQTT/HTTP TLS examples, certificate options table, and MqttTlsDevice reference; example count updated to 22; Future Directions marked W22 complete

### Examples (22 total)
- **MqttTlsDevice**: Secure environmental monitor with encrypted MQTT (TLS) using `WiFiClientSecure`
- **SmartGreenhouseDevice**: Updated servo control from two-step `setTargetAngleBufferInDegrees` + `DRIVE_TO_BUFFERED_ANGLE_COMMAND` to single `DRIVE_TO_ANGLE_COMMAND` with payload angle
- **HomeSecurityDevice**: Added missing `#include <Arduino.h>` for consistency with other examples
- StatusIndicator (renamed from ClassicBlinker), ToggableLedDevice, SmartBandMqtt, AirConditioningDevice
- AirQualityMonitor, DriverAssistantDevice, SmartScaleDevice, PerimeterMonitor
- OutdoorSecurityLightController, RfidAccessController, SmartGreenhouseDevice
- WaterFlowMonitor, HomeSecurityDevice, CropMonitoringStation, WeatherStation
- ClimateDataUploader (renamed from HttpTelemetryDemo)
- **BleVitalsTransmitter**: BLE wearable fitness band with heart rate and temperature via BLE NUS
- **RemoteLedController**: MQTT-controlled RGB lamp with cloud-to-device commands via `enqueueCommand()`
- **IndoorAirQualityDisplay**: BME680 air quality monitor with OLED display and VOC bar graph
- **PipeTemperatureMonitor**: DS18B20 pipe freeze alarm with RGB LED and buzzer
- **EquipmentHealthMonitor**: Industrial equipment health dashboard with `ModestLog` and `diagnostics()`

### Testing
- 31 host test suites covering all sensors, actuators, scheduler, MQTT, BLE, HTTP, and telemetry
- **TimeSynchronizationDriver test**: 8 tests covering NTP initialization, timestamp fallback, sync status, RTC save/restore, and full sync cycle simulation
- **WeatherStation example**: Updated with NTP initialization and RTC time restore for deep-sleep time persistence
- **HttpGatewayClient test**: HTTP methods (GET/POST/PUT/DELETE), auth headers, response body handling, statistics tracking
- **Fake HTTPClient.h**: Mock HTTP client for host testing with configurable responses
- **Command payload tests** (W23): 6 new tests in CommandMarshaling suite for payload construction, equality, CommandEntry carry-through, and negative values
- **ServoMotor payload tests**: 6 tests for `DRIVE_TO_ANGLE_COMMAND` with payload angle and `validateOrAdjustPhysicalAngle()` static method
- **RgbLed payload tests**: 5 tests for `ADJUST_BRIGHTNESS_COMMAND` and `adjustColorSetIntensity()` direct method call
- **Buzzer test**: 10 new tests covering all commands (TURN_ON, TURN_OFF, PLAY_TONE, PLAY_FREQUENCY) with tone/noTone mock
- **ConfigurationStore test**: 66 tests covering namespace isolation, type-safe getters/setters, default fallbacks, read-only mode, schema validation, and error handling
- CI workflows: host tests, ESP32 compilation, linting
- AddressSanitizer and UndefinedBehaviorSanitizer enabled in tests

### Documentation
- 30 Architecture Decision Records (ADRs)
- Doxygen documentation with API navigation tree, key features section, and `@runs_on` threading tags
- Class diagram (PlantUML) covering all 48 source headers
- User stories and improvement tracking with pre-release readiness assessment (W20–W29)
- `keywords.txt` comprehensive rewrite: 461 lines with all 48 source headers, drivers, methods, and nested types

### Technical Details
- ESP32 Arduino core 3.x compatible
- FreeRTOS-based task scheduling
- C++17 standard
- Zero magic numbers in framework source - all constants use full-word suffixes (_IN_HERTZ, _IN_MILLISECONDS, _IN_MICROSECONDS)
- Examples follow the same convention: all serial baud rates, loop delays, and timing values are named constants
