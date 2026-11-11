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
- BmeEnvironmentalSensor: Extracted shared Bosch BME280/BME680 compensation formulas into abstract base class, eliminating ~200 lines of duplicated code between PthSensor and EnvironmentalGasSensor
- Hd44780Driver: Extracted HD44780 I²C protocol from CharacterLcdDisplay into standalone driver class
- Ssd1306Driver: Extracted SSD1306 I²C protocol from OledDisplay into standalone driver class

### Examples (16 total)
- ClassicBlinker, ToggableLedDevice, SmartBandMqtt, AirConditioningDevice
- AirQualityMonitor, DriverAssistantDevice, SmartScaleDevice, PerimeterMonitor
- OutdoorSecurityLightController, RfidAccessController, SmartGreenhouseDevice
- WaterFlowMonitor, HomeSecurityDevice, CropMonitoringStation, WeatherStation
- HttpTelemetryDemo

### Testing
- 27 host test suites covering all sensors, actuators, scheduler, MQTT, BLE, and telemetry
- CI workflows: host tests, ESP32 compilation, linting
- AddressSanitizer and UndefinedBehaviorSanitizer enabled in tests

### Documentation
- 30 Architecture Decision Records (ADRs)
- Doxygen documentation with API navigation tree
- Class diagram (PlantUML) covering all 47 source headers
- User stories and improvement tracking

### Technical Details
- ESP32 Arduino core 3.x compatible
- FreeRTOS-based task scheduling
- C++17 standard
- Zero magic numbers - all constants use full-word suffixes (_IN_HERTZ, _IN_MILLISECONDS, _IN_MICROSECONDS)
