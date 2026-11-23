# Modest IoT Improvements

Comparison tables and future work for the Modest IoT Nano Framework.

> **Disclaimer**: This roadmap reflects current priorities and may change based on community feedback and technical discoveries.
>
> **Internal planning**: A full internal planning document with detailed prioritization, cost/impact analysis, interdependency graphs, and implementation paths is maintained separately for framework maintainers.

## Document Map

| Information | Document |
|-------------|----------|
| Quick start guide | [quick-start.md](quick-start.md) |
| Troubleshooting guide | [troubleshooting.md](troubleshooting.md) |
| Migration guide | [migration.md](migration.md) |
| Quick start & components | [README.md](../README.md) |
| Version history | [CHANGELOG.md](../CHANGELOG.md) |
| Architecture decisions | [adrs.md](adrs.md) |
| User stories | [user-stories.md](user-stories.md) |
| Class diagram | [class-diagram.puml](class-diagram.puml) |
| Completed work items W1–W28 | [evolution.md](evolution.md) |
| Doxygen API reference | [doxygen-mainpage.md](doxygen-mainpage.md) |

## Status Overview

| Tier                                    | Status       |
|-----------------------------------------|--------------|
| 0: Make it correct and shippable        | **Complete** |
| 1: Safety net and visibility            | **Complete** |
| 2: Reliability, diagnostics & footprint | **Complete** |
| 3: Core performance and DX              | **Complete** |
| 4: Capabilities                         | **Complete** |
| 5: Optimization and power               | **Complete** |
| Pre-Release Prerequisites (W20–W24)     | **Complete** |
| v1.1.0 Features (W25–W28)              | **Complete** |

## Planned Improvements

Improvements are grouped by theme. Target versions indicate when each improvement is expected, not a commitment.

### ArduinoJson Removal (W46) ✓

**✓ Implemented**: ArduinoJson eliminated via `TelemetryWriter` abstraction and native `snprintf`-based JSON serialization. Saves ~30-40 KB Flash, removes per-serialization heap allocation, and unblocks downstream telemetry features. The framework now has only one external dependency (MQTT >=2.5.3).

### On-Device ML Inference (W47)

Latency-critical on-device ML inference using emlearn (~2 KB Flash, ~50 bytes RAM). Opt-in via build guards, Arduino Library Manager compatible. Supports classical ML models: Random Forest, Decision Trees, KNN, simple neural networks.

**Target**: v1.3.0

### Event Aggregation & Rules Engine (W29, W31)

- **Event Aggregator** (W31) ✓ Implemented: `EventAggregator` component for composite event pattern matching with time-windowed detection. Implements `EventHandler` to intercept sensor events. Fixed-size circular buffer (32 captured events) and pattern array (4 patterns) — zero heap allocation. Supports AND patterns via `addAllMatchPattern()` (all required events within time window) and OR patterns via `addAnyMatchPattern()` (any single event triggers). Source-aware matching distinguishes sensors sharing the same event identifier (e.g., 9+ sensors share `DATA_READ_EVENT_IDENTIFIER = 20`) using per-position source filters with `EventAggregator::ANY_SOURCE` wildcards. Conditional patterns via `PatternCondition` callback (`bool (*)(int compositeEventId)`) suppress composite events based on sensor values at match time. Dynamic pattern management with `removePattern()` and `cleanupPatterns()`. Nested aggregator chains enabled by `EventAggregator::DEVICE_SOURCE_ID` — composite events carry sourceId 0, allowing downstream aggregators to filter composite events from raw sensor events. Three grounded examples: IntrusionDetector (standard patterns), ColdChainMonitor (conditional patterns), and PharmaceuticalCleanRoom (nested chains with OR patterns). 43 host tests.
- **Rules Engine** (W29): Application-layer rules triggered by composite events. Can be implemented by users using the event handler pattern and `EventAggregator`.

**Target**: v1.2.0–v1.3.0

### Multi-Device Coordination (W30, W36)

ESP-NOW or MQTT-based device-to-device messaging for relay nodes and distributed sensing.

**Target**: v1.3.0–v1.4.0

### Telemetry Enhancements (W32–W34, W41–W43)

- **High-Priority Lane** (W32) ✓ Implemented: Separate bounded queue (4 slots) for critical alerts. Network task checks high-priority queue first (non-blocking), ensuring alerts bypass routine telemetry. Envelope includes `"priority": "high"` for edge-side routing.
- **Schema Registry** (W33) ✓ Implemented: Per-package schema versioning. Each TelemetryPackage subclass declares its own schema version; `setVersionMetadata()` falls back to global `Device::setTelemetrySchemaVersion()` only when the per-package version is not set.
- **MQTT QoS** (W34) ✓ Implemented: Configurable QoS (0/1/2) for publish and subscribe, retained message support for last-known-good telemetry. Backward-compatible defaults (QoS 0, non-retained).
- **Remote Diagnostics** (W41) ✓ Implemented: On-demand diagnostics via MQTT command (commandId = 300). Device responds with `DiagnosticsTelemetryPackage` containing all 14 health metrics. Framework-internal command handled by `handleInternalCommand()` before developer's `handle()`.
- **Statistical Aggregation** (W42): Min/max/average over time windows.
- **Change-Only Mode** (W43) ✓ Implemented: `ChangeFilter<T>` template class for per-slot deadband-based telemetry suppression. Configurable deadband threshold per sensor slot (0–7), with first-reading passthrough, NaN/Inf protection, and zero heap allocation. Integrated into `Device` via two new `appendSensorToScheduler()` overloads with deadband parameter. 45 host tests. Demonstrated in WeatherStation example with 0.5°C temperature deadband.

**Target**: v1.2.0–v1.3.0 (W32 complete)

### Power & Battery (W37–W38)

- **Adaptive Sampling** (W37): Automatically adjust sampling based on battery level.
- **Sleep Schedule** (W38): Time-aware sleep scheduling with wake windows.

**Target**: v1.3.0

### Security & Fleet Management (W39)

- **Secure Element** (W39): Hardware-backed credential storage via ESP32 eFuse or external secure elements.

**Target**: v1.4.0

### Developer Experience

- **PlatformIO Registry** (✓ v1.1.2): `library.json` manifest created for PlatformIO Registry compatibility. The framework is installable via `lib_deps` in `platformio.ini`.
- **Quick Start Guide** (✓ v1.1.2): Step-by-step onboarding from installation to first device. See [quick-start.md](quick-start.md).
- **Troubleshooting Guide** (✓ v1.1.2): Common issues and solutions covering serial output, board detection, upload failures, Wi-Fi, BLE, MQTT, memory, sensors, and scheduler. See [troubleshooting.md](troubleshooting.md).
- **Migration Guide** (✓ v1.1.2): Converting traditional `loop()` code to Modest IoT patterns with before/after examples. See [migration.md](migration.md).
- **Example Template Generator**: CLI tool to generate skeleton device projects.

**Target**: v1.2.0+ (PlatformIO, Quick Start, Troubleshooting, and Migration complete in v1.1.2)

### Sensor/Actuator Expansion

Expand the component library (currently 21 sensors, 9 actuators) with additional IoT components: BMP280, INA219, SGP30/SGP40, BMA4000/LIS2DH12, VL53L0X, CAP1188, DC Motor Controller, Stepper Motor, Multi-channel Relay Board.

**Target**: v1.2.0+

### Component Enhancements

- **ConfigurationStore**: Schema migration, configuration profiles, backup/restore.
- **PowerManager**: Fuel gauge integration, charging status, power budget tracking.

**Target**: v1.3.0

### Process & Release

- **Automated Release Workflow**: Full test suite, example compilation, Doxygen generation, and registry publishing on tag push.
- **Code Coverage Reporting**: gcov/lcov integration with minimum threshold.
- **Dependency Update Monitoring**: Monitor MQTT library and ESP32 Arduino core updates.
- **Benchmark Table**: Publish measurable evidence of framework performance claims.

**Target**: v1.2.0+

---

## Deliberately out of scope

- TLS/SSL implementation in the framework (use ESP32's built-in `WiFiClientSecure`; W22 examples are complete).
- Targeted (per-instance) commands until a use case appears.
- Switchable sensor power rails and ADC/DMA sampling.
- Non-ESP32 targets. The ESP32 family (WROOM, S3, C3, C6, H2) covers the framework's target use cases: Wi-Fi + BLE + FreeRTOS + deep sleep + ample RAM/Flash. A hardware abstraction layer would require ~2,900 lines of new code for uncertain community demand. Re-evaluate if community demand emerges; STM32WB series is the most viable non-ESP32 target.

### Microcontroller Coverage Expansion

**Status**: Deferred — ESP32-only remains the strategic choice.

**Rationale**: The ESP32 family provides sufficient variety across price points and capabilities. Edge Framework dual-framework vision provides more value than multi-platform Nano support.

**Re-evaluation trigger**: If community demand for non-ESP32 support emerges (e.g., GitHub issues, forum requests), re-evaluate via a phased approach. STM32WB series (Wi-Fi + BLE + FreeRTOS) is the most viable non-ESP32 target. RP2040 requires external Wi-Fi/BLE modules. ESP8266 lacks BLE and has tight RAM/Flash constraints.

---

## Comparison with traditional development

Compares three points: a traditional Arduino `loop()` app, Modest IoT v1.2.0 (current with W46), and Modest IoT v1.3.0 (planned).

> **Note**: Comparison tables are replaced with each release — past comparisons are not archived. When v1.3.0 ships, this table is updated to compare v1.3.0 vs v1.4.0.

### Developer experience

| Aspect          | Traditional `loop()`                         | Modest IoT v1.2.0 (current)                                                 | Modest IoT v1.3.0 (planned)                                 |
|-----------------|----------------------------------------------|-------------------------------------------------------------------------------|----------------------------------------------------------------------|
| First build     | Works with the board package                 | All 26 examples compile in CI; 1 dependency (MQTT)                            | Same                                                                |
| Structure       | One growing `loop()` with `millis()` flags   | Sensors emit events, a `Device` mediates, actuators take commands             | Same, with event aggregation for composite patterns (W29)    |
| Adding a sensor | New globals, new timing variable, new branch | Subclass or reuse, register in the scheduler; per-sensor default periods; `PowerManager` for battery-aware sensors | Same, with event aggregation for composite patterns (W29)                  |
| Sampling rates  | Hand-written per-sensor timers               | Per-slot periods, minimums enforced, staggering handled                       | Same, with event aggregation for composite patterns (W29)                |
| Timing mistakes | Easy: a blocking call stalls everything      | Handler budgets documented; blocking HX711/ultrasonic removed; interrupt-driven sensors; `PowerManager` for configurable sleep modes | Same, with event aggregation for composite patterns (W29)          |
| Debugging       | `Serial.println` scattered                   | `MODEST_LOG_*`, stack high-water marks, sensor fault events, diagnostics, `firmwareVersion` in telemetry for fleet tracking | Same, with event aggregation for composite patterns (W29)         |
| Configuration   | Hardcoded credentials and intervals          | `ConfigurationStore` with ESP32 NVS, namespace isolation, type-safe access, read-only mode    | Same                                                               |
| Time handling   | `millis()` from boot                         | `TimeSynchronizationDriver` with NTP over Wi-Fi and RTC fallback for absolute time             | Same, with synchronized timestamps across devices (W30)                   |
| Networking      | Blocking connect/publish in `loop()`         | Separate network task, reconnect policy, keep-alive, TLS examples, `createSecureSocket()`, TLS/SSL as documented best practice     | Same, with event aggregation for composite patterns (W29)                      |
| Cloud-to-device | Hand-rolled callback                         | `Device::enqueueCommand` with `int32_t` payload for parameterized commands                   | Same, with event aggregation for composite patterns (W29)   |
| Offline data    | Lost when network unavailable                | `TelemetryBuffer` with LittleFS persistence for offline replay                                      | Same, with event aggregation for composite patterns (W29)       |
| Updates         | Manual reflashing required                   | OTA updates via `OtaUpdateHandler` component                                                    | Same                                                     |
| Testing         | Rarely possible off-device                   | 49 host test suites with CI, sanitizers enabled                               | Same                                                |
| Documentation   | README or inline comments                    | 30 ADRs with rationale, user stories with ADR cross-references, PlantUML class diagram, Doxygen API reference, versioned CHANGELOG, and cross-linked doc ecosystem | Same                                                |

### Performance

| Aspect                     | Traditional `loop()`              | Modest IoT v1.2.0 (current)                             | Modest IoT v1.3.0 (planned)                              |
|----------------------------|-----------------------------------|-----------------------------------------------------------|--------------------------------------------------------------------|
| Timing model               | Cooperative polling; jitter grows | Timer ISR plus sampler task; per-slot periods; interrupt-driven sensors | Same, with event aggregation for composite patterns (W29) |
| Sample period accuracy     | Depends on `loop()` duration      | Per-slot periods independent of slot count                | Same |
| Worst-case stall           | Any blocking call                 | Interrupt-driven HX711/ultrasonic; documented budgets; `PowerManager` handles sleep/wake transitions     | Same, with event aggregation for composite patterns (W29)          |
| Edge latency (button, PIR) | Up to one `loop()` pass           | Interrupt mode: task wake-up latency                      | Same, with event aggregation for composite event patterns (W29)    |
| Network isolation          | A slow publish blocks sensing     | Isolated in the network task; bounded queue policy, `TelemetryBuffer` for offline replay        | Same, with event aggregation for composite patterns (W29)        |
| Heap behaviour             | Usually static                    | `unique_ptr` ownership, drop-oldest policy; LittleFS reduces heap pressure via `TelemetryBuffer` | Same; pool if measurement justifies it (W18)   |
| CPU when idle              | Spins in `loop()`                 | Sleeps between ticks; tickless and deep sleep supported; `PowerManager` handles sleep/wake transitions | Same, with event aggregation for composite patterns (W29) |
| Energy                     | High unless hand-tuned            | Deep sleep: ~10µA; light sleep via tickless mode; battery monitoring enables adaptive sleep strategies and optimized sleep modes via `PowerManager` | Same, with event aggregation for composite patterns (W29)               |
| Time synchronization      | None; boot-relative only          | NTP-synchronized absolute time via `TimeSynchronizationDriver`; RTC fallback for deep-sleep devices | Same, with synchronized timestamps across devices (W30) |
| Secure communication      | Manual TLS setup if needed        | TLS examples provided; `createSecureSocket()` builder; `WiFiClientSecure` documented; TLS/SSL as standard practice   | Same, with event aggregation for composite patterns (W29)       |

### Resource consumption

| Resource                 | Traditional `loop()` | Modest IoT v1.2.0 (current)                                                               | Modest IoT v1.3.0 (planned)                           |
|--------------------------|----------------------|---------------------------------------------------------------------------------------------|----------------------------------------------------------------|
| Task stacks              | `loop()` task only   | 4096 B sampler + 8192 B network, configurable via `EngineConfiguration`; `PowerManager` adds minimal stack for sleep handling | Same defaults                                             |
| Queue and packets        | None                 | Queue of pointers + one heap packet per reading; bounded by queue policy; `TelemetryBuffer` adds LittleFS storage for offline replay | Same, with event aggregation for composite patterns (W29) |
| Flash                    | Smallest             | Framework, Wi-Fi, BLE driver; only 1 dependency (MQTT); ~30-40 KB saved by W46; plus `OtaUpdateHandler`, `TelemetryBuffer` | Same                                                                |
| Static RAM               | Small                | Slot table, per-object state; trigger table (4 slots); ConfigurationStore NVS cache; `TelemetryBuffer` adds LittleFS buffer | Same                                             |
| Heap fragmentation       | Low                  | Lower with explicit ownership; `unique_ptr` transfer; LittleFS reduces heap pressure        | Same; pool in Tier 5 if measured (W18)                              |
| Third-party dependencies | As needed            | Only MQTT — ArduinoJson removed by W46; ESP32Servo, LiquidCrystal_I2C, PubSubClient, DHT, Adafruit, and HX711 also removed | Same                                                                |
| Logging overhead         | User-controlled      | Zero when compiled out; UART cost when on                                                  | Same                                                           |
| Persistent storage       | Manual SPIFFS/LittleFS | ConfigurationStore uses ESP32 NVS with namespace isolation; `TelemetryBuffer` uses LittleFS for offline replay | Same, with event aggregation for composite patterns (W29)  |
| Security overhead        | None by default      | TLS examples provided; `NetworkClientSecure` usage guided; no framework-level TLS enforcement                                  | Same, with event aggregation for composite patterns (W29) |

---

## How to validate the comparison

Run the same three reference applications on the same ESP32 board, once per variant (plain `loop()`, Modest IoT v1.2.0, Modest IoT v1.3.0):

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

