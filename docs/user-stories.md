# User Stories for Modest IoT Nano Framework

This document contains user stories for the Modest IoT Nano Framework, designed for IoT developers creating applications on resource-constrained devices like the ESP32. Each story includes acceptance criteria, using the **Given-When-Then** format where applicable, covering the main functional and non-functional requirements that drove the design and implementation of the framework.

## IoT Developer Stories

### TS01: Handle Events for Sensors
- **As an** IoT Developer, **I want** a framework that defines and propagates sensor-originated events, **so that** I can build reactive IoT behavior without wiring ad-hoc polling logic.
- **Acceptance Criteria**:
    - **Given** a sensor input changes, **when** the framework captures that change, **then** an event is emitted and routed to a configured consumer.
    - **Given** no consumer is configured, **when** an event is emitted, **then** the framework remains stable and does not crash.
    - Event metadata is lightweight and deterministic enough for constrained devices.
    - **Given** a sensor faults after N consecutive read failures, **when** the threshold is reached, **then** a fault event (ID 22) is emitted exactly once.
    - **Given** a faulty sensor recovers with a successful read, **when** the next valid reading occurs, **then** a recovery event (ID 23) is emitted exactly once.
- **Status**: ✓ Implemented. See `Sensor.h` (fault identifiers 22/23), `Sensor.cpp` (debounced threshold state machine), and `tests/SensorFault/test.cpp`.
  Related: [ADR-009](adrs.md#adr-009-sensor-fault-events-with-debounced-threshold-state-machine), [ADR-003](adrs.md#adr-003-hybrid-approach-for-sensor-sampling-intervals)

### TS02: Issue Commands to Actuators
- **As an** IoT Developer, **I want** a framework that defines and executes commands for outputs, **so that** I can control actuators in a structured, reusable way.
- **Acceptance Criteria**:
    - **Given** a valid command is issued, **when** the framework dispatches it, **then** the target output state changes consistently.
    - **Given** a command has an upstream propagation target, **when** it is processed, **then** it can be forwarded after local execution.
    - Command handling is deterministic and safe under repeated invocations.
    - **Given** an actuator receives a command, **when** it executes, **then** it runs exclusively on the sampler task (documented via `@runs_on sampler`).
- **Status**: ✓ Implemented. See `Actuator.h`, `Buzzer.h`, `Led.h`, `RgbLed.h`, `ServoMotor.h`, `CharacterLcdDisplay.h`, and `Command.h`.
  Related: [ADR-015](adrs.md#adr-015-identifier-registry-with-reserved-ranges), [ADR-029](adrs.md#adr-029-native-servo-and-lcd-drivers-w19)

### TS03: Combine Inputs and Outputs in Devices
- **As an** IoT Developer, **I want** a unified device abstraction that combines event and command flows, **so that** I can build cohesive IoT units with minimal boilerplate.
- **Acceptance Criteria**:
    - **Given** a custom device implementation, **when** input events are received, **then** the device can map them into output commands.
    - **Given** a single device instance owns multiple inputs and outputs, **when** it runs, **then** it acts as the composition root for orchestration.
    - The abstraction supports scaling from simple one-input/one-output cases to multi-component devices.
    - **Given** a device with registered sensors, **when** the engine starts, **then** sensors are scheduled via `SamplingScheduler` with per-slot intervals.
    - **Given** a device needs to shut down, **when** `stopAsynchronousEngine()` is called, **then** tasks terminate cooperatively and resources are cleaned up.
- **Status**: ✓ Implemented. See `Device.h`, `Device.cpp`, `SamplingScheduler.h`, and `EngineConfiguration`.
  Related: [ADR-010](adrs.md#adr-010-engineconfig-for-tunable-freertos-task-parameters), [ADR-011](adrs.md#adr-011-per-slot-samplingscheduler-with-countdown-based-dispatch), [ADR-014](adrs.md#adr-014-threading-model--single-context-sampler-with-isolated-network-task), [ADR-020](adrs.md#adr-020-stopasynchronousengine--cooperative-shutdown-via-notification-bits)

### TS04: Modular and Reusable Components
- **As an** IoT Developer, **I want** modular and reusable component contracts, **so that** I can reuse capabilities across projects with different hardware mappings.
- **Acceptance Criteria**:
    - **Given** components are configured with different physical mappings, **when** the application starts, **then** behavior remains correct without framework source changes.
    - **Given** a new project, **when** the same component contracts are reused, **then** a different device behavior can be assembled quickly.
    - Developers can add new component types by extending documented contracts without modifying existing implementations.
    - **Given** a sensor subclass, **when** it declares default and minimum sampling intervals, **then** the scheduler resolves them polymorphically via virtual getters.
    - **Given** a sensor instance needs a custom period, **when** `setSamplingIntervalInMilliseconds()` is called, **then** the per-instance override takes precedence over the class default.
- **Status**: ✓ Implemented. See `Sensor.h` (virtual getters, runtime overrides), 10 sensor subclasses, and `tests/SamplingScheduler/test.cpp`.
  Related: [ADR-003](adrs.md#adr-003-hybrid-approach-for-sensor-sampling-intervals), [ADR-011](adrs.md#adr-011-per-slot-samplingscheduler-with-countdown-based-dispatch)

### TS05: Lightweight and Efficient Design
- **As an** IoT Developer, **I want** a lightweight and efficient framework, **so that** it performs well on resource-constrained microcontrollers.
- **Acceptance Criteria**:
    - **Given** a minimal application that uses core capabilities, **when** it is built, **then** framework overhead remains bounded and documented.
    - **Given** typical event/command workloads, **when** execution latency is measured, **then** timing remains within documented real-time targets.
    - Memory behavior is predictable under sustained operation and does not degrade over time.
    - **Given** logging is compiled out, **when** the application runs, **then** `MODEST_LOG_*` macros produce zero Flash and zero runtime overhead.
    - **Given** a telemetry payload exceeds the bounded serialization buffer, **when** serialization is attempted, **then** a warning is logged and the sending is skipped without heap allocation.
- **Status**: ✓ Implemented. See `ModestLog.h` (compile-time filtering), `MqttGatewayClient.cpp` (512-byte bounded buffer), and `DiagnosticsSnapshot`.
  Related: [ADR-001](adrs.md#adr-001-framework-naming-convention--modest_log_-macros-with-miot_log_-aliases), [ADR-006](adrs.md#adr-006-bounded-serialization-buffer-in-mqttgatewayclient), [ADR-018](adrs.md#adr-018-diagnosticssnapshot--on-demand-stack-and-heap-metrics)

### TS06: Manage Connectivity Lifecycle
- **As an** IoT Developer, **I want** connectivity to be abstracted behind a transport contract, **so that** link setup and health checks are portable across networks.
- **Acceptance Criteria**:
    - **Given** no active network link, **when** a connection is requested, **then** the framework initiates a non-blocking or bounded connection workflow.
    - **Given** a degraded or lost link, **when** connectivity is checked, **then** the framework reports not-ready state accurately.
    - **Given** a healthy link, **when** transmission readiness is queried, **then** the framework reports ready state consistently.
    - **Given** a Wi-Fi link drops, **when** reconnection is attempted, **then** the driver retries with configurable intervals and re-arms on failure.
    - **Given** an MQTT broker disconnects, **when** reconnection is attempted, **then** the client uses rate-limited backoff and continuous keep-alive via `service()`.
    - **Given** a BLE peer connects, **when** data is transmitted, **then** GATT notifications are sent via the Nordic UART Service (NUS) TX characteristic.
- **Status**: ✓ Implemented. See `WiFiConnectivityDriver`, `MqttGatewayClient` (reconnect backoff, keep-alive), `BleConnectivityDriver` (NUS GATT), and `tests/MqttGatewayClient/test.cpp`.
  Related: [ADR-002](adrs.md#adr-002-ble-connectivity-via-nordic-uart-service-nus), [ADR-019](adrs.md#adr-019-mqtt-reconnect-policy--rate-limited-backoff-with-continuous-keep-alive)

### TS07: Publish Telemetry to Edge Services
- **As an** IoT Developer, **I want** telemetry payload publication to be protocol-agnostic at the application level, **so that** I can switch transport strategies without changing device business logic.
- **Acceptance Criteria**:
    - **Given** a telemetry payload object, **when** publication is requested, **then** it is serialized to a transportable representation and sent through the selected gateway.
    - **Given** transient publication failure, **when** a publication attempt occurs, **then** the framework returns a clear success/failure result without crashing.
    - **Given** both request/response and broker-style integrations, **when** the same payload contract is used, **then** both publication paths are supported.
    - **Given** a telemetry package is enqueued, **when** it enters the pipeline, **then** it receives a monotonically increasing sequence number and a capture timestamp.
    - **Given** the telemetry queue is full, **when** a new packet is enqueued, **then** the oldest packet is dropped and the new one is inserted (drop-oldest policy).
    - **Given** a telemetry package is created with `std::make_unique`, **when** enqueue fails, **then** the `unique_ptr` automatically deletes the packet without manual cleanup.
- **Status**: ✓ Implemented. See `TelemetryPackage.h` (envelope metadata), `Device.cpp` (drop-oldest, `unique_ptr` overload), `MqttGatewayClient.cpp` (bounded buffer), and `tests/TelemetryPackage/test.cpp`.
  Related: [ADR-004](adrs.md#adr-004-unique_ptr-ownership-for-telemetry-enqueue), [ADR-005](adrs.md#adr-005-drop-oldest-queue-policy), [ADR-006](adrs.md#adr-006-bounded-serialization-buffer-in-mqttgatewayclient), [ADR-007](adrs.md#adr-007-envelope-metadata-in-telemetry-packages), [ADR-021](adrs.md#adr-021-golden-host-tests-for-telemetry-json-serialization)

### TS08: Orchestrate Asynchronous Sampling and Dispatch
- **As an** IoT Developer, **I want** asynchronous sampling and dispatch orchestration, **so that** hardware polling and network publication do not block each other.
- **Acceptance Criteria**:
    - **Given** multiple registered input tasks, **when** the scheduler runs, **then** work is processed in a bounded round-robin sequence.
    - **Given** telemetry items are produced faster than they are transmitted, **when** buffering occurs, **then** queue behavior follows documented capacity rules.
    - **Given** long-running operation, **when** background workers execute, **then** input capture and dispatch remain isolated and responsive.
    - **Given** sensors with different sampling periods, **when** the scheduler advances, **then** each sensor polls at its own interval via per-slot countdown timers.
    - **Given** a sensor's requested period is less than its minimum, **when** it is registered, **then** the period is clamped to the minimum and a warning is logged.
    - **Given** the network task runs, **when** the telemetry queue is empty, **then** it services MQTT keep-alive and Wi-Fi reconnection in the background.
- **Status**: ✓ Implemented. See `SamplingScheduler.h/.cpp`, `Device.cpp` (per-slot scheduling, bounded-timeout network worker), and `tests/SamplingScheduler/test.cpp`.
  Related: [ADR-011](adrs.md#adr-011-per-slot-samplingscheduler-with-countdown-based-dispatch), [ADR-014](adrs.md#adr-014-threading-model--single-context-sampler-with-isolated-network-task), [ADR-019](adrs.md#adr-019-mqtt-reconnect-policy--rate-limited-backoff-with-continuous-keep-alive)

### TS09: Handle Data Quality and Failure Conditions
- **As an** IoT Developer, **I want** explicit handling for invalid or duplicate measurements, **so that** downstream logic receives meaningful signals instead of noisy data.
- **Acceptance Criteria**:
    - **Given** an invalid sensor reading, **when** processing occurs, **then** propagation is suppressed or marked as invalid per policy.
    - **Given** unchanged readings where change-only propagation is enabled, **when** sampling repeats, **then** duplicate events are suppressed.
    - **Given** operational errors, **when** they occur, **then** the framework fails safely and preserves runtime stability.
    - **Given** a sensor faults, **when** the mediator receives the fault event, **then** it can react (e.g., turn LED red, publish health telemetry).
    - **Given** a packet is dropped from the queue, **when** the first drop occurs, **then** a warning is logged exactly once.
    - **Given** an MQTT send fails, **when** the first failure occurs, **then** a warning is logged exactly once with the topic name.
- **Status**: ✓ Implemented. See `Sensor.cpp` (fault state machine), `Device.cpp` (first-drop logging), `MqttGatewayClient.cpp` (first-failure logging), and `tests/SensorFault/test.cpp`.
  Related: [ADR-009](adrs.md#adr-009-sensor-fault-events-with-debounced-threshold-state-machine), [ADR-005](adrs.md#adr-005-drop-oldest-queue-policy)

### TS10: Extend Framework Safely
- **As an** IoT Developer, **I want** stable extension points for inputs, outputs, telemetry payloads, and transports, **so that** I can customize behavior without forking the framework.
- **Acceptance Criteria**:
    - **Given** a new custom component implementation, **when** it adheres to framework contracts, **then** it integrates without changing existing modules.
    - **Given** a custom telemetry payload shape, **when** it is serialized through the payload contract, **then** gateways can publish it without special-case logic.
    - **Given** a new connectivity backend, **when** it implements the transport contract, **then** higher-level application code remains unchanged.
    - **Given** a custom event identifier, **when** it is defined in the user range (1000+), **then** it does not collide with framework-reserved identifiers.
    - **Given** a new sensor subclass, **when** it implements virtual sampling interval getters, **then** it integrates with the scheduler automatically.
- **Status**: ✓ Implemented. See `EventHandler.h`, `Sensor.h`, `Actuator.h`, `ConnectivityDriver.h`, `GatewayClient.h`, `TelemetryPackage.h`, and README (Identifier Registry).
  Related: [ADR-015](adrs.md#adr-015-identifier-registry-with-reserved-ranges), [ADR-023](adrs.md#adr-023-domain-driven-design-adoption-and-bounded-contexts)

### TS11: Observe System Health via Diagnostics
- **As an** IoT Developer, **I want** to query runtime diagnostics at any time, **so that** I can tune task stacks, detect memory pressure, and observe queue behavior.
- **Acceptance Criteria**:
    - **Given** the engine is running, **when** `Device::collectDiagnostics()` is called, **then** a 16-field snapshot is returned with heap, stack, queue, and send statistics.
    - **Given** stack high-water marks are queried, **when** the snapshot is computed, **then** values are reported in words, as returned by the FreeRTOS API.
    - **Given** the diagnostics snapshot includes `worstCaseWakeMicros`, **when** the snapshot is collected, **then** its current value is reported as 0 and is not treated as a measured wake-time metric.
    - **Given** the diagnostics query runs, **when** it completes, **then** no background CPU overhead is incurred during normal operation.
    - **Given** gateway send statistics are available, **when** the snapshot is computed, **then** send success and failure counts are included.
- **Status**: ✓ Implemented. See `Device.h` (`DiagnosticsSnapshot`), `Device.cpp` (`collectDiagnostics()`), `GatewayClient.h` (`GatewayStatistics`), and `tests/DiagnosticsSnapshot/test.cpp`.
  Related: [ADR-018](adrs.md#adr-018-diagnosticssnapshot--on-demand-stack-and-heap-metrics), [ADR-010](adrs.md#adr-010-engineconfig-for-tunable-freertos-task-parameters)

### TS12: Log Framework Activity with Zero-Cost Macros
- **As an** IoT Developer, **I want** compile-time filtered logging macros, **so that** I can debug device behavior without runtime overhead when logging is disabled.
- **Acceptance Criteria**:
    - **Given** `MODEST_LOG_LEVEL` is set to 2 (WARN), **when** `MODEST_LOG_D` is called, **then** the macro compiles to `((void)0)` with zero Flash and zero runtime cost.
    - **Given** `MODEST_LOG_LEVEL` is set to 4 (DEBUG), **when** `MODEST_LOG_D` is called, **then** the message is formatted and sent to the configured sink.
    - **Given** a host unit test needs to capture logs, **when** `modest_iot::setLogSink()` is called, **then** log output is redirected to the test sink.
    - **Given** a developer prefers short macro names, **when** `MODEST_USE_SHORT_LOG_MACROS` is defined, **then** `MIOT_LOG_*` aliases are available.
- **Status**: ✓ Implemented. See `ModestLog.h` and `tests/ModestLog/test.cpp`.
  Related: [ADR-001](adrs.md#adr-001-framework-naming-convention--modest_log_-macros-with-miot_log_-aliases), [ADR-013](adrs.md#adr-013-screaming_snake_case-for-all-preprocessor-macros)

### TS13: Configure Engine Task Parameters
- **As an** IoT Developer, **I want** to configure FreeRTOS task stack sizes, priorities, and core pinning, **so that** I can tune the engine for my device's memory and performance requirements.
- **Acceptance Criteria**:
    - **Given** an `EngineConfiguration` struct, **when** `initializeAsynchronousEngine(config)` is called, **then** tasks are created with the specified stack sizes, priorities, and core affinity.
    - **Given** no config is provided, **when** `initializeAsynchronousEngine(tickInterval)` is called, **then** default values are used (4096/8192 bytes, priority 3/1, core 1/1, queue 10).
    - **Given** the engine is running, **when** `stopAsynchronousEngine()` is called, **then** tasks terminate cooperatively, the timer is cleaned up, and the queue is drained.
    - **Given** `stopAsynchronousEngine()` is called twice, **when** the second call executes, **then** it is a no-op (idempotent double-stop guard).
- **Status**: ✓ Implemented. See `Device.h` (`EngineConfiguration`), `Device.cpp` (`stopAsynchronousEngine`), and `tests/EngineConfig/test.cpp`.
  Related: [ADR-010](adrs.md#adr-010-engineconfig-for-tunable-freertos-task-parameters), [ADR-020](adrs.md#adr-020-stopasynchronousengine--cooperative-shutdown-via-notification-bits)

### TS14: Ensure Telemetry Ownership Safety
- **As an** IoT Developer, **I want** safe ownership transfer for telemetry packages, **so that** I don't leak memory when enqueue fails or forget to clean up on error paths.
- **Acceptance Criteria**:
    - **Given** a telemetry package created with `std::make_unique`, **when** `enqueueTelemetryPayload(std::move(packet))` succeeds, **then** ownership transfers to the queue and the caller no longer owns the packet.
    - **Given** a telemetry package created with `std::make_unique`, **when** `enqueueTelemetryPayload(std::move(packet))` fails, **then** the `unique_ptr` automatically deletes the packet.
    - **Given** the telemetry queue is full, **when** a new packet is enqueued, **then** the oldest packet is dropped and the new one is inserted.
    - **Given** a telemetry package is serialized, **when** the JSON exceeds the 512-byte buffer, **then** a `PayloadTooLarge` warning is logged and the sending is skipped.
- **Status**: ✓ Implemented. See `Device.h` (`unique_ptr` overload), `Device.cpp` (drop-oldest), `MqttGatewayClient.cpp` (bounded buffer), and `tests/DiagnosticsSnapshot/test.cpp`.
  Related: [ADR-004](adrs.md#adr-004-unique_ptr-ownership-for-telemetry-enqueue), [ADR-005](adrs.md#adr-005-drop-oldest-queue-policy), [ADR-006](adrs.md#adr-006-bounded-serialization-buffer-in-mqttgatewayclient)

### TS15: Validate Framework with Automated Tests
- **As an** IoT Developer, **I want** the framework to have comprehensive host unit tests and CI workflows, **so that** I can trust the codebase compiles and behaves correctly without deploying to hardware.
- **Acceptance Criteria**:
    - **Given** a new pull request, **when** CI runs, **then** all 53 current host test suites pass with Address and Undefined sanitizers enabled.
    - **Given** a new pull request, **when** CI runs, **then** all 28 current examples compile cleanly on ESP32 and ESP32-S3 targets.
    - **Given** library metadata is changed, **when** CI runs, **then** `arduino-lint` validates Arduino Library Manager compliance.
    - **Given** a telemetry package is serialized in a host test, **when** the golden JSON is compared, **then** the exact output matches the expected string.
- **Status**: ✓ Implemented. See `tests/run_all.sh`, `.github/workflows/host-tests.yml`, `.github/workflows/compile-examples.yml`, `.github/workflows/lint.yml`, and `tests/TelemetryPackage/test.cpp`.
  Related: [ADR-017](adrs.md#adr-017-shared-mock-harness-for-host-unit-tests), [ADR-021](adrs.md#adr-021-golden-host-tests-for-telemetry-json-serialization)

### TS16: Monitor Battery State

- **As an** IoT Developer, **I want** to monitor battery voltage and level on battery-powered devices, **so that** I can take action when the battery is low (reduce sampling, enter deep sleep, or alert the user).
- **Acceptance Criteria**:
    - **Given** a battery is connected to an ADC pin, **when** `PowerManager::begin()` is called, **then** the voltage divider ratio is configured and readings begin.
    - **Given** the battery voltage is read, **when** `getBatteryVoltageInMillivolts()` is called, **then** the value is returned in millivolts with proper ADC-to-voltage conversion.
    - **Given** the battery voltage is known, **when** `getBatteryLevelInPercentage()` is called, **then** a 0.0–100.0 percentage is returned based on the configured voltage range.
    - **Given** the battery voltage drops below the threshold, **when** `isLowBattery()` is called, **then** it returns `true` so the application can take its configured low-battery action; `PowerManager` does not provide a callback API.
    - **Given** a custom battery chemistry, **when** `setBatteryVoltageRange()` is called, **then** the min/max voltage bounds are updated for level calculation.
- **Status**: ✓ Implemented (v1.1.0). See `PowerManager.h`, `PowerManager.cpp`, and `tests/PowerManager/test.cpp` (18 tests).

### TS17: Buffer Telemetry Offline

- **As an** IoT Developer, **I want** an optional bounded persistent telemetry buffer that my application can integrate when a send fails, **so that** buffered packets can be replayed after connectivity returns, subject to the buffer capacity and eviction policy.
- **Acceptance Criteria**:
    - **Given** an application integrates `TelemetryBuffer`, **when** it chooses to persist a failed telemetry send, **then** it can store the packet JSON in LittleFS via `TelemetryBuffer::store()`; the base `Device` pipeline does not persist failed sends automatically.
    - **Given** `ClimateDataUploader` handles a failed send in its custom `processQueuedTelemetryData()` override, **when** it stores the packet, **then** the example uses `TelemetryBuffer` for offline persistence.
    - **Given** the buffer is at capacity, **when** a new packet is stored, **then** the oldest packet is evicted (FIFO policy), so persistence is bounded and does not guarantee that no data is lost; the eviction callback fires if registered.
    - **Given** connectivity is restored, **when** `TelemetryBuffer::retrieveNext()` is called, **then** packets are returned in FIFO order until the buffer is empty.
    - **Given** the device reboots unexpectedly, **when** `TelemetryBuffer::begin()` is called, **then** previously buffered packets are recovered from LittleFS.
    - **Given** the buffer is cleared, **when** `TelemetryBuffer::clear()` is called, **then** all buffered packets are removed from LittleFS.
- **Status**: ✓ Implemented (v1.1.0). See `TelemetryBuffer.h`, `TelemetryBuffer.cpp`, and `tests/TelemetryBuffer/test.cpp` (22 tests).

### TS18: Receive OTA Updates

- **As an** IoT Developer, **I want** deployed devices to receive Over-The-Air firmware updates, **so that** I can fix bugs and add features without physical access to the device.
- **Acceptance Criteria**:
    - **Given** OTA is configured, **when** `OtaUpdateHandler::begin()` is called, **then** the device listens for incoming firmware updates with the specified hostname.
    - **Given** an update is in progress, **when** the progress callback fires, **then** the number of bytes written and total bytes are reported.
    - **Given** an update fails, **when** the error callback fires, **then** the error code is reported and the status transitions to `Error`.
    - **Given** the `service()` method is called, **when** no update is pending, **then** the call is a no-op (safe to call repeatedly from `maintainNetworkServices()`).
    - **Given** a password is set, **when** an update is requested, **then** the password must match for the update to proceed.
- **Status**: ✓ Implemented (v1.1.0). See `OtaUpdateHandler.h`, `OtaUpdateHandler.cpp`, and `tests/OtaUpdateHandler/test.cpp` (21 tests).

### TS19: Report Firmware Version in Telemetry

- **As an** IoT Developer, **I want** every telemetry packet to include the firmware version, **so that** I can track which version each device is running and correlate telemetry format changes.
- **Acceptance Criteria**:
    - **Given** a firmware version is set, **when** `Device::setFirmwareVersion()` is called, **then** the version string appears in the JSON envelope of every telemetry record.
    - **Given** a schema version is set, **when** `Device::setTelemetrySchemaVersion()` is called, **then** the schema version appears in the JSON envelope of every telemetry record.
    - **Given** neither version is set, **when** telemetry is serialized, **then** both fields are omitted from the JSON output with zero overhead.
    - **Given** only the firmware version is set, **when** telemetry is serialized, **then** only `firmwareVersion` appears, `schemaVersion` is omitted.
- **Status**: ✓ Implemented (v1.1.0). See `TelemetryPackage.h`, `Device.h`, `Device.cpp`, and `tests/TelemetryPackage/test.cpp` (5 new tests).

### TS20: Install via Main IoT Registry Providers

- **As an** IoT Developer, **I want** to install the Modest IoT Nano Framework from a registry provider without manual steps, **so that** I can use the framework in my project with a single configuration line.
- **Acceptance Criteria**:
    - **Given** the framework is published to the Arduino Library Manager, **when** the Arduino IDE Library Manager lists available libraries, **then** the framework appears with the correct name, version, and description.
    - **Given** the framework is published to the Arduino Library Manager, **when** installation is triggered, **then** the framework and its dependency (MQTT) are downloaded and available in the sketch.
    - **Given** a `library.json` manifest exists, **when** `Modest-IoT-Nano-Framework` is added to `lib_deps` in `platformio.ini`, **then** PlatformIO resolves and downloads the framework and its dependencies.
    - **Given** the framework is installed via either registry, **when** a sketch that includes `ModestIoT.h` is built, **then** the build succeeds without missing header errors.
    - **Given** both `library.json` and `library.properties` exist, **when** the metadata is compared, **then** the name, version, and dependencies are consistent between the two files.
- **Status**: ✓ Implemented (v1.1.2). See `library.properties` (Arduino Library Manager, published) and `library.json` (PlatformIO Registry manifest).

### TS21: Follow Quick Start Guide

- **As an** IoT Developer, **I want** a step-by-step Quick Start Guide, **so that** I can go from zero to a working device in under 10 minutes.
- **Acceptance Criteria**:
    - **Given** the Quick Start Guide offers two routes, **when** the user chooses the Simulation route, **then** the `StatusIndicator` example runs in Wokwi without any hardware or toolchain installation.
    - **Given** the Simulation route is followed, **when** the required libraries are added in Wokwi, **then** the framework and MQTT are available for the simulated sketch.
    - **Given** the user chooses the Hardware route, **when** the Quick Start Guide is followed, **then** the framework is installed, the `StatusIndicator` example is opened, and the sketch is uploaded to the ESP32 board.
    - **Given** the Hardware route covers installation, **when** the Installation section is read, **then** three toolchain paths are documented: Arduino Library Manager, PlatformIO in VS Code, and PlatformIO in CLion.
    - **Given** the `StatusIndicator` example is uploaded, **when** the Serial Monitor is connected, **then** LED toggle status messages are output confirming the framework is running.
    - **Given** the Quick Start Guide is complete, **when** the Next Steps section is reached, **then** links to the full README, examples, and documentation are provided.
- **Status**: ✓ Implemented (v1.1.2). See `docs/quick-start.md` (two-route guide: Simulation via Wokwi, Hardware via ESP32) and `examples/StatusIndicator`.

### TS22: Format-Neutral Telemetry Serialization

- **As an** IoT Developer, **I want** telemetry packages to be decoupled from the JSON serialization format, **so that** I can swap JSON for CBOR, MessagePack, or binary protocols without changing telemetry package code.
- **Acceptance Criteria**:
    - **Given** a `TelemetryWriter` interface exists, **when** a `TelemetryPackage` subclass implements `write(TelemetryWriter&)`, **then** the package declares data fields without knowing the serialization format.
    - **Given** a `JsonTelemetryWriter` implementation exists, **when** `write()` is called with a `JsonTelemetryWriter`, **then** the output is valid JSON with envelope metadata (timestamp, sequence, schemaVersion, firmwareVersion).
    - **Given** the `TelemetryWriter` interface is abstract, **when** a new format writer (e.g., CBOR) is needed, **then** only the new writer class needs to be implemented — no telemetry package changes required.
    - **Given** `JsonTelemetryWriter` uses a fixed stack buffer, **when** serialization occurs, **then** no heap allocation is made (zero `JsonDocument` usage).
- **Status**: ✓ Implemented (v1.2.0). See `TelemetryWriter.h`, `JsonTelemetryWriter.h`, `JsonTelemetryWriter.cpp`, and `TelemetryPackage.h`.

### TS23: Native JSON Parsing for MQTT Commands

- **As an** IoT Developer, **I want** incoming MQTT commands to be parsed without ArduinoJson, **so that** the framework has zero external dependencies for JSON handling.
- **Acceptance Criteria**:
    - **Given** an incoming MQTT message with JSON payload `{"commandId": N, "payload": V}`, **when** the native JSON parser extracts values, **then** the command ID and payload are extracted correctly.
    - **Given** the JSON payload is malformed, **when** the native JSON parser processes it, **then** an error is logged and the message is ignored.
    - **Given** the JSON payload is missing the `commandId` field, **when** the native JSON parser processes it, **then** an error is logged and the message is ignored.
- **Status**: ✓ Implemented (v1.2.0). See `MqttGatewayClient.cpp` (`JsonParser::parseInt()`, `JsonParser::parseInt32()`).

### TS24: ArduinoJson Dependency Removal

- **As an** IoT Developer, **I want** the framework to have no ArduinoJson dependency, **so that** my firmware saves ~30-40 KB of Flash and avoids per-serialization heap allocations.
- **Acceptance Criteria**:
    - **Given** W46 is complete, **when** `library.properties` and `library.json` are checked, **then** ArduinoJson is removed from the dependencies list.
    - **Given** W46 is complete, **when** all source files are searched for `#include <ArduinoJson.h>`, **then** no matches are found in production code.
    - **Given** W46 is complete, **when** all 26 examples in the v1.2.0 baseline compile, **then** the build succeeds without ArduinoJson.
    - **Given** W46 is complete, **when** all 51 host test suites in the v1.2.0 baseline run, **then** all tests pass without the ArduinoJson mock.
- **Status**: ✓ Implemented (v1.2.0). See `library.properties`, `library.json`, and all migrated source files.

### TS25: Implement TelemetryWriter Abstraction

- **As an** IoT Developer, **I want** a `TelemetryWriter` interface and `JsonTelemetryWriter` implementation, **so that** telemetry packages can serialize to JSON without ArduinoJson.
- **Acceptance Criteria**:
    - **Given** `TelemetryWriter` is an abstract class, **when** the interface is reviewed, **then** it has pure virtual `field()` overloads for `int32_t`, `float`, `bool`, and `const char*`.
    - **Given** `JsonTelemetryWriter` implements `TelemetryWriter`, **when** `field()` is called, **then** the value is appended to a fixed-size stack buffer using `snprintf`.
    - **Given** `JsonTelemetryWriter` has a `finalize()` method, **when** all fields are written, **then** the buffer contains valid JSON with proper comma separation and closing brace.
    - **Given** `JsonTelemetryWriter` uses a 512-byte buffer, **when** the buffer is full, **then** serialization stops and an error is logged.
- **Status**: ✓ Implemented (v1.2.0). See `TelemetryWriter.h`, `JsonTelemetryWriter.h`, `JsonTelemetryWriter.cpp`.

### TS26: Migrate TelemetryPackage to TelemetryWriter

- **As an** IoT Developer, **I want** `TelemetryPackage` to use `TelemetryWriter` instead of `JsonDocument`, **so that** the core API is decoupled from ArduinoJson.
- **Acceptance Criteria**:
    - **Given** `TelemetryPackage` has a `write(TelemetryWriter&)` pure virtual method, **when** subclasses override it, **then** they call `writer.field()` for each data field.
    - **Given** `TelemetryPackage::toJsonString()` exists, **when** it is called, **then** it creates a `JsonTelemetryWriter`, calls `addEnvelopeMetadata()` and `write()`, and returns the JSON string.
    - **Given** `TelemetryPackage::addEnvelopeMetadata()` exists, **when** it is called with a `TelemetryWriter`, **then** timestamp, sequence, schemaVersion, and firmwareVersion fields are written.
    - **Given** the old `serialize(JsonDocument&)` method is removed, **when** the codebase is searched for `serialize(JsonDocument&)`, **then** no matches are found.
- **Status**: ✓ Implemented (v1.2.0). See `TelemetryPackage.h`.

### TS27: Migrate Gateway Clients and Add Native JSON Parser

- **As an** IoT Developer, **I want** gateway clients to use `JsonTelemetryWriter` and a native JSON parser, **so that** no ArduinoJson is used in the network task.
- **Acceptance Criteria**:
    - **Given** `MqttGatewayClient::sendTelemetryRecord()` uses `serializeToBuffer()`, **when** telemetry is sent, **then** the JSON payload is serialized to a fixed buffer without heap allocation.
    - **Given** `BleGatewayClient::sendTelemetryRecord()` uses `serializeToBuffer()`, **when** telemetry is sent, **then** the JSON payload is serialized to a fixed buffer without heap allocation.
    - **Given** `mqttMessageCallback()` uses a native JSON parser, **when** a command arrives, **then** `commandId` and `payload` are extracted without ArduinoJson.
    - **Given** the native JSON parser handles malformed JSON, **when** invalid JSON arrives, **then** an error is logged and the message is ignored.
- **Status**: ✓ Implemented (v1.2.0). See `MqttGatewayClient.cpp`, `BleGatewayClient.cpp`, `HttpGatewayClient.cpp`.

### TS28: Enqueue Critical Alerts via High-Priority Lane

- **As an** IoT Developer, **I want** to enqueue critical alerts via a high-priority lane, **so that** alarms bypass the standard telemetry queue and are transmitted immediately.
- **Acceptance Criteria**:
    - **Given** a Device has a high-priority queue, **when** `enqueueHighPriorityPayload()` is called with a TelemetryPackage, **then** the package is placed in the high-priority queue instead of the standard queue.
    - **Given** the high-priority queue is bounded (4 slots), **when** `enqueueHighPriorityPayload()` is called and the queue is full, **then** the alert is dropped and a warning is logged.
    - **Given** a high-priority alert is enqueued, **when** the network task processes the high-priority lane, **then** the alert packet gets envelope field `"priority": "high"` for edge-side routing.
    - **Given** a high-priority alert is enqueued while the standard queue has pending packets, **when** the network task runs, **then** the alert is processed before standard telemetry packets.
- **Status**: ✓ Implemented (v1.2.0). See `Device.h`, `Device.cpp`, `TelemetryPackage.h`.

### TS29: High-Priority Lane Diagnostics

- **As an** IoT Developer, **I want** to monitor high-priority lane statistics via DiagnosticsSnapshot, **so that** I can detect dropped alerts and high-priority queue pressure.
- **Acceptance Criteria**:
    - **Given** high-priority alerts have been enqueued, **when** `Device::collectDiagnostics()` is called, **then** `highPriorityEnqueued` reflects the total count of alerts successfully enqueued.
    - **Given** high-priority alerts have been dropped due to a full queue, **when** `Device::collectDiagnostics()` is called, **then** `highPriorityDropped` reflects the total count of dropped alerts.
    - **Given** the high-priority queue has reached a certain depth, **when** `Device::collectDiagnostics()` is called, **then** `highPriorityQueueHighWaterMark` reflects the maximum depth observed.
- **Status**: ✓ Implemented (v1.2.0). See `Device.h` (DiagnosticsSnapshot extension), `Device.cpp`.

### TS30: Configure MQTT QoS for Telemetry Publishing

- **As an** IoT Developer, **I want** to configure the QoS level for MQTT telemetry publishing, **so that** I can ensure message delivery reliability based on my application's requirements.
- **Acceptance Criteria**:
    - **Given** a MqttGatewayClient is created with QoS 1, **when** `sendTelemetryRecord()` publishes a message, **then** the MQTT publish uses QoS 1.
    - **Given** a MqttGatewayClient is created with QoS 0 (default), **when** `sendTelemetryRecord()` publishes a message, **then** the MQTT publish uses QoS 0.
    - **Given** a MqttGatewayClient is created with QoS 2, **when** `sendTelemetryRecord()` publishes a message, **then** the MQTT publish uses QoS 2.
    - **Given** a MqttGatewayClient is created, **when** QoS is changed via `setPublishQoS()`, **then** subsequent publishes use the new QoS level.
- **Status**: ✓ Implemented (v1.2.0). See `MqttGatewayClient.h`, `MqttGatewayClient.cpp`.

### TS31: Configure Retained Messages for Last-Known-Good Telemetry

- **As an** IoT Developer, **I want** to publish telemetry with the retained flag, **so that** the MQTT broker stores the last-known-good message for new subscribers.
- **Acceptance Criteria**:
    - **Given** a MqttGatewayClient is created with retained mode enabled, **when** `sendTelemetryRecord()` publishes a message, **then** the MQTT publish includes the retained flag.
    - **Given** a MqttGatewayClient is created with retained mode disabled (default), **when** `sendTelemetryRecord()` publishes a message, **then** the MQTT publish does not include the retained flag.
    - **Given** a MqttGatewayClient is created, **when** retained mode is changed via `setRetained()`, **then** subsequent publishes use the new retained setting.
- **Status**: ✓ Implemented (v1.2.0). See `MqttGatewayClient.h`, `MqttGatewayClient.cpp`.

### TS32: Configure MQTT QoS for Subscribe

- **As an** IoT Developer, **I want** to configure the QoS level for MQTT topic subscription, **so that** incoming commands are received with the desired delivery guarantee.
- **Acceptance Criteria**:
    - **Given** a MqttGatewayClient is created with subscribe QoS 1, **when** the client subscribes to the command topic, **then** the subscription uses QoS 1.
    - **Given** a MqttGatewayClient is created with subscribe QoS 0 (default), **when** the client subscribes to the command topic, **then** the subscription uses QoS 0.
- **Status**: ✓ Implemented (v1.2.0). See `MqttGatewayClient.h`, `MqttGatewayClient.cpp`.

### TS33: Add QoS and Retained Configuration to MqttGatewayClient

- **As an** IoT Developer, **I want** MqttGatewayClient to accept QoS and retained parameters, **so that** I can configure delivery guarantees at construction time.
- **Acceptance Criteria**:
    - **Given** MqttGatewayClient constructors accept QoS and retained parameters, **when** a client is created, **then** the parameters are stored as member variables.
    - **Given** the default constructor parameters, **when** a client is created without explicit QoS/retained, **then** QoS defaults to 0 and retained defaults to false.
    - **Given** `setPublishQoS()`, `setRetained()`, and `setSubscribeQoS()` exist, **when** they are called, **then** the stored values are updated.
    - **Given** `getPublishQoS()`, `getRetained()`, and `getSubscribeQoS()` exist, **when** they are called, **then** the current values are returned.
- **Status**: ✓ Implemented (v1.2.0). See `MqttGatewayClient.h`, `MqttGatewayClient.cpp`.

### TS34: Update MQTT publish and subscribe to use QoS and Retained

- **As an** IoT Developer, **I want** MqttGatewayClient to pass QoS and retained to the MQTT library, **so that** messages are published with the configured delivery guarantees.
- **Acceptance Criteria**:
    - **Given** `sendTelemetryRecord()` calls `mqttClient.publish()`, **when** the method executes, **then** the publish call includes the configured QoS and retained parameters.
    - **Given** `subscribeToTopic()` calls `mqttClient.subscribe()`, **when** the method executes, **then** the subscribe call uses the configured subscribe QoS.
    - **Given** the test mock is updated, **when** a test verifies QoS and retained, **then** the mock records and exposes these values for assertion.
- **Status**: ✓ Implemented (v1.2.0). See `MqttGatewayClient.cpp`, `tests/fakes/MQTT.h`, `tests/MqttGatewayClient/test.cpp`.

### TS35: Parse JSON Commands with JsonParser

- **As an** IoT Developer, **I want** a general-purpose JSON parser for application payloads, **so that** I can extract values without ArduinoJson and use it with data from MQTT, HTTP, BLE, or other sources.
- **Acceptance Criteria**:
    - **Given** a JSON string `{"commandId": 1, "threshold": 42.5}`, **when** `JsonParser::parseInt()` is called with key "commandId", **then** the value 1 is extracted.
    - **Given** a JSON string `{"enabled": true}`, **when** `JsonParser::parseBool()` is called with key "enabled", **then** the value true is extracted.
    - **Given** a JSON string `{"name": "sensor_01"}`, **when** `JsonParser::parseString()` is called with key "name", **then** the string "sensor_01" is copied to the output buffer.
    - **Given** a missing key, **when** any `JsonParser` method is called, **then** false is returned and the output value is unchanged.
    - **Given** an application receives JSON through HTTP or BLE, **when** it wants to parse that payload, **then** it can call `JsonParser` itself; the built-in transport integration is the MQTT command callback.
- **Status**: ✓ Implemented (v1.2.0). See `JsonParser.h`, `JsonParser.cpp`, `tests/JsonParser/test.cpp`.

### TS36: Request On-Demand Diagnostics via MQTT Command

- **As an** IoT Developer, **I want** to request diagnostics from a device via an MQTT command, **so that** I can monitor fleet health without continuous telemetry overhead.
- **Acceptance Criteria**:
    - **Given** a device is connected to an MQTT broker, **when** a command with `commandId` equal to 300 is published, **then** the device creates a `DiagnosticsTelemetryPackage` from `Device::collectDiagnostics()`.
    - **Given** a diagnostics command is received, **when** the device serializes the diagnostics snapshot, **then** the JSON output contains all 16 DiagnosticsSnapshot fields (missedTicks, packetsEnqueued, highPriorityEnqueued, sendsSucceeded, freeHeapBytes, etc.).
    - **Given** a diagnostics command is received, **when** the device sends the diagnostics telemetry, **then** it uses `enqueueTelemetryPayload()` (standard queue) to avoid blocking the sampler task.
    - **Given** a diagnostics command is received but no gateway client is registered, **when** the device processes the command, **then** a warning is logged and no telemetry is sent.
- **Status**: ✓ Implemented (v1.2.0). See `Device.h`, `Device.cpp`, `DiagnosticsTelemetryPackage.h`.

### TS37: Diagnostics Command Integration with JsonParser

- **As an** IoT Developer, **I want** the diagnostics command to be parsed using `JsonParser`, **so that** the framework maintains zero external dependencies for JSON handling.
- **Acceptance Criteria**:
    - **Given** an incoming MQTT message with JSON payload `{"commandId": 300}`, **when** `JsonParser::parseInt()` extracts the command ID, **then** the diagnostics command is recognized and processed.
    - **Given** the JSON payload is malformed or missing `commandId`, **when** the MQTT callback processes it, **then** an error is logged and the message is ignored.
- **Status**: ✓ Implemented (v1.2.0). See `MqttGatewayClient.cpp` (uses `JsonParser::parseInt()`), `Device.cpp` (handle diagnostics command).

### TS38: Implement DiagnosticsTelemetryPackage

- **As an** IoT Developer, **I want** a `DiagnosticsTelemetryPackage` that wraps `DiagnosticsSnapshot` fields, **so that** diagnostics data is serialized as JSON via `TelemetryWriter`.
- **Acceptance Criteria**:
    - **Given** `DiagnosticsTelemetryPackage` exists, **when** it is constructed with a `DiagnosticsSnapshot`, **then** all 16 fields are stored as member variables.
    - **Given** `DiagnosticsTelemetryPackage::write()` is called, **when** a `TelemetryWriter` serializes it, **then** the JSON output contains all 16 fields with correct key names.
    - **Given** a golden test for `DiagnosticsTelemetryPackage`, **when** `toJsonString()` is called, **then** the output matches the expected JSON format with envelope metadata and all 16 data fields.
- **Status**: ✓ Implemented (v1.2.0). See `DiagnosticsTelemetryPackage.h`, `DiagnosticsTelemetryPackage.cpp`, `tests/TelemetryPackage/test.cpp`.

### TS39: Implement Diagnostics Command Handler in Device

- **As an** IoT Developer, **I want** Device to handle the diagnostics command internally, **so that** my `handle()` override receives only custom commands without framework-internal logic.
- **Acceptance Criteria**:
    - **Given** Device has a diagnostics command ID constant (300), **when** `handleInternalCommand(Command)` receives a command with that ID, **then** `Device::collectDiagnostics()` is called and the result is wrapped in a `DiagnosticsTelemetryPackage`.
    - **Given** a gateway client is registered, **when** the diagnostics command is processed, **then** the `DiagnosticsTelemetryPackage` is enqueued via `enqueueTelemetryPayload()`.
    - **Given** no gateway client is registered, **when** the diagnostics command is processed, **then** a warning is logged and no telemetry is sent.
    - **Given** `handleInternalCommand()` returns true for framework commands, **when** `processPendingCommands()` runs, **then** `handle()` is not called for framework commands — only for developer-defined custom commands.
- **Status**: ✓ Implemented (v1.2.0). See `Device.h`, `Device.cpp`.

### TS40: Declare Per-Package Schema Version

- **As an** IoT Developer, **I want** each TelemetryPackage subclass to declare its own schema version, **so that** different telemetry packages can evolve independently without breaking each other.
- **Acceptance Criteria**:
    - **Given** a TelemetryPackage subclass declares a schema version constant, **when** the package is serialized, **then** the schema version is included in the JSON envelope metadata.
    - **Given** a TelemetryPackage subclass does not declare a schema version, **when** the package is serialized, **then** the schema version falls back to the global `Device::setTelemetrySchemaVersion()` value (if set).
    - **Given** a TelemetryPackage subclass declares schema version "1.0", **when** `toJsonString()` is called, **then** the JSON output includes `"schemaVersion": "1.0"`.
- **Status**: ✓ Implemented (v1.2.0). See `TelemetryPackage.h`, `DiagnosticsTelemetryPackage.h`, `SmartBandMqtt.ino`, `HomeSecurityDevice.ino`.

### TS41: Maintain Backward Compatibility with Global Schema Version

- **As an** IoT Developer, **I want** the global `Device::setTelemetrySchemaVersion()` to remain functional, **so that** existing code continues to work without modification.
- **Acceptance Criteria**:
    - **Given** `Device::setTelemetrySchemaVersion()` is called with a version string, **when** a TelemetryPackage without a per-package schema version is serialized, **then** the global schema version is used.
    - **Given** `Device::setTelemetrySchemaVersion()` is not called, **when** a TelemetryPackage without a per-package schema version is serialized, **then** no schema version is included in the output.
- **Status**: ✓ Implemented (v1.2.0). See `TelemetryPackage.h` (`setVersionMetadata()` fallback logic).

### TS42: Add Per-Package Schema Version to TelemetryPackage

- **As an** IoT Developer, **I want** TelemetryPackage to support per-package schema versions, **so that** subclasses can declare their own schema version.
- **Acceptance Criteria**:
    - **Given** TelemetryPackage has a protected `schemaVersionValue` member, **when** a subclass sets it, **then** the value is used in `addEnvelopeMetadata()`.
    - **Given** TelemetryPackage's `setVersionMetadata()` checks per-package schema first, **when** the per-package schema is set, **then** it's used instead of the global schema.
    - **Given** TelemetryPackage's `setVersionMetadata()` falls back to global schema, **when** the per-package schema is not set, **then** the global schema is applied.
- **Status**: ✓ Implemented (v1.2.0). See `TelemetryPackage.h`.

### TS43: Update Examples and Tests for Per-Package Schema Versions

- **As an** IoT Developer, **I want** examples to declare per-package schema versions, **so that** developers see the pattern in practice.
- **Acceptance Criteria**:
    - **Given** SmartBandTelemetryPackage declares `static constexpr const char* SCHEMA_VERSION = "1.0"`, **when** the package is serialized, **then** the JSON output includes `"schemaVersion": "1.0"`.
    - **Given** DiagnosticsTelemetryPackage declares `static constexpr const char* SCHEMA_VERSION = "1.0"`, **when** the package is serialized, **then** the JSON output includes `"schemaVersion": "1.0"`.
    - **Given** golden tests are updated, **when** tests run, **then** all tests pass with the new schema version in the output.
- **Status**: ✓ Implemented (v1.2.0). See `DiagnosticsTelemetryPackage.h`, `SmartBandMqtt.ino`, `HomeSecurityDevice.ino`, `tests/TelemetryPackage/test.cpp`.

### TS44: Define Composite Event Patterns

- **As an** IoT Developer, **I want** to define composite event patterns (e.g., "motion AND door opened within 5 seconds"), **so that** the device can detect complex automation triggers on-device.
- **Acceptance Criteria**:
    - **Given** an EventAggregator exists, **when** `addAllMatchPattern()` is called with two event identifiers and a time window, **then** the AND pattern is registered for matching.
    - **Given** a pattern is registered, **when** both event identifiers occur within the time window, **then** a composite event is emitted via the registered handler.
    - **Given** a pattern is registered, **when** only one event identifier occurs within the time window, **then** no composite event is emitted.
    - **Given** a pattern is registered, **when** both event identifiers occur but outside the time window, **then** no composite event is emitted.
    - **Given** an EventAggregator exists, **when** `addAnyMatchPattern()` is called with two event identifiers and a time window, **then** the OR pattern is registered — any single event triggers the composite event.
    - **Given** an OR pattern is registered, **when** only one of the event identifiers occurs, **then** the composite event is emitted immediately.
    - **Given** an EventAggregator exists, **when** `addAllMatchPattern()` is called with source IDs, **then** the pattern matches only events from the specified sensor sources, using `EventAggregator::ANY_SOURCE` for wildcard positions.
    - **Given** a conditional pattern is registered via `addAllMatchPattern()` with a `PatternCondition` callback, **when** the pattern matches but the condition returns false, **then** the composite event is suppressed.
- **Status**: ✓ Implemented (v1.2.0). See `EventAggregator.h`, `EventAggregator.cpp`.

### TS45: Event Aggregator as EventHandler

- **As an** IoT Developer, **I want** the EventAggregator to implement EventHandler, **so that** it can be registered as a sensor's handler to intercept events.
- **Acceptance Criteria**:
    - **Given** an EventAggregator implements EventHandler, **when** a sensor's `setHandler()` is called with the aggregator, **then** events from the sensor are passed to the aggregator's `on(Event)` method.
    - **Given** the EventAggregator receives an event, **when** the event matches a registered pattern, **then** the composite event is emitted via the output handler.
    - **Given** the EventAggregator receives an event, **when** no pattern matches, **then** the event is captured but no composite event is emitted.
    - **Given** two EventAggregator instances are chained via `setOutputHandler()`, **when** the first aggregator emits a composite event, **then** the second aggregator receives it and can match it against its own patterns.
    - **Given** a composite event is emitted, **when** the event is inspected, **then** its sourceId is `EventAggregator::DEVICE_SOURCE_ID` (0), distinguishing it from raw sensor events.
- **Status**: ✓ Implemented (v1.2.0). See `EventAggregator.h` (implements `EventHandler`).

### TS46: Implement EventAggregator Core

- **As an** IoT Developer, **I want** EventAggregator to capture events and match patterns, **so that** composite events are emitted when patterns match.
- **Acceptance Criteria**:
    - **Given** EventAggregator has a fixed-size circular buffer (32 captured events), **when** `on(Event)` is called, **then** the event is captured with its timestamp and sourceId.
    - **Given** EventAggregator has a fixed-size pattern array (4 patterns), **when** `addAllMatchPattern()` is called, **then** the AND pattern is stored.
    - **Given** EventAggregator has a fixed-size pattern array (4 patterns), **when** `addAnyMatchPattern()` is called, **then** the OR pattern is stored.
    - **Given** an event is captured, **when** `checkForEventPatternMatch()` runs, **then** all registered patterns are checked using their respective logic (AND requires all identifiers, OR requires any one).
    - **Given** a pattern matches, **when** the composite event is emitted, **then** the output handler's `on(Event)` is called with the composite event identifier and `EventAggregator::DEVICE_SOURCE_ID` as the source.
    - **Given** a pattern has a `PatternCondition` callback, **when** the pattern matches, **then** the condition function is called with the composite event ID before emitting.
    - **Given** a pattern is registered, **when** `removePattern()` is called with the composite event ID, **then** the pattern is removed and the slot is available for reuse.
    - **Given** multiple patterns are registered, **when** `cleanupPatterns()` is called, **then** all patterns are removed in one operation.
- **Status**: ✓ Implemented (v1.2.0). See `EventAggregator.cpp`.

### TS47: Captured Event Cleanup and Buffer Management

- **As an** IoT Developer, **I want** EventAggregator to remove expired captured events, **so that** the circular buffer doesn't grow unbounded.
- **Acceptance Criteria**:
    - **Given** captured events are older than the maximum pattern time window, **when** `removeExpiredCapturedEvents()` runs, **then** the expired captured events are removed.
    - **Given** the circular buffer is full, **when** a new event is captured, **then** the oldest captured event is evicted.
    - **Given** the pattern array is full, **when** `addAllMatchPattern()` or `addAnyMatchPattern()` is called, **then** the new pattern is rejected and false is returned.
    - **Given** a pattern is removed via `removePattern()`, **when** the pattern array is inspected, **then** the remaining patterns are compacted to fill the gap.
    - **Given** all patterns are removed via `cleanupPatterns()`, **when** a new pattern is registered, **then** the slot is available and the pattern is accepted.
- **Status**: ✓ Implemented (v1.2.0). See `EventAggregator.cpp`, `tests/EventAggregator/test.cpp`.

---

### TS48: Suppress Insignificant Telemetry Changes

- **As an** IoT Developer, **I want** a per-sensor deadband filter, **so that** stable measurements do not generate redundant telemetry.
- **Acceptance Criteria**:
    - **Given** a filter slot has no previous value, **when** its first valid reading arrives, **then** the reading passes.
    - **Given** a slot has a nonzero deadband, **when** a reading changes by more than that threshold, **then** it passes and becomes the new reference value.
    - **Given** a reading changes by less than or equal to the deadband, **when** it is checked, **then** it is suppressed.
    - **Given** a slot's deadband is zero, **when** a valid reading arrives, **then** filtering is disabled for that slot.
    - **Given** a floating-point reading is NaN or infinite, or the slot index is invalid, **when** the filter checks it, **then** the reading is suppressed without an out-of-bounds access.
- **Status**: ✓ Implemented (v1.3.0, W43). See `ChangeFilter.h`, `Device.h`, `tests/ChangeFilter/test.cpp`.

### TS49: Measure Motion and Vibration with an MPU6050

- **As an** IoT Developer, **I want** calibrated acceleration and angular-rate readings with windowed statistics, **so that** I can detect motion and vibration on-device.
- **Acceptance Criteria**:
    - **Given** an MPU6050 is connected over I²C, **when** `ImuSensor` initializes successfully, **then** it provides cached acceleration, angular-rate, and temperature readings.
    - **Given** samples are collected, **when** the configured sample window advances, **then** per-axis RMS and peak-to-peak statistics are available.
    - **Given** the sensor is not initialized or a read fails, **when** readings are requested, **then** the documented failure/default behavior is returned and no invalid sample is added.
- **Status**: ✓ Implemented (v1.3.0). See `ImuSensor.h`, `tests/ImuSensor/test.cpp`, and `examples/VibrationMonitor/`.

### TS50: Run Optional On-Device Inference

- **As an** IoT Developer, **I want** a uniform inference interface with an optional emlearn adapter, **so that** generated or trained models can run locally without requiring cloud inference.
- **Acceptance Criteria**:
    - **Given** a model is loaded, **when** prediction or regression is requested with valid features, **then** the result and inference status are reported.
    - **Given** no model is loaded or the input is invalid, **when** inference is requested, **then** the operation fails safely and `lastResult()` identifies the outcome.
    - **Given** an application uses a generated C model, **when** it builds with the framework, **then** the generated-model path does not require the emlearn package.
- **Status**: ✓ Implemented (v1.3.0, W47). See `AiMicroModelController.h`, `EmlearnController.h`, `tests/AiMicroModelController/test.cpp`.

### TS51: Dispatch Actions for Composite Events

- **As an** IoT Developer, **I want** rules to map composite event identifiers to actuator commands, **so that** common automation does not require a large `on(Event)` switch statement.
- **Acceptance Criteria**:
    - **Given** a rule is registered for a composite event, **when** the matching event arrives, **then** its registered actions are dispatched to their command handlers.
    - **Given** a rule contains multiple actions, **when** it matches, **then** each active action is dispatched in registration order, including any command payload.
    - **Given** the fixed rule or action capacity is full, **when** another entry is registered, **then** registration fails without overwriting existing entries.
    - **Given** a rule is removed or all rules are cleaned up, **when** a new rule is added, **then** the available slot can be reused.
- **Status**: ✓ Implemented (v1.3.0, W29). See `RulesEngine.h`, `tests/RulesEngine/test.cpp`, and `examples/SmartFactoryAlarm/`.

### TS52: Support BLE across ESP32 Arduino Core Generations

- **As an** IoT Developer, **I want** the same BLE NUS driver to work with ESP32 Arduino core 3.x and 4.x, **so that** BLE support does not force an upgrade to core 4.0.0.
- **Acceptance Criteria**:
    - **Given** an ESP32 board uses core 3.x or 4.x, **when** the BLE driver is built, **then** the HAL selects the compatible platform API.
    - **Given** the BLE driver is used by an application, **when** data is sent or received, **then** its NUS behavior remains independent of the underlying core API differences.
- **Status**: ✓ Implemented (v1.3.0). See `BleHal.h`, `BleHal.cpp`, `BleConnectivityDriver.h`, and `tests/BleConnectivityDriver/test.cpp`.

---

*Created: March 23, 2025.*
*Last Updated: November 24, 2026.*
