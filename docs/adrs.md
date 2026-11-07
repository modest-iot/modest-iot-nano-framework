# Architecture Decision Records

Architecture Decision Records (ADRs) capture the key technical decisions made during the development of the Modest IoT Nano Framework, along with their context, trade-offs, and consequences.

Each ADR documents **why** a decision was made, not just **what** was decided. This enables future developers to understand the rationale, evaluate whether circumstances have changed, and make informed decisions about superseding or revisiting past choices.

## Overview

The Modest IoT Nano Framework is a lightweight, deterministic IoT framework for ESP32-based devices. It provides a Mediator design pattern for sensor-actuator wiring, a per-slot sampling scheduler, a telemetry pipeline with ownership safety, and connectivity drivers for Wi-Fi, BLE, and MQTT. The framework runs on FreeRTOS with two isolated tasks: a sampler task for all sensor/actuator I/O and a network task for asynchronous transmission.

The 22 ADRs below span five categories that shaped the framework's architecture:

| # | Title | Status | Category |
|---|---|---|---|
| [ADR-001](#adr-001-framework-naming-convention--modest_log_-macros-with-miot_log_-aliases) | Framework Naming Convention — `MODEST_LOG_*` Macros | Accepted | Naming |
| [ADR-002](#adr-002-ble-connectivity-via-nordic-uart-service-nus) | BLE Connectivity via Nordic UART Service (NUS) | Accepted | Connectivity |
| [ADR-003](#adr-003-hybrid-approach-for-sensor-sampling-intervals) | Hybrid Approach for Sensor Sampling Intervals | Accepted | Scheduling |
| [ADR-004](#adr-004-unique_ptr-ownership-for-telemetry-enqueue) | `unique_ptr` Ownership for Telemetry Enqueue | Accepted | Telemetry |
| [ADR-005](#adr-005-drop-oldest-queue-policy) | Drop-Oldest Queue Policy | Accepted | Telemetry |
| [ADR-006](#adr-006-bounded-serialization-buffer) | Bounded Serialization Buffer | Accepted | Telemetry |
| [ADR-007](#adr-007-envelope-metadata--timestamp-and-sequence) | Envelope Metadata — Timestamp and Sequence | Accepted | Telemetry |
| [ADR-008](#adr-008-serialize-overload-rename--todocument-and-tojsonstring) | `serialize` Overload Rename — `toDocument` and `toJsonString` | Accepted | API Design |
| [ADR-009](#adr-009-sensor-fault-events--debounced-threshold-state-machine) | Sensor Fault Events — Debounced Threshold State Machine | Accepted | Diagnostics |
| [ADR-010](#adr-010-engineconfig--configurable-task-stacks-and-priorities) | `EngineConfig` — Configurable Task Stacks and Priorities | Accepted | Threading |
| [ADR-011](#adr-011-samplingscheduler--per-slot-countdown-scheduling) | `SamplingScheduler` — Per-Slot Countdown Scheduling | Accepted | Scheduling |
| [ADR-012](#adr-012-retain-arduinojson-for-telemetry-serialization) | Retain ArduinoJson for Telemetry Serialization | Accepted | Dependencies |
| [ADR-013](#adr-013-screaming_snake_case-macro-convention) | `SCREAMING_SNAKE_CASE` Macro Convention | Accepted | Naming |
| [ADR-014](#adr-014-threading-model--single-context-sampler-with-isolated-network-task) | Threading Model — Single-Context Sampler | Accepted | Threading |
| [ADR-015](#adr-015-identifier-registry-with-reserved-ranges) | Identifier Registry with Reserved Ranges | Accepted | API Design |
| [ADR-016](#adr-016-dependency-hygiene-principles) | Dependency Hygiene Principles | Accepted | Dependencies |
| [ADR-017](#adr-017-shared-mock-harness-for-host-unit-tests) | Shared Mock Harness for Host Unit Tests | Accepted | Testing |
| [ADR-018](#adr-018-diagnosticssnapshot--on-demand-stack-and-heap-metrics) | `DiagnosticsSnapshot` — On-Demand Stack and Heap Metrics | Accepted | Diagnostics |
| [ADR-019](#adr-019-mqtt-reconnect-policy--rate-limited-backoff-with-continuous-keep-alive) | MQTT Reconnect Policy — Rate-Limited Backoff | Accepted | Connectivity |
| [ADR-020](#adr-020-stopasynchronousengine--cooperative-shutdown-via-notification-bits) | `stopAsynchronousEngine` — Cooperative Shutdown | Accepted | Threading |
| [ADR-021](#adr-021-golden-host-tests-for-telemetry-json-serialization) | Golden Host Tests for Telemetry JSON Serialization | Accepted | Testing |
| [ADR-022](#adr-022-notification-redesign-for-interrupt-driven-sensors-n1) | Notification Redesign for Interrupt-Driven Sensors (N1) | Proposed | Threading |

### Status Legend

- **Accepted** — The decision has been implemented and is in effect.
- **Proposed** — The decision is documented but not yet implemented; it is a prerequisite for future work.
- **Superseded** — The decision has been replaced by a newer ADR (none yet).

### Category Summary

| Category | Count | Key Decisions |
|---|---|---|
| **Naming** | 2 | `MODEST_LOG_*` macros with `MIOT_LOG_*` aliases; `SCREAMING_SNAKE_CASE` convention |
| **Connectivity** | 2 | BLE via NUS GATT profile; MQTT rate-limited reconnect with keep-alive |
| **Scheduling** | 2 | Hybrid sensor intervals with virtual getters; per-slot countdown scheduler |
| **Telemetry** | 4 | `unique_ptr` ownership; drop-oldest policy; bounded buffer; envelope metadata |
| **Diagnostics** | 2 | Sensor fault events with debounced threshold; on-demand `DiagnosticsSnapshot` |
| **Threading** | 4 | Single-context sampler model; `EngineConfig`; cooperative shutdown; notification redesign (proposed) |
| **API Design** | 2 | `serialize` overload rename; identifier registry with reserved ranges |
| **Dependencies** | 2 | Retain ArduinoJson; five hygiene principles for dependency decisions |
| **Testing** | 2 | Shared mock harness; golden host tests for JSON serialization |

---

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

- **Positive**: Zero namespace collision risk, aligns with `ModestIoT.h` branding, follows C/C++ `SCREAMING_SNAKE_CASE` macro convention, complies with Clang-Tidy and MISRA rules.
- **Negative**: 2 characters longer than `MIOT_LOG_*` per call site.
- **Rationale**: For the cost of 2 extra characters, the framework eliminates namespace collisions with Xiaomi IoT and maintains complete brand consistency. The alias provides an opt-in ergonomic path.

### References

- `src/ModestLog.h`
- `tests/ModestLog/test.cpp`
- Evaluation discussion in session history

---

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
- **Negative**: Cloud-to-device commands require `Device::post` marshalling (W12). Developers must understand the two-task model.
- **Rationale**: The single-context model is simpler, safer, and matches the embedded IoT use case where sensors are polled and actuators are commanded sequentially.

### References

- `src/Device.h`, `src/Device.cpp`
- `README.md` (Threading Model section)
- `docs/improvements.md`

---

## ADR-015: Identifier Registry with Reserved Ranges

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

The framework uses integer identifiers for events and commands. Without a registry, user-defined identifiers could collide with framework-internal ones, causing silent misrouting.

Two alternatives were evaluated:
1. **Centralized registry** — Framework reserves ranges (0-999), users use 1000+. Reserved blocks for future framework additions.
2. **String-based identifiers** — Zero collision risk, but higher memory and comparison cost.

### Decision

Maintain a centralized identifier registry with reserved ranges:
- Framework events: 0-99 (currently 10, 20-23, 30-31, 40, 50-51)
- Framework commands: 100-399 (currently 100-102, 150-152, 200-202, 250-252, 300-303)
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

## ADR-016: Dependency Hygiene Principles

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

The framework depends on third-party Arduino libraries (DHT, PubSubClient, ArduinoJson, etc.). Each dependency increases install friction, Flash/RAM footprint, and potential API breakage.

Five principles were established to govern dependency decisions:
1. Remove what is unused first. It costs nothing and cuts install friction.
2. Replace only simple, well-specified protocols, where a native implementation is a few dozen lines.
3. Never replace protocols with timing risk or large state machines (DHT, MQTT, TLS, JSON).
4. Always wrap before replacing. Introduce a small internal interface first.
5. Library Manager friction counts. Each extra `depends=` entry is auto-installed for every user.

### Decision

Apply the five principles to all dependency decisions. Delete `LoadCellController` and remove `HX711_Arduino_Library` and `EspSoftwareSerial` from `library.properties`. Retain `Adafruit_Unified_Sensor` as a transitive dependency of the DHT library. Pin `ArduinoJson (>=7.0.0)`.

### Consequences

- **Positive**: Reduced install friction, smaller footprint, no unnecessary dependencies. Clear criteria for future dependency decisions.
- **Negative**: Native HX711 implementation in `LoadCellAmplifier` requires maintenance.
- **Rationale**: The principles balance developer experience with framework autonomy. Simple protocols (HX711) are replaced; complex ones (DHT, MQTT, JSON) are retained.

### References

- `library.properties`
- `src/LoadCellAmplifier.h`, `src/LoadCellAmplifier.cpp`
- `docs/improvements.md` (W2)

---

## ADR-017: Shared Mock Harness for Host Unit Tests

**Status**: Accepted | **Date**: 2026-11-06 | **Supersedes**: N/A

### Context

The framework needed host unit tests that compile without Arduino/ESP32 toolchains. Initially, each test suite maintained its own local `fakes/` directory with duplicate mock headers.

Two alternatives were evaluated:
1. **Centralized `tests/fakes/`** — Single shared mock harness for all test suites.
2. **Per-suite fakes** — Each test suite maintains its own mocks.

### Decision

Consolidate all mocks into a centralized `tests/fakes/` directory with shared headers: `Arduino.h`, `Wire.h`, `SPI.h`, `BLEDevice.h`, `BLEServer.h`, `BLEUtils.h`, `PubSubClient.h`, `WiFi.h`, `ArduinoJson.h`, `Client.h`. All test suites include from the shared location.

### Consequences

- **Positive**: Single source of truth for mocks. Changes to a mock (e.g., adding a new API) benefit all test suites. Eliminates duplicate code.
- **Negative**: All tests share the same mock interface; test-specific mocks require conditional compilation.
- **Rationale**: With 11 test suites, the shared harness eliminates ~10× duplicate mock code and ensures consistent behavior across all tests.

### References

- `tests/fakes/`
- `tests/run_all.sh`
- `CMakeLists.txt`
- `docs/improvements.md` (W5)

---

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

## ADR-022: Notification Redesign for Interrupt-Driven Sensors (N1)

**Status**: Proposed | **Date**: 2026-11-06 | **Supersedes**: N/A

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
- **Rationale**: Bit-flag notifications are the standard FreeRTOS pattern for multi-source wakeups. They enable interrupt-driven sensors (W13) and command marshalling (W12) without redesigning the notification mechanism again.

### References

- `src/Device.h`, `src/Device.cpp`
- `docs/improvements.md` (W13, W12)

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

*Last updated: 2026-11-06 (22 ADRs: 13 original + 9 added from improvements.md review)*
