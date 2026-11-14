# Architecture Decision Records

Architecture Decision Records (ADRs) capture the key technical decisions made during the development of the Modest IoT Nano Framework, along with their context, trade-offs, and consequences.

Each ADR documents **why** a decision was made, not just **what** was decided. This enables future developers to understand the rationale, evaluate whether circumstances have changed, and make informed decisions about superseding or revisiting past choices.

## Overview

The Modest IoT Nano Framework is a lightweight, deterministic IoT framework for ESP32-based devices. It provides a Mediator design pattern for sensor-actuator wiring, a per-slot sampling scheduler, a telemetry pipeline with ownership safety, and connectivity drivers for Wi-Fi, BLE, and MQTT. The framework runs on FreeRTOS with two isolated tasks: a sampler task for all sensor/actuator I/O and a network task for asynchronous transmission.

The 30 ADRs below span six categories that shaped the framework's architecture:

| #                                                                                          | Title                                                         | Status   | Category     |
|--------------------------------------------------------------------------------------------|---------------------------------------------------------------|----------|--------------|
| [ADR-001](#adr-001-framework-naming-convention--modest_log_-macros-with-miot_log_-aliases) | Framework Naming Convention — `MODEST_LOG_*` Macros           | Accepted | Naming       |
| [ADR-002](#adr-002-ble-connectivity-via-nordic-uart-service-nus)                           | BLE Connectivity via Nordic UART Service (NUS)                | Accepted | Connectivity |
| [ADR-003](#adr-003-hybrid-approach-for-sensor-sampling-intervals)                          | Hybrid Approach for Sensor Sampling Intervals                 | Accepted | Scheduling   |
| [ADR-004](#adr-004-unique_ptr-ownership-for-telemetry-enqueue)                             | `unique_ptr` Ownership for Telemetry Enqueue                  | Accepted | Telemetry    |
| [ADR-005](#adr-005-drop-oldest-queue-policy)                                               | Drop-Oldest Queue Policy                                      | Accepted | Telemetry    |
| [ADR-006](#adr-006-bounded-serialization-buffer-in-mqttgatewayclient)                      | Bounded Serialization Buffer                                  | Accepted | Telemetry    |
| [ADR-007](#adr-007-envelope-metadata-in-telemetry-packages)                                | Envelope Metadata — Timestamp and Sequence                    | Accepted | Telemetry    |
| [ADR-008](#adr-008-serialize-overload-rename-to-prevent-name-hiding)                       | `serialize` Overload Rename — `toDocument` and `toJsonString` | Accepted | API Design   |
| [ADR-009](#adr-009-sensor-fault-events-with-debounced-threshold-state-machine)             | Sensor Fault Events — Debounced Threshold State Machine       | Accepted | Diagnostics  |
| [ADR-010](#adr-010-engineconfig-for-tunable-freertos-task-parameters)                      | `EngineConfig` — Configurable Task Stacks and Priorities      | Accepted | Threading    |
| [ADR-011](#adr-011-per-slot-samplingscheduler-with-countdown-based-dispatch)               | `SamplingScheduler` — Per-Slot Countdown Scheduling           | Accepted | Scheduling   |
| [ADR-012](#adr-012-arduinojson-retained-for-telemetry-serialization)                       | Retain ArduinoJson for Telemetry Serialization                | Accepted | Dependencies |
| [ADR-013](#adr-013-screaming_snake_case-for-all-preprocessor-macros)                       | `SCREAMING_SNAKE_CASE` Macro Convention                       | Accepted | Naming       |
| [ADR-014](#adr-014-threading-model--single-context-sampler-with-isolated-network-task)     | Threading Model — Single-Context Sampler                      | Accepted | Threading    |
| [ADR-015](#adr-015-identifier-registry-with-reserved-ranges)                               | Identifier Registry with Reserved Ranges                      | Accepted | API Design   |
| [ADR-016](#adr-016-dependency-hygiene-principles)                                          | Dependency Hygiene Principles                                 | Accepted | Dependencies |
| [ADR-017](#adr-017-shared-mock-harness-for-host-unit-tests)                                | Shared Mock Harness for Host Unit Tests                       | Accepted | Testing      |
| [ADR-018](#adr-018-diagnosticssnapshot--on-demand-stack-and-heap-metrics)                  | `DiagnosticsSnapshot` — On-Demand Stack and Heap Metrics      | Accepted | Diagnostics  |
| [ADR-019](#adr-019-mqtt-reconnect-policy--rate-limited-backoff-with-continuous-keep-alive) | MQTT Reconnect Policy — Rate-Limited Backoff                  | Accepted | Connectivity |
| [ADR-020](#adr-020-stopasynchronousengine--cooperative-shutdown-via-notification-bits)     | `stopAsynchronousEngine` — Cooperative Shutdown               | Accepted | Threading    |
| [ADR-021](#adr-021-golden-host-tests-for-telemetry-json-serialization)                     | Golden Host Tests for Telemetry JSON Serialization            | Accepted | Testing      |
| [ADR-022](#adr-022-notification-redesign-for-interrupt-driven-sensors-n1)                  | Notification Redesign for Interrupt-Driven Sensors (N1)       | Accepted | Threading    |
| [ADR-023](#adr-023-domain-driven-design-adoption-and-bounded-contexts)                     | Domain-Driven Design Adoption and Bounded Contexts            | Accepted | Architecture |
| [ADR-024](#adr-024-command-marshaling-for-cross-task-safety-w12)                           | Command Marshaling for Cross-Task Safety (W12)                | Accepted | Threading    |
| [ADR-025](#adr-025-non-blocking-sensor-protocols-w14)                                      | Non-Blocking Sensor Protocols (W14)                           | Accepted | Scheduling   |
| [ADR-026](#adr-026-cloud-to-device-mqtt-subscribe-w15)                                     | Cloud-to-Device MQTT Subscribe (W15)                          | Accepted | Connectivity |
| [ADR-027](#adr-027-tickless-scheduling-and-light-sleep-w16)                                | Tickless Scheduling and Light Sleep (W16)                     | Accepted | Power        |
| [ADR-028](#adr-028-deep-sleep-cycle-and-battery-example-w17)                               | Deep-Sleep Cycle and Battery Example (W17)                    | Accepted | Power        |
 | [ADR-029](#adr-029-native-servo-and-lcd-drivers-w19)                                       | Native Servo and LCD Drivers (W19)                            | Accepted | Dependencies |
 | [ADR-030](#adr-030-bosch-environmental-sensor-base-class)                                   | Bosch Environmental Sensor Base Class                         | Accepted | Refactoring    |

### Status Legend

- **Accepted** — The decision has been implemented and is in effect.
- **Proposed** — The decision is documented but not yet implemented; it is a prerequisite for future work.
- **Superseded** — The decision has been replaced by a newer ADR (none yet).

### Category Summary

| Category         | Count | Key Decisions                                                                                        |
|------------------|-------|------------------------------------------------------------------------------------------------------|
| **Naming**       | 2     | `MODEST_LOG_*` macros with `MIOT_LOG_*` aliases; `SCREAMING_SNAKE_CASE` convention                   |
| **Connectivity** | 3     | BLE via NUS GATT profile; MQTT reconnect; cloud-to-device MQTT subscribe                            |
| **Scheduling**   | 3     | Hybrid sensor intervals with virtual getters; per-slot countdown scheduler; non-blocking sensors     |
| **Telemetry**    | 4     | `unique_ptr` ownership; drop-oldest policy; bounded buffer; envelope metadata                        |
| **Diagnostics**  | 2     | Sensor fault events with debounced threshold; on-demand `DiagnosticsSnapshot`                        |
| **Threading**    | 5     | Single-context sampler model; `EngineConfig`; cooperative shutdown; notification redesign; command marshaling |
| **API Design**   | 2     | `serialize` overload rename; identifier registry with reserved ranges                                |
| **Dependencies** | 3     | Retain ArduinoJson; five hygiene principles for dependency decisions; native servo and LCD drivers |
| **Testing**      | 2     | Shared mock harness; golden host tests for JSON serialization                                        |
| **Architecture** | 1     | DDD adoption, bounded contexts, ubiquitous language, embedded-friendly subset                        |
| **Power**        | 3     | Tickless scheduling with light sleep; deep sleep with RTC memory; native drivers for battery operation |

---

<a id="adr-001-framework-naming-convention--modest_log_-macros-with-miot_log_-aliases"></a>
## ADR-001: Framework Naming Convention — `MODEST_LOG_*` Macros with `MIOT_LOG_*` Aliases

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

The framework needed a standardized logging facility. Three naming alternatives were considered:
- `MODEST_LOG_*` — Full framework prefix, self-documenting, zero collision risk.
- `MODEST_LOGGING_*` — Longer, semantically ambiguous when paired with severity levels.
- `MIOT_LOG_*` — Compact, but collides with Xiaomi IoT (MIoT) ecosystem namespace.

### Decision

Use `MODEST_LOG_*` as the official macro family (`MODEST_LOG_E`, `MODEST_LOG_W`, `MODEST_LOG_I`, `MODEST_LOG_D`, `MODEST_LOG_V`). Provide optional `MIOT_LOG_*` shorthand aliases via `#if defined(MODEST_USE_SHORT_LOG_MACROS)` for developers who prefer compact call sites.

### Consequences

- **Positive**: Zero namespace collision risk, aligns with `Modest IoT` branding (`ModestIoT.h` umbrella header), follows C/C++ `SCREAMING_SNAKE_CASE` macro convention, complies with Clang-Tidy and MISRA rules.
- **Negative**: 2 characters longer than `MIOT_LOG_*` per call site.
- **Rationale**: For the cost of 2 extra characters, the framework eliminates namespace collisions with Xiaomi IoT and maintains complete brand consistency. The alias provides an opt-in ergonomic path.

### References

- `src/ModestLog.h`
- `tests/ModestLog/test.cpp`
- Evaluation discussion in session history

---

<a id="adr-002-ble-connectivity-via-nordic-uart-service-nus"></a>
## ADR-002: BLE Connectivity via Nordic UART Service (NUS)

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

`BluetoothSerial` (Bluetooth Classic BR/EDR SPP) is deprecated in Arduino-ESP32 Core 3.x and unsupported on modern ESP32 variants (S3, C3, C6, H2). The framework needed a replacement serial transport for point-to-point telemetry.

Four alternatives were evaluated:
1. **BLE NUS via NimBLE** — Universal SoC support, ~25-35 KB SRAM, mobile/web interoperability.
2. **BLE NUS via Bluedroid** — Universal SoC support, ~80-120 KB SRAM, heavy footprint.
3. **ESP-NOW** — Ultra-lightweight (<4 KB), ESP32-to-ESP32 only, no phone/web support.
4. **Pure deprecation** — Remove Bluetooth entirely, no replacement.

### Decision

Implement `BleConnectivityDriver` using the Nordic UART Service (NUS) GATT profile with 128-bit UUIDs (`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`). The legacy `BluetoothConnectivityDriver` was initially isolated behind preprocessor guards and marked `[[deprecated]]`, then fully removed as Arduino-ESP32 Core 4.0.0 drops `BluetoothSerial` support and modern ESP32 variants (S3, C3, C6, H2) lack Bluetooth Classic hardware entirely.

### Consequences

- **Positive**: Runs on all ESP32 variants (original, S3, C3, C6, H2), direct serial equivalence to `BluetoothSerial`, interoperable with nRF Connect, Web Bluetooth API, and mobile apps.
- **Negative**: Requires NimBLE or BLE stack enabled in ESP-IDF configuration.
- **Rationale**: NUS provides the highest net utility: universal compatibility, low memory footprint, and ecosystem interoperability while preserving the `ConnectivityDriver` contract.

### References

- `src/BleConnectivityDriver.h`, `src/BleConnectivityDriver.cpp`
- `tests/BleConnectivityDriver/test.cpp`
- `docs/improvements.md`

---

<a id="adr-003-hybrid-approach-for-sensor-sampling-intervals"></a>
## ADR-003: Hybrid Approach for Sensor Sampling Intervals

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

The `SamplingScheduler` needed a way to resolve per-sensor sampling periods. The legacy two-argument `appendSensorToScheduler(EventHandler*, int)` overload passed period 0 (every tick), ignoring sensor-specific requirements.

Four alternatives were evaluated:
1. **Virtual getters only** — Zero memory overhead, but no runtime per-instance tuning.
2. **Base attributes with virtual defaults (Hybrid)** — 8 bytes per instance, runtime tuning, polymorphic resolution.
3. **Template-based resolution** — Zero runtime overhead, requires `public` constants, no base pointer support.
4. **Key-value builder** — Runtime flexibility, but high memory overhead and no compile-time type safety.

### Decision

Implement the Hybrid approach: add two `protected` attributes (`samplingIntervalInMilliseconds`, `minimalSamplingIntervalInMilliseconds`) defaulting to 0, two pure virtual methods (`getDefaultSamplingIntervalInMilliseconds`, `getMinimalSamplingIntervalInMilliseconds`), and four non-virtual getter/setter methods that combine attribute overrides with virtual defaults.

### Consequences

- **Positive**: Enables `appendSensorToScheduler(Sensor*, int)` to query periods polymorphically, allows per-instance runtime tuning, maintains backward compatibility.
- **Negative**: 8 bytes per sensor instance (negligible on ESP32 with ~320 KB SRAM), requires virtual method implementations in all 10 sensor subclasses.
- **Rationale**: Combines polymorphic resolution with runtime flexibility, keeping memory overhead negligible. The `Sensor*` overload naturally takes compiler precedence for Sensor arguments, while the `EventHandler*` overload remains for non-Sensor handlers.

### References

- `src/Sensor.h`, `src/Sensor.cpp`
- `src/Device.h`, `src/Device.cpp` (new `Sensor*` overload)
- `docs/improvements.md`

---

<a id="adr-004-unique_ptr-ownership-for-telemetry-enqueue"></a>
## ADR-004: `unique_ptr` Ownership for Telemetry Enqueue

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

The telemetry pipeline used raw `new`/`delete` across two FreeRTOS tasks. The sampler task allocated `TelemetryPackage` objects, passed raw pointers through the queue, and the network task deleted them. If enqueue failed, the caller had to remember to delete the packet.

Three alternatives were evaluated:
1. **`unique_ptr` overload** — Ownership transfer on success, automatic deletion on failure. Additive, no breaking changes.
2. **Packet pool with placement-new** — Eliminates heap allocation entirely, but high complexity and requires measuring heap behavior first.
3. **Keep raw pointers** — Simple, but error-prone and causes heap fragmentation over long uptimes.

### Decision

Add `bool enqueueTelemetryPayload(std::unique_ptr<TelemetryPackage>)` as the recommended path. Keep the legacy raw-pointer overload for backward compatibility.

### Consequences

- **Positive**: Eliminates the most common error pattern (forgetting to `delete` on enqueue failure), zero breaking changes, immediate ownership safety.
- **Negative**: The `new` still happens on the heap (moved to `make_unique`); allocator lock contention remains until a packet pool is implemented.
- **Rationale**: Provides immediate safety with zero breaking changes. The packet pool (deferred) can be added later when heap measurements justify the architectural investment.

### References

- `src/Device.h`, `src/Device.cpp`
- `examples/SmartBandMqtt/SmartBandMqtt.ino`
- `examples/AirQualityMonitor/AirQualityMonitor.ino`
- `docs/improvements.md`

---

<a id="adr-005-drop-oldest-queue-policy"></a>
## ADR-005: Drop-Oldest Queue Policy

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

When the telemetry queue is full, the zero-timeout `xQueueSend` silently drops the newest packet. Under network outage, the oldest (stalest) data is transmitted first, and the freshest readings are lost.

Two alternatives were evaluated:
1. **Drop-oldest** — Receive and delete the oldest packet, then insert the new one. Keeps freshest data.
2. **Drop-newest** — Keep the current behavior. Preserves FIFO order but loses recent data.

### Decision

Implement drop-oldest: when the queue is full, receive and delete the oldest packet, then retry sending the new packet.

### Consequences

- **Positive**: Ensures the freshest telemetry data is always transmitted. Under network outage, the queue retains the most recent readings.
- **Negative**: Breaks strict FIFO ordering. The network task may race to consume the oldest item, which is acceptable.
- **Rationale**: In IoT telemetry, the latest reading is typically more valuable than historical data. Drop-oldest aligns with the framework's goal of providing current sensor state.

### References

- `src/Device.cpp` (`enqueueTelemetryPayload` overloads)
- `src/Device.h` (`packetsDroppedOldest` field in `DiagnosticsSnapshot`)
- `docs/improvements.md`

---

<a id="adr-006-bounded-serialization-buffer-in-mqttgatewayclient"></a>
## ADR-006: Bounded Serialization Buffer in MqttGatewayClient

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

`MqttGatewayClient::sendTelemetryRecord()` used a three-stage JSON construction: `serializeAsString()` → `JsonDocument` → `String` → `c_str()`. Each stage allocated heap memory, causing fragmentation on the network task.

Two alternatives were evaluated:
1. **Bounded buffer** — Fixed 512-byte stack buffer, direct `serializeJson(doc, buffer, sizeof(buffer))`. Eliminates `String` allocation.
2. **Keep `String` allocation** — Simple, but causes heap fragmentation.

### Decision

Replace `serializeAsString()` with a fixed 512-byte `serializationBuffer` member and direct `serializeJson(doc, buffer, sizeof(buffer))`. Log a `PayloadTooLarge` warning when serialization exceeds the buffer.

### Consequences

- **Positive**: Eliminates the intermediate `String` allocation, reducing heap fragmentation on the network task. Bounded memory usage.
- **Negative**: 512-byte limit may be insufficient for very large payloads. The `JsonDocument` heap allocation remains.
- **Rationale**: Most telemetry packages (3-5 scalar fields) produce payloads under 200 bytes. The 512-byte buffer provides ample headroom while bounding memory usage. Full heap-stability requires the `TelemetryWriter` abstraction (deferred).

### References

- `src/MqttGatewayClient.h`, `src/MqttGatewayClient.cpp`
- `docs/improvements.md`

---

<a id="adr-007-envelope-metadata-in-telemetry-packages"></a>
## ADR-007: Envelope Metadata in Telemetry Packages

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

Telemetry packages had no timestamp or sequence number. Delayed messages lost their true sample time, and dropped packets were invisible to the backend.

Two alternatives were evaluated:
1. **Envelope metadata** — Add `timestamp` (millis from boot) and `sequence` (monotonically increasing) fields to every package.
2. **No metadata** — Keep payloads minimal; let the backend assign timestamps.

### Decision

Add `captureTimestamp` and `sequenceNumber` protected fields to `TelemetryPackage`. Assign sequence before enqueue attempt so drops consume a sequence number, making drops visible as gaps.

### Consequences

- **Positive**: Preserves sample time for delayed messages, makes drops observable as sequence gaps, enables proper time-series analysis on the backend.
- **Negative**: Adds 8 bytes per package (two `uint32_t`). JSON output is slightly larger.
- **Rationale**: The observability benefit outweighs the minimal overhead. Sequence gaps make the drop-oldest policy visible, and timestamps enable correct time-series alignment.

### References

- `src/TelemetryPackage.h`, `src/TelemetryPackage.cpp`
- `src/Device.cpp` (`nextSequenceNumber` counter)
- `docs/improvements.md`

---

<a id="adr-008-serialize-overload-rename-to-prevent-name-hiding"></a>
## ADR-008: `serialize` Overload Rename to Prevent Name Hiding

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

`TelemetryPackage` declared two convenience overloads: `serialize()` (returns `JsonDocument`) and `serializeAsString()` (returns `String`). When a subclass overrides `serialize(JsonDocument&)`, it hides both base class convenience methods — a silent C++ gotcha.

Two alternatives were evaluated:
1. **Rename convenience overloads** — `serialize()` → `toDocument()`, `serializeAsString()` → `toJsonString()`.
2. **Keep names, use `using` declarations** — Require subclasses to add `using TelemetryPackage::serialize;` — error-prone.

### Decision

Rename the convenience overloads to `toDocument()` and `toJsonString()` so they can no longer be hidden by the subclass's `serialize` override.

### Consequences

- **Positive**: Prevents future bugs where subclasses inadvertently hide the base class methods. Zero breaking changes for calls through base references (as gateways do).
- **Negative**: Requires updating call sites that use the convenience overloads directly.
- **Rationale**: The pure virtual `serialize(JsonDocument&)` remains the contract that subclasses implement. Renaming the convenience overloads eliminates the name collision entirely.

### References

- `src/TelemetryPackage.h`, `src/TelemetryPackage.cpp`
- `src/HttpGatewayClient.cpp`
- `docs/improvements.md`

---

<a id="adr-009-sensor-fault-events-with-debounced-threshold-state-machine"></a>
## ADR-009: Sensor Fault Events with Debounced Threshold State Machine

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

Sensor read failures (DHT NaN, HX711 timeout, ultrasonic echo timeout, I²C bus errors) silently returned `-1`. The mediator received no notification when a sensor disconnected or failed in the field.

Three alternatives were evaluated:
1. **Debounced threshold state machine** — Fault events emit exactly once when consecutive failures reach a configurable threshold. Recovery events emit exactly once on the next successful read. Default threshold is 0 (disabled).
2. **Error bitmask in TelemetryPackage** — Includes health status in cloud payloads, but local logic cannot react immediately.
3. **Callback interface** — Bypasses the event queue, violates the Mediator design pattern.

### Decision

Implement the debounced threshold state machine in the `Sensor` base class with `reportReadFailure()`, `reportReadSuccess()`, and `setFaultThreshold()` helpers. Reserved event IDs 22 (`SENSOR_FAULT_EVENT_IDENTIFIER`) and 23 (`SENSOR_RECOVERED_EVENT_IDENTIFIER`).

### Consequences

- **Positive**: Prevents event storms (does not flood queues on every failed poll), zero breaking changes (disabled by default), pure state logic easily tested.
- **Negative**: Requires converting each sensor to call the helpers. Default threshold of 0 means existing sketches are unaffected.
- **Rationale**: The state machine provides the best balance: it prevents event flooding via latching, maintains clean mediator decoupling, and is 100% backward compatible.

### References

- `src/Sensor.h`, `src/Sensor.cpp`
- `tests/SensorFault/test.cpp`
- `examples/OutdoorSecurityLightController/OutdoorSecurityLightController.ino`
- `docs/improvements.md`

---

<a id="adr-010-engineconfig-for-tunable-freertos-task-parameters"></a>
## ADR-010: EngineConfig for Tunable FreeRTOS Task Parameters

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

FreeRTOS task stack sizes, priorities, and core pinning were hard-coded in `Device::initializeAsynchronousEngine()`. Developers had no way to tune these parameters without modifying framework source code.

Two alternatives were evaluated:
1. **`EngineConfig` struct** — Configurable stack sizes, priorities, core pinning, and queue depth. Defaults match original hard-coded values.
2. **Individual constructor parameters** — Clutters the constructor signature, hard to remember parameter order.

### Decision

Add `EngineConfig` struct with `samplerStackBytes`, `networkStackBytes`, `samplerPriority`, `networkPriority`, `samplerCore`, `networkCore`, and `telemetryQueueDepth`. Provide `initializeAsynchronousEngine(const EngineConfig&)` overload; the existing single-argument form delegates for backward compatibility.

### Consequences

- **Positive**: Makes stack sizes, priorities, and core pinning configurable from sketches. Enables `stopAsynchronousEngine()` for graceful shutdown. Defaults preserve backward compatibility.
- **Negative**: Adds one struct to the public API. Developers must understand FreeRTOS to tune parameters correctly.
- **Rationale**: The struct groups related parameters logically, provides sensible defaults, and enables the `DiagnosticsSnapshot` stack high-water marks to be meaningful (developers can measure and adjust).

### References

- `src/Device.h`, `src/Device.cpp`
- `tests/EngineConfig/test.cpp`
- `docs/improvements.md`

---

<a id="adr-011-per-slot-samplingscheduler-with-countdown-based-dispatch"></a>
## ADR-011: Per-Slot SamplingScheduler with Countdown-Based Dispatch

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

`Device` used fixed-tick round-robin dispatch across 8 slots. Every registered sensor was polled on every timer tick, regardless of the sensor's declared sampling requirements.

Two alternatives were evaluated:
1. **Per-slot countdown scheduler** — Each slot tracks its own period, phase, and elapsed ticks. `advance(elapsed)` handles accumulated tick counts from `ulTaskNotifyTake`.
2. **Per-sensor timers** — Each sensor gets its own hardware timer. Complex, requires multiple timer ISRs.

### Decision

Implement `SamplingScheduler` with per-slot periods, staggered initial phases, elapsed tick handling, phase-aligned re-arming, and minimum period enforcement with clamping reports.

### Consequences

- **Positive**: Sensors poll at their declared intervals, reducing CPU overhead. DHT22 polls every 2000ms instead of every tick. Phase alignment prevents thundering herd.
- **Negative**: Adds one class to the framework. Requires sensor interval constants.
- **Rationale**: The countdown scheduler is pure C++ with no hardware dependencies, easily testable in host unit tests, and integrates cleanly with the existing timer ISR and notification mechanism.

### References

- `src/SamplingScheduler.h`, `src/SamplingScheduler.cpp`
- `src/Device.cpp` (replaces `activeSequencePointer` round-robin)
- `tests/SamplingScheduler/test.cpp`
- `docs/improvements.md`

---

<a id="adr-012-arduinojson-retained-for-telemetry-serialization"></a>
## ADR-012: ArduinoJson Retained for Telemetry Serialization

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

The framework uses ArduinoJson exclusively for JSON serialization in the telemetry pipeline. For the current use case (3-5 simple scalar fields to a flat JSON object), manual `snprintf` would produce the same output with less Flash/RAM overhead.

Two alternatives were evaluated:
1. **Keep ArduinoJson** — Type safety, automatic escaping, future extensibility for nested objects/arrays and deserialization.
2. **Replace with `snprintf`** — Zero dependency overhead, but manual format strings and no automatic escaping.

### Decision

Retain ArduinoJson (pinned to `>=7.0.0` in `library.properties`) for now. Defer the `TelemetryWriter` abstraction (Phase C in `improvements.md`) until measured on hardware.

### Consequences

- **Positive**: Type safety via `operator[]`, automatic string escaping, supports nested objects and arrays if needed, enables deserialization for MQTT subscribe commands.
- **Negative**: ~30-40 KB Flash footprint, `JsonDocument` heap allocation per serialization.
- **Rationale**: The framework's usage is lightweight (3 features out of many), and the dependency is properly pinned and cleaned up. The `TelemetryWriter` abstraction can decouple serialization later if needed.

### References

- `src/TelemetryPackage.h`
- `library.properties`
- `docs/improvements.md`

---

<a id="adr-013-screaming_snake_case-for-all-preprocessor-macros"></a>
## ADR-013: SCREAMING_SNAKE_CASE for All Preprocessor Macros

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

The framework needed a consistent naming convention for preprocessor macros. CamelCase/PascalCase macros were considered for aesthetic consistency with C++ class/method naming.

Two alternatives were evaluated:
1. **`SCREAMING_SNAKE_CASE`** — Traditional C/C++ convention, zero collision risk, complies with Clang-Tidy and MISRA.
2. **CamelCase/PascalCase** — Blends with C++ naming, but collides with types/functions, triggers linter violations, and camouflages macros as type-safe functions.

### Decision

Enforce `SCREAMING_SNAKE_CASE` for all preprocessor macros. Use CamelCase/PascalCase strictly for first-class C++ entities (types, functions, variables).

### Consequences

- **Positive**: Virtually zero namespace collision risk, immediate cognitive transparency (signals non-hygienic preprocessor substitution), zero linter friction.
- **Negative**: Visually distinct from surrounding C++ code.
- **Rationale**: The preprocessor operates on raw textual substitution before lexical analysis. `ALL_CAPS` creates a distinct namespace partition that eliminates accidental collisions with standard C++ identifiers.

### References

- `src/ModestLog.h` (all `MODEST_LOG_*` macros)
- All header guard macros (`MODEST_IOT_H`, `DEVICE_H`, etc.)

---

<a id="adr-014-threading-model--single-context-sampler-with-isolated-network-task"></a>
## ADR-014: Threading Model — Single-Context Sampler with Isolated Network Task

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

The framework runs on ESP32 with FreeRTOS. Sensors and actuators are not thread-safe, and calling them from multiple tasks introduces race conditions. The telemetry pipeline spans two tasks: a sampler task (sensing) and a network task (transmission).

Two alternatives were evaluated:
1. **Single-context sampler** — All sensor reads and actuator writes happen on one task. A queue bridges to the network task for async transmission.
2. **Lock-based multi-context** — Allow any task to call any sensor/actuator, protected by mutexes.

### Decision

Enforce a single-context sampler model: all `Sensor::on()` and `Actuator::handle()` calls run exclusively on the sampler task. The network task handles only I/O (Wi-Fi, MQTT, BLE). Cross-task communication uses FreeRTOS queues and notifications.

Five threading rules were established:
1. Sensors and actuators run only on the sampler task.
2. Network I/O runs only on the network task.
3. The timer ISR wakes the sampler task; it does not call application code.
4. `Device::post()` (future) marshals commands from network to sampler.
5. No locks in sensors or actuators.

`@runs_on` Doxygen tags document which context each public hook executes in.

### Consequences

- **Positive**: Eliminates race conditions in sensors/actuators without mutexes. Predictable execution context for debugging. Network latency does not block sensing.
- **Negative**: Cloud-to-device commands require `Device::post` marshaling (W12). Developers must understand the two-task model.
- **Rationale**: The single-context model is simpler, safer, and matches the embedded IoT use case where sensors are polled and actuators are commanded sequentially.

### References

- `src/Device.h`, `src/Device.cpp`
- `README.md` (Threading Model section)
- `docs/improvements.md`

---

<a id="adr-015-identifier-registry-with-reserved-ranges"></a>
## ADR-015: Identifier Registry with Reserved Ranges

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

The framework uses integer identifiers for events and commands. Without a registry, user-defined identifiers could collide with framework-internal ones, causing silent misrouting.

Two alternatives were evaluated:
1. **Centralized registry** — Framework reserves ranges (0-999), users use 1000+. Reserved blocks for future framework additions.
2. **String-based identifiers** — Zero collision risk, but higher memory and comparison cost.

### Decision

Maintain a centralized identifier registry with reserved ranges:
- Framework events: 0-99 (currently 10, 20-23, 30-31, 40, 50-51, 60-61, 70-73)
- Framework commands: 100-399 (currently 100-102, 150-152, 200-202, 250-252, 300-302, 304-307)
- Reserved blocks: sensor-health 22-23, connectivity link events 900-909, power events 910-919
- User identifiers: 1000+

The registry is documented in the README and in `docs/improvements.md`.

### Consequences

- **Positive**: Zero collision risk between framework and user code. Future framework additions have reserved space. Integer comparison is fast and low-memory.
- **Negative**: Requires documentation discipline. New framework identifiers must check the registry first.
- **Rationale**: Integer identifiers are standard in embedded systems. The registry approach provides safety without the overhead of string-based identifiers.

### References

- `README.md` (Identifier Registry section)
- `docs/improvements.md` (W4)
- `src/Event.h`, `src/Command.h`

---

<a id="adr-016-dependency-hygiene-principles"></a>
## ADR-016: Dependency Hygiene Principles

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

The framework depends on third-party Arduino libraries (DHT, MQTT, ArduinoJson, etc.). Each dependency increases install friction, Flash/RAM footprint, and potential API breakage.

Five principles were established to govern dependency decisions:
1. Remove what is unused first. It costs nothing and cuts install friction.
2. Replace only simple, well-specified protocols, where a native implementation is a few dozen lines.
3. Never replace protocols with timing risk or large state machines (DHT, MQTT, TLS, JSON).
4. Always wrap before replacing. Introduce a small internal interface first.
5. Library Manager friction counts. Each extra `depends=` entry is auto-installed for every user.

### Decision

Apply the five principles to all dependency decisions. Delete `LoadCellController` and remove `HX711_Arduino_Library`, `EspSoftwareSerial`, and `Adafruit_Unified_Sensor` from `library.properties`. Pin `ArduinoJson (>=7.0.0)`.

### Consequences

- **Positive**: Reduced install friction, smaller footprint, no unnecessary dependencies. Clear criteria for future dependency decisions.
- **Negative**: Native HX711 implementation in `LoadCellAmplifier` requires maintenance.
- **Rationale**: The principles balance developer experience with framework autonomy. Simple protocols (HX711) are replaced; complex ones (DHT, MQTT, JSON) are retained.

### References

- `library.properties`
- `src/LoadCellAmplifier.h`, `src/LoadCellAmplifier.cpp`
- `docs/improvements.md` (W2)

---

<a id="adr-017-shared-mock-harness-for-host-unit-tests"></a>
## ADR-017: Shared Mock Harness for Host Unit Tests

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

The framework needed host unit tests that compile without Arduino/ESP32 toolchains. Initially, each test suite maintained its own local `fakes/` directory with duplicate mock headers.

Two alternatives were evaluated:
1. **Centralized `tests/fakes/`** — Single shared mock harness for all test suites.
2. **Per-suite fakes** — Each test suite maintains its own mocks.

### Decision

Consolidate all mocks into a centralized `tests/fakes/` directory with shared headers: `Arduino.h`, `Wire.h`, `SPI.h`, `BLEDevice.h`, `BLEServer.h`, `BLEUtils.h`, `MQTT.h`, `WiFi.h`, `ArduinoJson.h`, `Client.h`. All test suites include from the shared location.

### Consequences

- **Positive**: Single source of truth for mocks. Changes to a mock (e.g., adding a new API) benefit all test suites. Eliminates duplicate code.
- **Negative**: All tests share the same mock interface; test-specific mocks require conditional compilation.
- **Rationale**: With 33 test suites, the shared harness eliminates ~33× duplicate mock code and ensures consistent behavior across all tests.

### References

- `tests/fakes/`
- `tests/run_all.sh`
- `CMakeLists.txt`
- `docs/improvements.md` (W5)

---

<a id="adr-018-diagnosticssnapshot--on-demand-stack-and-heap-metrics"></a>
## ADR-018: DiagnosticsSnapshot — On-Demand Stack and Heap Metrics

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

The framework runs two FreeRTOS tasks with configurable stack sizes. Developers need visibility into stack consumption, heap usage, and queue behavior to tune `EngineConfig` parameters and detect issues.

Three alternatives were evaluated:
1. **On-demand snapshot** — Query metrics only when `Device::diagnostics()` is called. Zero background overhead.
2. **Continuous background monitor** — Periodically check and log warnings. Background CPU cost.
3. **FreeRTOS overflow hook only** — Catch fatal overflows only. No proactive visibility.

### Decision

Implement `DiagnosticsSnapshot` as an on-demand 12-field struct queried via `Device::diagnostics() const`. Fields include: missedTicks, worstCaseWakeMicros, packetsEnqueued, packetsDropped, queueHighWaterMark, sendsSucceeded, sendsFailed, freeHeapBytes, minFreeHeapBytes, samplerStackFreeWords, networkStackFreeWords, uptimeSeconds.

Stack high-water marks are queried via `uxTaskGetStackHighWaterMark()` and converted from words to bytes. Heap stats from `ESP.getFreeHeap()` and `ESP.getMinFreeHeap()`.

### Consequences

- **Positive**: Zero background CPU overhead during normal execution. All metrics available when needed. Enables `EngineConfig` tuning based on measured data.
- **Negative**: Metrics are point-in-time snapshots; transient issues may be missed without continuous monitoring.
- **Rationale**: On-demand diagnostics provide the best balance: developers get full visibility when they need it, without any runtime cost during normal operation.

### References

- `src/Device.h`, `src/Device.cpp`
- `src/GatewayClient.h`
- `tests/DiagnosticsSnapshot/test.cpp`
- `docs/improvements.md` (W6)

---

<a id="adr-019-mqtt-reconnect-policy--rate-limited-backoff-with-continuous-keep-alive"></a>
## ADR-019: MQTT Reconnect Policy — Rate-Limited Backoff with Continuous Keep-Alive

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

MQTT connections can drop due to network outages, broker restarts, or keep-alive timeouts. Without reconnection logic, the device becomes permanently disconnected. Without keep-alive servicing, the broker disconnects idle clients.

Three alternatives were evaluated:
1. **Rate-limited backoff + continuous keep-alive** — Non-blocking reconnect with configurable interval. `mqttClient.loop()` runs continuously via `service()`.
2. **Immediate reconnect** — Aggressive reconnection on every disconnect. Risks broker overload and power waste.
3. **No reconnect** — Application handles reconnection. Inconsistent behavior across sketches.

### Decision

Implement non-blocking reconnection with configurable backoff (`reconnectIntervalMilliseconds`, default 5000 ms) in `MqttGatewayClient`. Add `service()` method for continuous keep-alive servicing. The network worker polls with bounded timeout (`pdMS_TO_TICKS(100)`) and calls `maintainNetworkServices()` for background maintenance.

`WiFiConnectivityDriver` uses re-arming retry logic with configurable reconnect intervals to prevent permanent lockouts.

### Consequences

- **Positive**: Device recovers from transient network outages automatically. Keep-alive prevents broker disconnections during idle periods. Configurable backoff prevents broker overload.
- **Negative**: Adds reconnection state machine complexity. Bounded timeout means up to 100 ms latency for queue processing.
- **Rationale**: Rate-limited backoff is the standard pattern for IoT reconnection. Continuous keep-alive ensures the connection stays alive even when no telemetry is being sent.

### References

- `src/MqttGatewayClient.h`, `src/MqttGatewayClient.cpp`
- `src/WiFiConnectivityDriver.cpp`
- `src/Device.cpp` (`maintainNetworkServices`)
- `tests/MqttGatewayClient/test.cpp`
- `docs/improvements.md` (W7)

---

<a id="adr-020-stopasynchronousengine--cooperative-shutdown-via-notification-bits"></a>
## ADR-020: stopAsynchronousEngine — Cooperative Shutdown via Notification Bits

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

The framework starts two FreeRTOS tasks and a hardware timer. Without a shutdown mechanism, tasks run indefinitely, preventing clean resets and causing resource leaks.

Two alternatives were evaluated:
1. **Cooperative shutdown via notification bits** — Set a stop bit, tasks check and exit cleanly. Timer cleanup, queue drain.
2. **Forced task deletion** — `vTaskDelete()` from outside. Risk of tasks holding resources.

### Decision

Implement `stopAsynchronousEngine()` with cooperative termination: set notification bit 1 (stop), tasks check the bit and exit their loops. Clean up the hardware timer, drain the queue with packet deletion, and guard against double-stop. The `~Device()` destructor calls `stopAsynchronousEngine()` as a safety net.

### Consequences

- **Positive**: Clean task termination with resource cleanup. Idempotent double-stop guard prevents crashes. Destructor safety net catches forgotten shutdowns.
- **Negative**: Tasks must check the notification bit; blocking calls may delay shutdown.
- **Rationale**: Cooperative shutdown is the FreeRTOS-recommended pattern. It ensures tasks release resources and exit cleanly, preventing memory leaks and dangling timers.

### References

- `src/Device.h`, `src/Device.cpp`
- `tests/EngineConfig/test.cpp`
- `docs/improvements.md` (W9)

---

<a id="adr-021-golden-host-tests-for-telemetry-json-serialization"></a>
## ADR-021: Golden Host Tests for Telemetry JSON Serialization

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

The telemetry pipeline produces JSON output via ArduinoJson. Without host tests, changes to `TelemetryPackage` or `MqttGatewayClient` could silently break the JSON format, with bugs only surfacing in the field.

Two alternatives were evaluated:
1. **Golden host tests** — Assert exact JSON output strings for known inputs. Catches format regressions.
2. **No host tests** — Rely on ESP32 compilation and manual testing. Bugs found only on hardware.

### Decision

Create `tests/telemetry/test.cpp` with 10 golden test cases covering `SmartBandTelemetryPackage` and `AirQualityTelemetryPackage` serialization, envelope metadata, bounded buffer overflow, and field count validation. Tests run on the host with ArduinoJson mocks.

### Consequences

- **Positive**: Catches JSON format regressions before hardware deployment. Validates envelope metadata (timestamp/sequence) and bounded buffer behavior. Zero hardware dependency.
- **Negative**: ArduinoJson mock uses `std::map` which sorts keys alphabetically, so golden strings reflect alphabetical order.
- **Rationale**: Golden tests provide the highest assurance that the telemetry pipeline produces correct output. They are proportional in complexity to the value they provide.

### References

- `tests/TelemetryPackage/test.cpp`
- `tests/fakes/ArduinoJson.h`
- `docs/improvements.md` (W11, T13)

---

<a id="adr-022-notification-redesign-for-interrupt-driven-sensors-n1"></a>
## ADR-022: Notification Redesign for Interrupt-Driven Sensors (N1)

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

The current notification mechanism uses `vTaskNotifyGiveFromISR` for the timer ISR and `std::atomic<uint32_t> pendingTicks` for elapsed tick counting. Interrupt-driven sensors (W13) need a way to wake the sampler task without colliding with timer notifications.

Two alternatives were evaluated:
1. **Bit-flag notifications** — Replace `vTaskNotifyGiveFromISR` with `xTaskNotifyFromISR(handle, bit, eSetBits)`. Each source (timer, interrupt triggers) uses a distinct bit. Elapsed ticks in an ISR-incremented atomic that the task exchanges to zero.
2. **Multiple notification queues** — Separate queues for timer and interrupt events. Higher overhead.

### Decision

Redesign notifications to use bit flags: timer uses bit 0, stop uses bit 1, interrupt triggers use bits 2+. Elapsed ticks remain in `std::atomic<uint32_t> pendingTicks` incremented by the ISR and exchanged to zero by the task.

ISR rules: mark handlers `IRAM_ATTR`, keep them tiny, no heap/Serial/logging, use `portYIELD_FROM_ISR` when a higher-priority task was woken, rate-limit contact bounce, never call `digitalRead` from the ISR.

### Consequences

- **Positive**: Multiple notification sources coexist without collision. ISR-safe bit operations are fast and deterministic. Atomic tick counter avoids race conditions.
- **Negative**: Requires changing all existing notification calls. Limited to 32 bits (sufficient for current use cases).
- **Rationale**: Bit-flag notifications are the standard FreeRTOS pattern for multi-source wakeups. They enable interrupt-driven sensors (W13) and command marshaling (W12) without redesigning the notification mechanism again.

### References

- `src/Device.h`, `src/Device.cpp`
- `docs/improvements.md` (W13, W12)

---

<a id="adr-024-command-marshaling-for-cross-task-safety-w12"></a>
## ADR-024: Command Marshaling for Cross-Task Safety (W12)

**Status**: Accepted | **Date**: 2026-11-09 | **Supersedes**: N/A

### Context

The framework runs two FreeRTOS tasks: a sampler task for all sensor/actuator I/O and a network task for asynchronous transmission. Cloud-to-device commands arrive on the network task via MQTT, but actuators must be controlled on the sampler task to preserve the single-context guarantee. Without cross-task marshaling, commands would execute on the wrong task, introducing race conditions.

Two alternatives were evaluated:
1. **Command queue with notification bit** — Network task enqueues `CommandEntry` (handler + command) to a FreeRTOS queue and sets `CommandAvailable` bit (0x8). Sampler task drains the queue and calls `handle(command)` on its own context.
2. **Direct handler call** — Network task calls `handle(command)` directly. Violates the single-context guarantee.

### Decision

Implement `Device::enqueueCommand(CommandHandler* targetHandler, Command command)` that pushes a `CommandEntry` to a FreeRTOS queue and sets the `CommandAvailable` notification bit. The sampler task drains the queue in `processPendingCommands()` and calls `targetHandler->handle(command)` on its own context.

### Consequences

- **Positive**: Preserves the single-context guarantee for all command execution. Enables cloud-to-device commands (W15) without redesigning the threading model.
- **Negative**: Adds one queue and one notification bit. Commands are delayed until the next sampler wake-up.
- **Rationale**: The queue-based approach is the standard FreeRTOS pattern for cross-task communication. It integrates cleanly with the existing notification bit-flag redesign (ADR-022).

### References

- `src/Device.h`, `src/Device.cpp`
- `docs/improvements.md` (W12)

---

<a id="adr-025-non-blocking-sensor-protocols-w14"></a>
## ADR-025: Non-Blocking Sensor Protocols (W14)

**Status**: Accepted | **Date**: 2026-11-09 | **Supersedes**: N/A

### Context

Two sensors block the sampler task, consuming its entire time budget:
1. **LoadCellAmplifier** (`readRawValue()`): Up to **100ms busy-wait** polling the HX711 data pin.
2. **UltrasonicSensor** (`processEvent()`): Up to **30ms blocking** inside `pulseIn()` waiting for the echo pin.

Two alternatives were evaluated:
1. **Interrupt-driven replacement** — HX711 uses `appendToBothSchedulers()` with CHANGE edge for data-ready; ultrasonic uses GPIO interrupt with `esp_timer_get_time()` for echo timing.
2. **Keep blocking** — Accept the latency; document in the handler time budget table.

### Decision

Convert both sensors to interrupt-driven operation:
- **LoadCellAmplifier**: Added `dataReady` volatile flag. ISR sets the flag when the HX711 data pin goes LOW. `processEvent()` checks `dataReady` first, suppressing propagation if not ready. The 24-bit read (~1.2ms) still blocks but is orders of magnitude shorter than the 100ms wait.
- **UltrasonicSensor**: Replaced `pulseIn()` with GPIO interrupt on the echo pin. Rising edge starts `esp_timer_get_time()`, falling edge stops it. Distance computed from the duration in `processEvent()`.

### Consequences

- **Positive**: Eliminates 99% of sampler blocking (100ms → ~1.2ms for HX711, 30ms → microseconds for ultrasonic). Enables higher sampling frequencies.
- **Negative**: Adds interrupt handlers and volatile state. HX711 timing remains sensitive to clock pulse accuracy.
- **Rationale**: The interrupt-driven pattern matches the triggering scheduler infrastructure (ADR-022). The performance improvement is significant and transparent to the API.

### References

- `src/LoadCellAmplifier.h`, `src/LoadCellAmplifier.cpp`
- `src/UltrasonicSensor.h`, `src/UltrasonicSensor.cpp`
- `docs/improvements.md` (W14)

---

<a id="adr-026-cloud-to-device-mqtt-subscribe-w15"></a>
## ADR-026: Cloud-to-Device MQTT Subscribe (W15)

**Status**: Accepted | **Date**: 2026-11-09 | **Supersedes**: N/A

### Context

The framework only supports outbound telemetry. There is no path from MQTT topic to `Command`, meaning devices cannot receive remote control commands. The command marshaling infrastructure (W12) provides `Device::enqueueCommand()` for cross-task safety.

Two alternatives were evaluated:
1. **Device* parameter** — `MqttGatewayClient` accepts `Device*` and uses `Device::enqueueCommand()` to marshal commands to the sampler task.
2. **CommandHandler* parameter** — `MqttGatewayClient` accepts `CommandHandler*` and calls `handle()` directly in the network task context.

### Decision

`MqttGatewayClient` accepts `Device* targetDevice` and uses `Device::enqueueCommand(targetDevice, command)` in the MQTT callback. The callback parses JSON to extract `commandId`, creates a `Command`, and marshals it to the sampler task. A fixed 256-byte buffer prevents heap allocation.

### Consequences

- **Positive**: Completes bidirectional communication. Commands are safely delivered to the sampler task. Enables remote actuator control and configuration updates.
- **Negative**: Adds subscribe topic parameter and callback infrastructure. MQTT callback runs in the network task context.
- **Rationale**: Since `Device` extends `CommandHandler`, the Device itself is the natural command target. Using `enqueueCommand()` ensures cross-task safety without requiring the caller to understand the threading model.

### References

- `src/MqttGatewayClient.h`, `src/MqttGatewayClient.cpp`
- `docs/improvements.md` (W15)

---

<a id="adr-027-tickless-scheduling-and-light-sleep-w16"></a>
## ADR-027: Tickless Scheduling and Light Sleep (W16)

**Status**: Accepted | **Date**: 2026-11-09 | **Supersedes**: N/A

### Context

The hardware timer wakes the system at a fixed interval forever, preventing sleep. With the pure scheduler (`SamplingScheduler`), the next due time is known from `ticksUntilNextDue()`. The device can sleep between samples, reducing idle CPU consumption.

Two alternatives were evaluated:
1. **One-shot timer with light sleep** — Convert the hardware timer from periodic to one-shot mode. After each sampler dispatch, re-arm the alarm for `ticksUntilNextDue()`. Enter `esp_light_sleep_start()` so the CPU halts until the next alarm.
2. **Keep periodic timer** — Accept the idle CPU consumption; document in the power budget.

### Decision

Add `PowerMode` enum (`Periodic`, `Tickless`) to Device. In tickless mode, the timer is re-armed after each dispatch using `scheduler.ticksUntilNextDue()`. The device enters ESP32 light sleep via `esp_light_sleep_start()`, halting the CPU until the timer fires. Periodic mode remains the default for backward compatibility.

### Consequences

- **Positive**: Reduces idle CPU consumption. Enables battery-powered always-on devices. Light sleep preserves RAM and CPU state; tasks resume on wake.
- **Negative**: Adds complexity to the timer ISR and sampler loop. Wi-Fi association must be maintained via modem-sleep configuration.
- **Rationale**: Tickless scheduling is the standard pattern for low-power IoT devices. The infrastructure (`ticksUntilNextDue()`) is minimal (~10 lines) and integrates cleanly with the existing scheduler.

### References

- `src/SamplingScheduler.h`, `src/SamplingScheduler.cpp`
- `src/Device.h`, `src/Device.cpp`
- `docs/improvements.md` (W16)

---

<a id="adr-028-deep-sleep-cycle-and-battery-example-w17"></a>
## ADR-028: Deep-Sleep Cycle and Battery Example (W17)

**Status**: Accepted | **Date**: 2026-11-09 | **Supersedes**: N/A

### Context

Battery-powered IoT devices need true deep sleep (~10µA) rather than light sleep (~100mA). Deep sleep shuts down the CPU, Wi-Fi, and most peripherals, retaining only RTC (Real-Time Clock) memory. The device reboots on each wake, requiring state to survive via `RTC_DATA_ATTR` variables.

Light sleep (W16) keeps Wi-Fi and RAM alive but consumes ~100mA. For coin-cell battery operation, deep sleep is essential — it reduces power by 10,000×, enabling months of operation.

Two alternatives were evaluated:
1. **Deep sleep with RTC memory** — Add `PowerMode::DeepSleep`, use `esp_deep_sleep_start()` after each sampler dispatch. Store state in RTC memory via `RTC_DATA_ATTR`. Report wake reason on boot.
2. **Light sleep only** — Accept ~100mA idle consumption; suitable for mains-powered devices but not battery operation.

### Decision

Add `PowerMode::DeepSleep` to the existing `PowerMode` enum in `Device`. When set, the device enters deep sleep after each sampler dispatch via `Device::enterDeepSleep()`, which configures the timer wake source and calls `esp_deep_sleep_start()`. On wake, `Device::reportWakeReason()` uses `esp_sleep_get_wakeup_cause()` to report the wake source (timer, GPIO, reset).

Application state that must survive deep sleep uses the `RTC_DATA_ATTR` compiler attribute on free-standing variables, as ESP32 compiler restrictions prevent class member attributes.

The `WeatherStation` example demonstrates deep sleep with DHT22 and BH1750 sensors, RTC boot count tracking, and wake-reason reporting.

### Consequences

- **Positive**: Enables true battery operation (~10µA vs ~100mA active). With 5-minute sleep intervals, the device spends >99.9% of its time asleep. Coin cell batteries last months.
- **Negative**: Wi-Fi is disconnected during deep sleep; the device must reconnect on each wake (~200ms boot time). RAM is lost except for RTC memory. Application code must use `RTC_DATA_ATTR` for state survival.
- **Rationale**: Deep sleep is the standard pattern for battery-powered IoT devices. The infrastructure is minimal — one enum value, two methods, and RTC memory attributes. The `WeatherStation` example provides a template for battery-powered deployments.

### References

- `src/Device.h`, `src/Device.cpp`
- `examples/WeatherStation/WeatherStation.ino`
- `tests/DeepSleep/test.cpp`
- `docs/improvements.md` (W17)

---

<a id="adr-023-domain-driven-design-adoption-and-bounded-contexts"></a>
## ADR-023: Domain-Driven Design Adoption and Bounded Contexts

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

The Modest IoT ecosystem spans two frameworks with different complexity levels: the Nano Framework (ESP32 C++ Arduino library with 48 headers) and the Edge Framework (Raspberry Pi Python/Flask application with SQLite). As the ecosystem grows, explicit architectural guidance is needed to ensure consistent terminology, clear context boundaries, and appropriate design patterns for each framework's constraints.

The Nano Framework already embodies several Domain-Driven Design (DDD) concepts implicitly:
- **CQRS-inspired separation**: `EventHandler`/`Event` (queries/events) vs `CommandHandler`/`Command` (commands).
- **Aggregate Root**: `Device` orchestrates sensors, actuators, scheduler, and telemetry queue.
- **Value Objects**: `Event`, `Command`, `EngineConfig`, `DiagnosticsSnapshot` — equality by value, no identity.
- **Repository-like pattern**: `SamplingScheduler` manages sensor registration and per-slot scheduling.
- **Domain Events**: Sensor fault events (IDs 22/23), measurement requests.
- **Anti-Corruption Layer**: `ConnectivityDriver` interface isolates framework from Arduino/ESP-IDF core APIs.
- **Ubiquitous Language**: Consistent naming across 48 headers, 22 examples, and documentation.

However, the framework lacked explicit DDD **strategic design**: no documented bounded contexts, no context map, no documented aggregate invariants, and no shared glossary between Nano and Edge frameworks.

Three alternatives were evaluated:
1. **Full DDD** — Apply all DDD tactical patterns (aggregates, repositories, domain services, factories, specifications) to both frameworks.
2. **DDD subset for Nano, full DDD for Edge** — Use DDD's embedded-friendly subset for Nano (ubiquitous language, bounded contexts, value objects, documented invariants) while applying full DDD tactical patterns to the Edge Framework where Python/Flask can handle the overhead.
3. **No explicit DDD** — Continue implicit DDD patterns without documenting the strategic design.

### Decision

Adopt DDD explicitly with a **two-tier approach**:

**Nano Framework (Embedded DDD Subset)**:
- **Ubiquitous Language**: Enforce consistent naming: `Device`, `Sensor`, `Actuator`, `Event`, `Command`, `TelemetryPackage`.
- **Bounded Context**: Document the Nano Device Context boundaries (ESP32, Arduino core, FreeRTOS tasks).
- **Aggregate Root**: `Device` is the sole aggregate; document its invariants (sensor registration before engine start, queue length limits, fault event latching).
- **Value Objects**: `Event`, `Command`, `EngineConfig`, `DiagnosticsSnapshot` — already implemented.
- **Repository**: `SamplingScheduler` as sensor slot repository — already implemented.
- **Skip**: Full repository pattern, domain event dispatcher, factories, specifications — too heavy for embedded C++.

**Edge Framework (Full DDD)**:
- **Aggregates**: `DeviceProfile` (owns device metadata, online status, last-seen), `Rule` (owns condition, action, enabled state).
- **Repositories**: `DeviceRegistry` (DeviceProfile queries), `TelemetryRepository` (time-series storage and aggregation).
- **Domain Services**: `TelemetryIngestionService` (processes incoming telemetry, validates envelope metadata, updates device status), `RuleEngine` (evaluates automation rules).
- **Domain Events**: `DeviceOfflineEvent`, `TelemetryReceivedEvent`, `RuleTriggeredEvent` for decoupled service communication.
- **Anti-Corruption Layer**: REST API isolates Edge's internal models (Peewee ORM) from external clients (HTMX dashboard, cloud relay).

**Bounded Contexts and Context Map**:
- **Nano Device Context**: Bounded by ESP32 microcontroller. Aggregate: `Device`. Language: `Sensor`, `Actuator`, `TelemetryPackage`.
- **Edge Gateway Context**: Bounded by Raspberry Pi. Aggregates: `DeviceProfile`, `Rule`. Language: `TelemetryRecord`, `Alert`, `CommandQueue`.
- **Relationship**: **Published Language** — MQTT topics and BLE NUS define the stable contract between Nano and Edge. Nano publishes telemetry; Edge subscribes and publishes commands.
- **External Clients Context**: **Anti-Corruption Layer** — REST API isolates Edge from HTMX dashboard, cloud relay, and third-party integrations.

**Ubiquitous Language Dictionary**:
A shared glossary ensures consistent terminology across both frameworks:

| Term | Definition | Nano Equivalent | Edge Equivalent |
|---|---|---|---|
| **Device** | Physical IoT endpoint | `Device` class | `DeviceProfile` aggregate |
| **Sensor** | Reads measurements | `Sensor` subclass | Telemetry field |
| **Actuator** | Executes commands | `Actuator` subclass | Command action |
| **Telemetry** | Measurement data | `TelemetryPackage` | `TelemetryRecord` entity |
| **Event** | Domain event | `Event` class | `TelemetryReceivedEvent` |
| **Command** | Intent to act | `Command` class | MQTT command payload |
| **Rule** | Automation logic | N/A (local mediator) | `Rule` aggregate |
| **Alert** | Triggered condition | Sensor fault (ID 22) | `Alert` entity |

### Consequences

- **Positive**: Explicit bounded contexts prevent model leakage between Nano and Edge. Ubiquitous language ensures consistent communication across frameworks. DDD tactical patterns in Edge provide structure for Python/Flask application. Embedded-friendly subset avoids overhead in Nano.
- **Negative**: DDD adds documentation overhead. Full DDD in Edge requires discipline to maintain aggregate boundaries and repository abstractions.
- **Rationale**: DDD is not a framework or library — it's a set of principles and patterns that guide design decisions. The Nano Framework's Mediator pattern, CQRS-inspired Event/Command separation, and telemetry pipeline already reflect DDD thinking. Making this explicit improves communication, documentation, and future development without code overhead for Nano. The Edge Framework benefits from full DDD patterns appropriate for Python/Flask applications.

### References

- `src/Device.h`, `src/Device.cpp` (Nano aggregate root)
- `src/Sensor.h`, `src/Actuator.h` (Nano ubiquitous language)
- `src/Event.h`, `src/Command.h` (Nano CQRS separation)
- `src/TelemetryPackage.h` (Nano value object)
- `src/SamplingScheduler.h` (Nano repository)
- `docs/vision.md` (Edge Framework architecture, bounded contexts)
- `docs/improvements.md` (W12: command marshaling, W13: interrupts)

---

<a id="adr-029-native-servo-and-lcd-drivers-w19"></a>
## ADR-029: Native Servo and LCD Drivers (W19)

**Status**: Accepted | **Date**: 2026-11-09 | **Supersedes**: N/A

### Context

The framework depended on two external libraries for actuator control:
1. **ESP32Servo** — Used by `ServoMotor` for PWM-based servo control via the `Servo` class.
2. **LiquidCrystal_I2C** — Used by `CharacterLcdDisplay` for I2C-based HD44780 LCD control via a PCF8574 backpack.

These dependencies added ~10KB of flash footprint, introduced library version conflicts (LiquidCrystal_I2C has many forks), and prevented the framework from being truly self-contained. The ESP32 Arduino core provides native APIs (`ledcAttach`, `ledcWrite`, `Wire`) that can replace both libraries entirely.

Three alternatives were evaluated:
1. **Retain external libraries** — Simple, but adds dependencies and version conflicts.
2. **Replace only ESP32Servo** — Removes the servo dependency but keeps LiquidCrystal_I2C.
3. **Replace both with native implementations** — Eliminates both dependencies, making the framework self-contained.

### Decision

Replace both external libraries with native implementations:

**ServoMotor**: Uses `ledcAttach()`/`ledcWrite()` at 50Hz with 13-bit LEDC resolution. The ESP32 Arduino core 3.x API is pin-based (not channel-based), simplifying the implementation. The angle-to-duty-cycle conversion uses the existing pulse width constants (500-2400µs range).

**CharacterLcdDisplay**: Implements the HD44780 4-bit mode protocol natively via `Wire` with PCF8574 I/O expander backpack. The implementation includes the full initialization sequence (8-bit to 4-bit mode transition), enable pulse timing (~1µs high, ~37µs low), cursor positioning via DDRAM addresses (0x80, 0xC0, 0x94, 0xD4), and backlight control via PCF8574 bit manipulation.

**library.properties**: Removed `ESP32Servo` and `LiquidCrystal_I2C` from the `depends=` field.

### Consequences

- **Positive**: Zero external dependencies beyond ArduinoJson, MQTT (>=2.5.3), and DHT sensor library. ~10KB flash footprint reduction. No library version conflicts. Framework is truly self-contained for v1.0.0 release.
- **Negative**: ~250 lines of additional code for the HD44780 protocol. The LCD driver is specific to the PCF8574 backpack (most common I2C LCD backpack).
- **Rationale**: Both ESP32Servo and LiquidCrystal_I2C are thin wrappers around ESP32 hardware peripherals. Replacing them with native implementations eliminates the dependencies while maintaining the same public API. The LEDC peripheral and I2C Wire API are stable across ESP32 Arduino core versions.

### Design Note: 4-Bit Mode Only

The `Hd44780Driver` supports only **4-bit mode** due to the PCF8574 I2C backpack hardware constraint. The PCF8574 I/O expander has 8 pins, but only 4 are available for data lines (P0–P3 map to HD44780 D4–D7). The remaining pins are consumed by control signals: Enable (P5), Register Select (P7), Backlight (P4), and Read/Write (P6, typically grounded).

8-bit mode requires 8 data lines (D0–D7), which the PCF8574 cannot provide. Supporting 8-bit mode would require either two PCF8574 chips (impractical) or a direct parallel connection using 11 GPIO pins (a fundamentally different hardware design). All 13 examples using CharacterLcdDisplay use I2C backpacks with 4-bit mode, and no component in the framework requires 8-bit mode.

If 8-bit mode support is ever needed, it would be a separate driver class (e.g., `Hd44780ParallelDriver`) using direct GPIO connections, not an extension of the I2C-based `Hd44780Driver`.

### References

- `src/ServoMotor.h`, `src/ServoMotor.cpp`
- `src/CharacterLcdDisplay.h`, `src/CharacterLcdDisplay.cpp`
- `tests/ServoMotor/test.cpp` (10 host tests)
- `tests/CharacterLcdDisplay/test.cpp` (10 host tests)
- `library.properties`

---

<a id="adr-030-bosch-environmental-sensor-base-class"></a>
## ADR-030: Bosch Environmental Sensor Base Class

**Status**: Accepted | **Date**: 2026-11-10 | **Supersedes**: N/A

### Context

The framework contains two Bosch environmental sensors:
1. **PthSensor** (BME280) — Temperature, humidity, pressure via I²C.
2. **EnvironmentalGasSensor** (BME680) — Temperature, humidity, pressure, and gas resistance via I²C.

Both sensors share identical Bosch compensation formulas for temperature, pressure, and humidity. The calibration data reading code (digT1-3, digP1-9, digH1-6) is also identical, as is the I²C register access pattern. The only differences are:
- Chip ID (0x60 for BME280, 0x61 for BME680)
- Operating mode (normal for BME280, forced for BME680)
- EnvironmentalGasSensor additionally reads gas calibration coefficients (digG1-3) and gas resistance data

The three compensation formulas alone account for ~100 lines of identical code. The calibration reading adds another ~50 lines of duplication.

### Decision

Extract the shared Bosch compensation formulas, calibration reading, and I²C helpers into a `BmeEnvironmentalSensor` base class that both `PthSensor` and `EnvironmentalGasSensor` extend.

**BmeEnvironmentalSensor** provides:
- Shared calibration coefficients (digT1-3, digP1-9, digH1-6)
- Shared compensation methods (`compensateTemperature`, `compensatePressure`, `compensateHumidity`)
- Shared I²C helpers (`readRegister16`, `readCalibrationData`, `readRawData`)
- Shared cached/last-notified fields for temperature, humidity, pressure
- Shared epsilon-based change detection thresholds
- Public getters (`getTemperatureInCelsius`, `getHumidityInPercentage`, `getPressureInHectopascals`, `getAltitudeInMeters`)

**PthSensor** adds:
- BME280 chip ID validation (0x60)
- Normal mode configuration
- `processEvent()` for three-value change detection

**EnvironmentalGasSensor** adds:
- BME680 chip ID validation (0x61)
- Forced mode configuration
- Gas calibration coefficients (digG1-3)
- Gas resistance compensation and getter
- `processEvent()` for four-value change detection

### Consequences

- **Positive**: Eliminates ~200 lines of duplicated compensation code. Single source of truth for Bosch formulas. Easier to maintain and test. Future BME sensors (e.g., BME280 variants) can extend the base class trivially.
- **Negative**: Adds a level of indirection. The base class is abstract in practice (no standalone use case) but not formally `abstract` in C++.
- **Rationale**: The compensation formulas are character-for-character identical between BME280 and BME680. The Bosch datasheet explicitly states these formulas are shared across the BME family. Extracting them into a base class follows the DRY principle and reduces maintenance burden.

### References

- `src/BmeEnvironmentalSensor.h`, `src/BmeEnvironmentalSensor.cpp`
- `src/PthSensor.h`, `src/PthSensor.cpp`
- `src/EnvironmentalGasSensor.h`, `src/EnvironmentalGasSensor.cpp`
- `tests/PthSensor/test.cpp` (12 host tests)
- `tests/EnvironmentalGasSensor/test.cpp` (11 host tests)

---

## Consolidated Improvements Document

The 8 individual improvement documents and the roadmap have been consolidated into a single `docs/improvements.md` file, organized by tier. This eliminates cross-document maintenance overhead while keeping all information accessible through headings and search.

### Current Structure

```
docs/
├── adrs.md                          (This file: Architecture Decision Records)
├── improvements.md                  (Consolidated: all tiers, work items, and detailed plans)
├── user-stories.md                  (Original requirements)
├── class-diagram.puml               (PlantUML class diagram)
└── doxygen-mainpage.dox             (Doxygen main page)
```

The flat structure is simpler for small teams but becomes unwieldy as the number of documents grows. The subdirectory approach scales better for long-term maintenance.

---

## ADR Template

For future architecture decisions, use this template:

```markdown
## ADR-XXX: [Short Title]

**Status**: Proposed | Accepted | Deprecated | Superseded by ADR-YYY | **Date**: YYYY-MM-DD

### Context

[What is the issue or motivation? What forces are at play?]

### Decision

[What is the decision we are making?]

### Consequences

[What becomes easier or more difficult to do because of this change? What are the positive and negative effects?]

### References

[Links to source files, tests, or related ADRs]
```

---

*Last updated: 2026-11-10 (30 ADRs: 13 original + 9 from improvements.md review + 1 DDD adoption + 4 from W12-W16 + 1 from W17 + 1 from W19 + 1 BME base class)*
