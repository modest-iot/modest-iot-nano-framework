# Modest IoT Improvements

Comparison tables and future work for the Modest IoT Nano Framework.

> **Disclaimer**: This roadmap reflects current priorities and may change based on community feedback and technical discoveries.
>
> **Internal planning**: A full internal planning document with detailed prioritization, cost/impact analysis, interdependency graphs, and implementation paths is maintained separately for framework maintainers.
>
> **Status snapshot**: Updated 2026-11-27. Tier completion describes the scoped work recorded for those tiers; it does not mean all platform-readiness or release gates are closed. v1.3.0 is the latest release; the I²C and sensor/event/actuator readiness work is unreleased, with physical validation still open.

## Document Map

| Information | Document |
|-------------|----------|
| Quick start guide | [quick-start.md](https://github.com/modest-iot/modest-iot-nano-framework/blob/release/v1.3.0/docs/quick-start.md) |
| Troubleshooting guide | [troubleshooting.md](https://github.com/modest-iot/modest-iot-nano-framework/blob/release/v1.3.0/docs/troubleshooting.md) |
| Migration guide | [migration.md](https://github.com/modest-iot/modest-iot-nano-framework/blob/release/v1.3.0/docs/migration.md) |
| Quick start & components | [README.md](../README.md) |
| Version history | `CHANGELOG.md` — current release and unreleased entries |
| Architecture decisions | [adrs.md](adrs.md) |
| User stories | [user-stories.md](user-stories.md) |
| Class diagram | [class-diagram.puml](class-diagram.puml) |
| Completed work items through W49 | [evolution.md](evolution.md) |
| Doxygen API reference | [doxygen-mainpage.md](doxygen-mainpage.md) |

## Status Overview

| Work area | Status |
|-----------|--------|
| Tiers 0–5 and pre-release work (scoped items) | **Complete for the recorded scope** |
| v1.3.0 release features (W29, W43, W47 and listed release items) | **Released** |
| Hardware & Platform Readiness | **Active** — I²C and sensor/event/actuator implementation gates are complete; physical-board checks and later power/network/security gates remain |
| Other planned improvements below | **Open unless explicitly marked implemented or released** |

## v1.3.0 Release Summary

- **BLE compatibility**: `BleHal` supports ESP32 Arduino core 3.x and 4.x while preserving the `BleConnectivityDriver` NUS interface.
- **W29 automation**: `RulesEngine` maps composite events to fixed-capacity actuator actions.
- **W43 change-only telemetry**: `ChangeFilter<T>` applies per-sensor deadbands without dynamic allocation.
- **W47 on-device ML**: `AiMicroModelController` and the optional `EmlearnController` provide local inference.
- **MPU6050 sensing**: `ImuSensor` supplies calibrated inertial readings and windowed statistics for the `VibrationMonitor` example.

## Planned Improvements

Improvements are grouped by theme. Target versions indicate when each improvement is expected, not a commitment.

### Hardware & Platform Readiness

Improve reliability on physical ESP32 devices while preparing incrementally for a native ESP-IDF component without destabilizing the existing Arduino library. Readiness work covers runtime and peripheral lifecycles, bounded I²C failure handling, sensor and actuator behavior, networking and OTA safety, preserved CI support, and validation on physical hardware where simulation is insufficient. Internal implementation gates and acceptance criteria are maintained in `.junie/plans/hardware-platform-readiness.md`.

The shared I²C backend and injected peripheral migration, runtime lifecycle/ISR work, and sensor/event/actuator/sampler-diagnostics gate are implemented in the current checkout and recorded under `[Unreleased]` in `CHANGELOG.md`. Host tests and the configured ESP32/ESP32-S3 × Arduino-ESP32 3.3.13/4.0.0 GitHub build matrix completed successfully. Physical electrical, stuck-line, sensor-recovery, and actuator-response checks remain outstanding, as do the power/storage and networking/security gates; CI and host evidence do not close those physical or broader readiness requirements.

### Sensor, Event, Actuator, and Sampler Readiness (W49)

**✓ Implemented in the current checkout; unreleased.** Read-capable sensors expose validity, freshness age, and latched read-fault state while preserving cached readings on failures; fault/recovery notifications are delivered independently of data-event suppression. `EventAggregator` forwards each raw event once and emits a composite only when a newly received event contributes to the match. The additive checked-command path preserves existing `void` handlers, reports `Unknown` when a handler cannot verify acceptance, and exposes rule-action results; `ServoMotor` separates requested targets from LEDC-accepted output and does not claim measured physical movement. Sampler diagnostics now report measured synchronous work duration and coalesced missed ticks without introducing a hard real-time guarantee.

**Validation**: The host suite and coverage run passed; reported coverage for the 53 coverage-runner suites was 90.1% line and 83.6% function. The configured Arduino example-build workflows passed for ESP32 and ESP32-S3 on cores 3.3.13 and 4.0.0. No physical sensor recovery, PWM response, load-safe-state, or electrical validation is claimed.

### ArduinoJson Removal (W46) ✓

**✓ Implemented**: ArduinoJson eliminated via `TelemetryWriter` abstraction and native `snprintf`-based JSON serialization. Saves ~30-40 KB Flash, removes per-serialization heap allocation, and unblocks downstream telemetry features. The framework now has only one external dependency (MQTT >=2.5.3).

### On-Device ML Inference (W47) ✓ Implemented

**✓ Implemented**: `AiMicroModelController` abstract base class defining the ML inference interface with `predict()`, `regress()`, `isModelLoaded()`, and `lastResult()` methods. `EmlearnController` wraps emlearn's C API (`eml_trees_predict`, `eml_net_predict`) behind the uniform interface with forward-declared emlearn types and float-to-int16_t quantization. Supports tree-based models (RandomForest, DecisionTree), neural networks, and emlearn-generated inline C code via `loadGeneratedModel()`. 28 host tests. Demonstrated in VibrationMonitor example.

**Released**: v1.3.0

### MPU6050 IMU Sensor

**✓ Implemented**: `ImuSensor` for MPU6050 6-axis inertial measurement unit (accelerometer + gyroscope). I²C communication with factory calibration data, compensation formulas, and cached readings. Circular sample buffer (configurable up to 32 samples, default 10) with automatic per-axis windowed RMS and peak-to-peak statistics. Follows the polling sensor event pattern (`DATA_READ_EVENT`) with change detection suppression. Demonstrated in VibrationMonitor example with EmlearnController for anomaly detection.

**Released**: v1.3.0

### Event Aggregation & Rules Engine (W29, W31; v1.3.0 baseline)

- **Event Aggregator** (W31) ✓ Implemented: `EventAggregator` component for composite event pattern matching with time-windowed detection. Implements `EventHandler` to intercept sensor events. Fixed-size circular buffer (32 captured events) and pattern array (4 patterns) — zero heap allocation. Supports AND/OR logic, source-aware and conditional matching, dynamic pattern management, and nested aggregation. The raw pass-through and contribution-gated matching behavior is a post-release W49 refinement and remains unreleased.
- **Rules Engine** (W29) ✓ Implemented: `RulesEngine` maps composite events to fixed-capacity actuator commands. The additive checked-result observer and `Unknown` outcome for legacy handlers without verifiable results are post-release W49 refinements and remain unreleased. `ServoMotor` driver-accepted PWM reporting likewise does not confirm physical movement.

**Base components released**: v1.3.0. The W49 refinements are listed under `Unreleased` in `CHANGELOG.md`.

### Multi-Device Coordination (W30, W36)

ESP-NOW or MQTT-based device-to-device messaging for relay nodes and distributed sensing.

**Target**: Future; no release target committed.

### Telemetry Enhancements (W32–W34, W41–W43)

- **High-Priority Lane** (W32) ✓ Implemented: Separate bounded queue (4 slots) for critical alerts. Network task checks high-priority queue first (non-blocking), ensuring alerts bypass routine telemetry. Envelope includes `"priority": "high"` for edge-side routing.
- **Schema Registry** (W33) ✓ Implemented: Per-package schema versioning. Each TelemetryPackage subclass declares its own schema version; `setVersionMetadata()` falls back to global `Device::setTelemetrySchemaVersion()` only when the per-package version is not set.
- **MQTT QoS** (W34) ✓ Implemented: Configurable QoS (0/1/2) for publish and subscribe, retained message support for last-known-good telemetry. Backward-compatible defaults (QoS 0, non-retained).
- **Remote Diagnostics** (W41) ✓ Implemented: On-demand diagnostics via MQTT command (commandId = 300). Device responds with `DiagnosticsTelemetryPackage` containing all 14 health metrics. Framework-internal command handled by `handleInternalCommand()` before developer's `handle()`.
- **Statistical Aggregation** (W42): Min/max/average over time windows.
- **Change-Only Mode** (W43) ✓ Implemented: `ChangeFilter<T>` template class for per-slot deadband-based telemetry suppression. Configurable deadband threshold per sensor slot (0–7), with first-reading passthrough, NaN/Inf protection, and zero heap allocation. Integrated into `Device` via two new `appendSensorToScheduler()` overloads with deadband parameter. 45 host tests. Demonstrated in WeatherStation example with 0.5°C temperature deadband.

**Status**: W32–W34, W41, and W43 are implemented; W42 statistical aggregation remains planned.

### Power & Battery (W37–W38)

- **Adaptive Sampling** (W37): Automatically adjust sampling based on battery level.
- **Sleep Schedule** (W38): Time-aware sleep scheduling with wake windows.

**Target**: Future; no release target committed.

### Security & Fleet Management (W39)

- **Secure Element** (W39): Hardware-backed credential storage via ESP32 eFuse or external secure elements.

**Target**: Future; no release target committed.

### Developer Experience

- **PlatformIO Registry** (✓ v1.1.2): `library.json` manifest created for PlatformIO Registry compatibility. The framework is installable via `lib_deps` in `platformio.ini`.
- **Quick Start Guide** (✓ v1.1.2): Step-by-step onboarding from installation to first device. See [quick-start.md](https://github.com/modest-iot/modest-iot-nano-framework/blob/release/v1.3.0/docs/quick-start.md).
- **Troubleshooting Guide** (✓ v1.1.2): Common issues and solutions covering serial output, board detection, upload failures, Wi-Fi, BLE, MQTT, memory, sensors, and scheduler. See [troubleshooting.md](https://github.com/modest-iot/modest-iot-nano-framework/blob/release/v1.3.0/docs/troubleshooting.md).
- **Migration Guide** (✓ v1.1.2): Converting traditional `loop()` code to Modest IoT patterns with before/after examples. See [migration.md](https://github.com/modest-iot/modest-iot-nano-framework/blob/release/v1.3.0/docs/migration.md).
- **Example Template Generator**: CLI tool to generate skeleton device projects.

**Status**: PlatformIO, Quick Start, Troubleshooting, and Migration are complete; the example template generator remains planned without a release target.

### Sensor/Actuator Expansion

Expand the component library (22 sensors and 9 actuators in the current README inventory) with additional IoT components: BMP280, INA219, SGP30/SGP40, BMA4000/LIS2DH12, VL53L0X, CAP1188, DC Motor Controller, Stepper Motor, and Multi-channel Relay Board.

**Target**: Future; no release target committed.

### Component Enhancements

- **ConfigurationStore**: Schema migration, configuration profiles, backup/restore.
- **PowerManager**: Fuel gauge integration, charging status, power budget tracking.

**Target**: Future; no release target committed.

### Process & Release

- **✓ Automated Release Workflow**: Release validation and artifact generation are documented in `docs/release.md` and implemented in the release workflows.
- **✓ Code Coverage Reporting**: gcov/lcov reporting is integrated in CI. The latest filtered report is 90.1% line and 83.6% function coverage across 53 coverage-runner entries; `I2cBus` is omitted, so it is not whole-inventory coverage.
- **✓ Dependency Update Monitoring**: GitHub Actions and dependency status workflows are present; the supported Arduino core compile matrix is explicitly pinned.
- **✓ Benchmark Table**: `docs/benchmarks.md` publishes estimates and a validation methodology; physical measurements remain distinct from source/build estimates.

**Status**: Process and reporting infrastructure implemented; the latest filtered coverage report covers 53 runners (and omits `I2cBus`), so completing full-inventory coverage and physical hardware benchmarks remain follow-up validation work.

---

## Deliberately out of scope

- TLS/SSL implementation in the framework (use ESP32's built-in `WiFiClientSecure`; W22 examples are complete).
- Targeted (per-instance) commands until a use case appears.
- Switchable sensor power rails and ADC/DMA sampling.
- Non-ESP32 targets remain deferred. The separate Hardware & Platform Readiness initiative explores a native ESP-IDF component for supported ESP32 devices; this is not a commitment to non-ESP32 portability. Re-evaluate additional MCU families if concrete community demand and a support/validation plan emerge.

### Microcontroller Coverage Expansion

**Status**: Deferred — ESP32-only remains the strategic choice.

**Rationale**: The current product scope is ESP32-focused. Native ESP-IDF support for ESP32, if pursued, is a platform-backend change rather than cross-vendor MCU support.

**Re-evaluation trigger**: If community demand for non-ESP32 support emerges (e.g., GitHub issues or forum requests), assess a specific MCU family and its available connectivity, memory, toolchain, and hardware-validation requirements before changing the support scope.

---

## Comparison with traditional development

Compares three points: a traditional Arduino `loop()` app, Modest IoT v1.2.1 (previous), and Modest IoT v1.3.0 (current release). Features recorded under `[Unreleased]` are not attributed to v1.3.0.

> **Note**: Comparison tables are replaced with each release — past comparisons are not archived. Hardware resource and performance figures remain estimates until the validation methodology below is run on physical boards.

### Developer experience

| Aspect          | Traditional `loop()`                         | Modest IoT v1.2.1 (previous)                                                | Modest IoT v1.3.0 (current)                                 |
|-----------------|----------------------------------------------|-------------------------------------------------------------------------------|----------------------------------------------------------------------|
| First build     | Works with the board package                 | 27 examples; 1 required external Arduino library (MQTT)                         | 28 examples; CI matrix targets ESP32/ESP32-S3 with core 3.3.13 and 4.0.0 |
| Structure       | One growing `loop()` with `millis()` flags   | Sensors emit events, a `Device` mediates, actuators take commands             | Same, with event aggregation and rules engine for automated actions (W29 + W31)    |
| Adding a sensor | New globals, new timing variable, new branch | Subclass or reuse, register in the scheduler; per-sensor default periods; `PowerManager` for battery-aware sensors | Same, with event aggregation and rules engine for automated actions (W29 + W31)                  |
| Sampling rates  | Hand-written per-sensor timers               | Per-slot periods, minimums enforced, staggering handled                       | Same, with event aggregation and rules engine for automated actions (W29 + W31)                |
| Timing mistakes | Easy: a blocking call stalls everything      | Handler budgets documented; blocking HX711/ultrasonic removed; interrupt-driven sensors; `PowerManager` for configurable sleep modes | Same, with event aggregation and rules engine for automated actions (W29 + W31)          |
| Debugging       | `Serial.println` scattered                   | `MODEST_LOG_*`, stack high-water marks, sensor fault events, diagnostics, `firmwareVersion` in telemetry for fleet tracking | Same, with event aggregation and rules engine for automated actions (W29 + W31)         |
| Configuration   | Hardcoded credentials and intervals          | `ConfigurationStore` with ESP32 NVS, namespace isolation, type-safe access, read-only mode    | Same                                                               |
| Time handling   | `millis()` from boot                         | `TimeSynchronizationDriver` with NTP over Wi-Fi and RTC fallback for absolute time             | Same, with synchronized timestamps across devices (W30)                   |
| Networking      | Blocking connect/publish in `loop()`         | Separate network task, reconnect policy, keep-alive, TLS examples, `createSecureSocket()`, TLS/SSL as documented best practice     | Same, with event aggregation and rules engine for automated actions (W29 + W31)                      |
| Cloud-to-device | Hand-rolled callback                         | `Device::enqueueCommand` with `int32_t` payload for parameterized commands                   | Same, with event aggregation and rules engine for automated actions (W29 + W31)   |
| Offline data    | Lost when network unavailable                | `TelemetryBuffer` with LittleFS persistence for offline replay                                      | Same, with event aggregation and rules engine for automated actions (W29 + W31)       |
| Updates         | Manual reflashing required                   | OTA updates via `OtaUpdateHandler` component                                                    | Same                                                     |
| Testing         | Rarely possible off-device                   | 53 host test runners                                                           | 54 registered CTest cases passed 54/54; `tests/run_all.sh` passed 53/53; filtered coverage is 90.1% line / 83.6% function over 53 runners and omits `I2cBus` |
| Documentation   | README or inline comments                    | 30 ADRs, 19 user stories, PlantUML class diagram, Doxygen API reference, versioned CHANGELOG, and cross-linked docs | 35 ADRs, 54 user stories, PlantUML class diagram, Doxygen API reference, versioned CHANGELOG, and cross-linked docs (current checkout includes unreleased W49) |
| v1.3.0 additions | N/A                                           | N/A                                                                           | BLE core 3.x/4.x HAL, `RulesEngine`, `ChangeFilter<T>`, `ImuSensor`, and optional on-device ML |

### Performance

| Aspect                     | Traditional `loop()`              | Modest IoT v1.2.1 (previous)                            | Modest IoT v1.3.0 (current)                              |
|----------------------------|-----------------------------------|-----------------------------------------------------------|--------------------------------------------------------------------|
| Timing model               | Cooperative polling; jitter grows | Timer ISR plus sampler task; per-slot periods; interrupt-driven sensors | Same, with event aggregation and rules engine for automated actions (W29 + W31) |
| Sample period accuracy     | Depends on `loop()` duration      | Per-slot periods independent of slot count                | Same |
| Worst-case stall           | Any blocking call                 | Interrupt-driven HX711/ultrasonic; documented budgets; `PowerManager` handles sleep/wake transitions     | Same, with event aggregation and rules engine for automated actions (W29 + W31)          |
| Edge latency (button, PIR) | Up to one `loop()` pass           | Interrupt mode: task wake-up latency                      | Same, with event aggregation and rules engine for automated actions (W29 + W31)    |
| Network isolation          | A slow publish blocks sensing     | Isolated in the network task; bounded queue policy, `TelemetryBuffer` for offline replay        | Same, with event aggregation and rules engine for automated actions (W29 + W31)        |
| Heap behaviour             | Usually static                    | `unique_ptr` ownership, drop-oldest policy; LittleFS reduces heap pressure via `TelemetryBuffer` | Same; pool if measurement justifies it (W18)   |
| CPU when idle              | Spins in `loop()`                 | Sleeps between ticks; tickless and deep sleep supported; `PowerManager` handles sleep/wake transitions | Same, with event aggregation and rules engine for automated actions (W29 + W31) |
| Energy                     | High unless hand-tuned            | Deep sleep: ~10µA; light sleep via tickless mode; battery monitoring enables adaptive sleep strategies and optimized sleep modes via `PowerManager` | Same, with event aggregation and rules engine for automated actions (W29 + W31)               |
| Time synchronization      | None; boot-relative only          | NTP-synchronized absolute time via `TimeSynchronizationDriver`; RTC fallback for deep-sleep devices | Same, with synchronized timestamps across devices (W30) |
| Secure communication      | Manual TLS setup if needed        | TLS examples provided; `createSecureSocket()` builder; `WiFiClientSecure` documented; TLS/SSL as standard practice   | Same, with event aggregation and rules engine for automated actions (W29 + W31)       |

### Resource consumption

| Resource                 | Traditional `loop()` | Modest IoT v1.2.1 (previous)                                                              | Modest IoT v1.3.0 (current)                           |
|--------------------------|----------------------|---------------------------------------------------------------------------------------------|----------------------------------------------------------------|
| Task stacks              | `loop()` task only   | 4096 B sampler + 8192 B network, configurable via `EngineConfiguration`; `PowerManager` adds minimal stack for sleep handling | Same defaults                                             |
| Queue and packets        | None                 | Queue of pointers + one heap packet per reading; bounded by queue policy; `TelemetryBuffer` adds LittleFS storage for offline replay | Same, with event aggregation and rules engine for automated actions (W29 + W31) |
| Flash                    | Smallest             | Framework, Wi-Fi, BLE driver; only 1 dependency (MQTT); ~30-40 KB saved by W46; plus `OtaUpdateHandler`, `TelemetryBuffer` | Same                                                                |
| Static RAM               | Small                | Slot table, per-object state; trigger table (4 slots); ConfigurationStore NVS cache; `TelemetryBuffer` adds LittleFS buffer | Same                                             |
| Heap fragmentation       | Low                  | Lower with explicit ownership; `unique_ptr` transfer; LittleFS reduces heap pressure        | Same; pool in Tier 5 if measured (W18)                              |
| Third-party dependencies | As needed            | Only MQTT is required — ArduinoJson removed by W46; emlearn remains optional | Same                                                                |
| Logging overhead         | User-controlled      | Zero when compiled out; UART cost when on                                                  | Same                                                           |
| Persistent storage       | Manual SPIFFS/LittleFS | ConfigurationStore uses ESP32 NVS with namespace isolation; `TelemetryBuffer` uses LittleFS for offline replay | Same, with event aggregation for composite patterns (W29)  |
| Security overhead        | None by default      | TLS examples provided; `NetworkClientSecure` usage guided; no framework-level TLS enforcement                                  | Same, with event aggregation for composite patterns (W29) |

---

## How to validate the comparison

Run the same three reference applications on the same ESP32 board, once per variant (plain `loop()`, Modest IoT v1.2.1, Modest IoT v1.3.0):

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
