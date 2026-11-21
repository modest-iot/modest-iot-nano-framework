# Changelog

All notable changes to the Modest IoT Nano Framework will be documented in this file.

## [Unreleased]

### Changed

#### Release Process — Benchmarks Update
- **docs/release.md**: Added `docs/benchmarks.md` to the release process as part of the version bump checklist. The benchmark table (version, date, platform) is now updated alongside the other version-managed files during each release preparation. Included in the Pre-Release Checklist, Documentation version references table, git add command, and Post-Release version bump section.

#### docs/benchmarks.md v1.2.1 Update
- **docs/benchmarks.md**: Updated version to 1.2.1, platform to ESP32 4.0.0, and added v1.2.1 column to all comparison tables. Added ESP32 4.0.0 BLE API migration data, CI-measured sketch sizes, code coverage (91.5%), and GitHub Actions Node.js 24 migration to the Framework Efficiency Claims table. Updated validation methodology to mention ESP32 4.0.0 and `huge_app` partition scheme.

#### Class Diagram — ESP32 4.0.0 BLE API Update
- **docs/class-diagram.puml**: Updated `BleConnectivityDriver` class to reflect ESP32 4.0.0 BLE API changes: member types changed from pointers to values (`BLEServer*` → `BLEServer`, `BLECharacteristic*` → `BLECharacteristic`), added missing members (`bleService`, `initialized`, `onDataReceivedHandler`), and added new methods (`handlePeerConnected`, `handlePeerDisconnected`, `disconnect`, `getSignalStrengthInDbm`, `handleDataReceived`). Title updated to v1.2.1.

#### GitHub Actions Node.js 24 Migration
- **.github/workflows/compile-examples.yml**: Updated `actions/checkout@v4` → `@v5` and `arduino/setup-arduino-cli@v1` → `@v2` to natively support Node.js 24, eliminating the deprecation warning.
- **.github/workflows/host-tests.yml**: Updated `actions/checkout@v4` → `@v5` (both jobs) and `actions/upload-artifact@v4` → `@v4.6.0` for Node.js 24 compatibility.
- **.github/workflows/lint.yml**: Updated `actions/checkout@v4` → `@v5` and `arduino/arduino-lint-action@v1` → `@v3` for Node.js 24 compatibility. Removed the `ACTIONS_RUNNER_NODE_VERSION: 24` environment variable as v3 natively supports Node.js 24.
- **.github/workflows/release.yml**: Updated `actions/checkout@v4` → `@v5` (both jobs), `actions/upload-artifact@v4` → `@v4.6.0`, `actions/download-artifact@v4` → `@v4.3.0`, and `softprops/action-gh-release@v2` → `@v3` for Node.js 24 compatibility.
- **.github/workflows/dependency-status.yml**: Updated `actions/checkout@v4` → `@v5` for Node.js 24 compatibility.

## [1.2.1] - 2026-11-21

### Fixed

- **tests/run_coverage.sh**: Added `--exclude` patterns to `lcov --capture` to filter out GCC standard library headers (Homebrew GCC, Xcode SDK, Linux system headers) from the coverage report. Previously, system headers compiled with coverage flags appeared in the HTML report, inflating the coverage numbers with irrelevant data.
- **docs/coverage.md**: Updated coverage numbers from 89.6% to 91.5% line coverage and 86.1% to 84.4% function coverage to reflect the GCC header exclusion. Added explanation of the `--exclude` mechanism.
- **docs/quick-start.md**: Updated version references from 1.1.2 to 1.2.0 in three locations: Arduino Library Manager version text, VS Code PlatformIO `lib_deps` + download message, and CLion PlatformIO `lib_deps`.
- **CHANGELOG.md**: Updated Code Coverage Reporting section coverage number from 89.6% to 91.5% line coverage.

### Changed

#### ESP32 4.0.0 BLE API Migration
- **src/BleConnectivityDriver.h/cpp**: Migrated from ESP32 3.x BLE API (`BLEDevice.h`/`BLEServer.h`) to ESP32 4.0.0 unified `BLE.h`. All BLE API calls updated: `BLE::begin()` instead of `BLEDevice::init()`, `BLE::createServer()` returns reference instead of pointer, lambda callbacks instead of callback classes, `BLEProperty::Notify` enum instead of `BLECharacteristic::PROPERTY_NOTIFY`, and `chr.getValue(&length)` replaces `chr.getData()`/`chr.getLength()`. ESP32 4.0.0 is now the minimum supported version for BLE functionality.
- **tests/fakes/BLE.h**: Consolidated all BLE mock types (BLEProperty, BLEPermission, BLEConnInfo, BTStatus, BLECharacteristic, BLEService, BLEServer, BLEAdvertising, BLE namespace) into a single file mirroring the production `BLE.h` structure. Renamed from `BLEDevice.h` to `BLE.h` to match the original header name.
- **tests/BleConnectivityDriver/test.cpp** and **tests/BleGatewayClient/test.cpp**: Updated all BLE API usage to match ESP32 4.0.0 value-based API.
- **README.md**: Added ESP32 4.0.0 requirement note to the BLE section.
- **docs/quick-start.md**: Added ESP32 4.0.0 requirement to the hardware prerequisites.
- **.github/workflows/compile-examples.yml**: Initialized arduino-cli config with ESPressif board package index URL (`https://espressif.github.io/arduino-esp32/package_esp32_index.json`) using `arduino-cli config init --additional-urls` before installing ESP32 core 4.0.0. The default Arduino index does not include ESP32 4.0.0; the ESPressif-specific index is required. Pinned ESP32 Arduino core to version 4.0.0 (`arduino-cli core install esp32:esp32@4.0.0`) to ensure CI compiles all examples against the required BLE API version. Added `--build-property build.partitions=huge_app --build-property upload.maximum_size=3145728` to the compile command (singular `--build-property`, one per property) to use the "Huge APP" partition scheme (3 MB app / 1 MB SPIFFS), as ESP32 4.0.0 changed the default partition scheme from "no_ota" (2 MB app) to "default" (1.2 MB app / 1.5 MB SPIFFS), which caused examples to exceed the Flash limit.

## [1.2.0] - 2026-11-20

### Fixed

- **.github/workflows/release.yml**: Doxygen logo image now displays correctly in the generated HTML. The README.md logo path (`assets/branding/logo.png`) is relative to the project root, but Doxygen outputs HTML to `docs/build/html/`. The release workflow now copies `assets/branding/logo.png` to `docs/build/html/assets/branding/` after Doxygen generation so the relative path resolves correctly.

### Added

#### Automated Release Workflow
- **.github/workflows/release.yml**: Automated release workflow triggered on version tag push (e.g., `v1.2.0`). Validation (host tests, coverage, compile examples, lint) is performed by the existing CI workflows when the release branch is pushed, ensuring all checks pass before merging to `main` and `develop`. The tag-triggered workflow assumes validation passed and focuses on generating Doxygen HTML API reference and creating a GitHub Release with changelog body and artifacts (library ZIP, Doxygen HTML ZIP). Doxygen generation runs first; the release job depends on Doxygen and uploads both the library source ZIP (following `library.json` export rules) and the Doxygen HTML ZIP as release assets.
- **docs/release.md**: Complete release process documentation covering the Git Flow Helper Plugin process (Release Start, Release Publish, manual merge, Release Finish), two-phase validation (release branch CI via existing workflows, then tag workflow for artifacts), pre-release checklist with CI verification gate, version bump instructions for all 7 version-managed files (4 core + 3 documentation), registry publishing (Arduino Library Manager auto-index, PlatformIO manual/automated), post-release steps, and troubleshooting.
- **CONTRIBUTING.md**: Updated Version Management section to document all 7 version-managed files (4 core + 3 documentation) with a note that other docs/ files contain historical version references that should not be updated during a release.

#### Code Coverage Reporting
- **tests/run_coverage.sh**: Coverage runner script that compiles all 49 test suites with gcov instrumentation (`-fprofile-arcs -ftest-coverage`), runs them, and generates an HTML coverage report using lcov's `genhtml`. Supports configurable minimum coverage threshold (default 80%) via `MIN_COVERAGE` environment variable, and optional enforcement via `ENFORCE_COVERAGE=1`. Runs without sanitizers to avoid gcov interference. Requires GCC (not Clang) and the lcov package. Includes a gcov wrapper that suppresses `.gcov` annotated source files from being written to the project root. Platform-specific files (OneWireTemperatureSensor.cpp, DhtSensor.cpp, UltrasonicSensor.cpp, LoadCellAmplifier.cpp, TelemetryBuffer.cpp, PowerManager.cpp, GatewayClient.h) are excluded from the coverage report and deleted from the build directory before `genhtml` runs, so the HTML report reflects only testable code (91.5% line coverage).
- **src/Device.cpp**: Added `LCOV_EXCL_START/END` markers to exclude FreeRTOS-specific functions from coverage: ISR handler, task creation, queue operations, command marshaling, diagnostics collection, power management, and engine stop. These ~200 lines cannot execute in host tests.
- **.github/workflows/host-tests.yml**: Added `coverage` job that runs on Ubuntu, installs lcov and bc, executes the coverage script, uploads the HTML report as a `coverage-report` artifact (14-day retention), and posts the coverage summary to the GitHub step summary.
- **.gitignore**: Added `*.gcov`, `*.gcda`, `*.gcno` to prevent accidental commits of gcov coverage artifacts.
- **docs/coverage.md**: Documentation covering local execution, CI integration, coverage threshold configuration, tool selection rationale, report interpretation, expected gaps, platform-specific file exclusions, and limitations.

#### Dependency Update Monitoring
- **.github/dependabot.yml**: Dependabot configuration for automated GitHub Actions version monitoring. Checks monthly for updates to `actions/checkout`, `actions/upload-artifact`, `arduino/arduino-lint-action`, and `arduino/setup-arduino-cli`. Opens pull requests with conventional commit prefix (`ci`) and labels (`dependencies`, `ci`) when new versions are available.
- **.github/workflows/dependency-status.yml**: Weekly workflow (Monday 09:00 UTC) that checks MQTT and ESP32 Arduino core versions via the GitHub Releases API. Compares MQTT against the minimum version in `library.properties`, updates `.badges/mqtt.json` and `.badges/esp32.json`, and commits the changes. Badges are displayed in README.md via shields.io endpoint badges. Can also be triggered manually via `workflow_dispatch`.
- **.badges/mqtt.json**: Shields.io endpoint badge JSON for MQTT library version status. Green when up-to-date, orange when a newer release is available.
- **.badges/esp32.json**: Shields.io endpoint badge JSON for ESP32 Arduino core version. Shows the latest release version (informational, since CI always installs latest).
- **README.md**: Added two dynamic shields.io badges for MQTT and ESP32 dependency status.
- **docs/dependencies.md**: Documentation listing all external dependencies (MQTT library, ESP32 Arduino core, GitHub Actions) with update processes. Dependabot handles GitHub Actions automatically; MQTT and ESP32 are monitored via the weekly Dependency Status workflow with shields.io badges.

#### Benchmark Table
- **docs/benchmarks.md**: Performance comparison table with 9 metrics comparing traditional `loop()`, Modest IoT v1.1.2, and v1.2.0. Covers Flash, RAM, latency, throughput, queue depth, stack usage, heap, serialization time, and power consumption. Values derived from source code analysis (exact constants and configuration) with a validation methodology for future hardware-measured measurements. Key findings: ~220-230 KB Flash (v1.2.0), ~40-50 KB RAM, zero heap allocation in critical path, ~30-40 KB Flash saved by ArduinoJson removal (W46), and ESP-IDF variant planned at ~180 KB (~73% reduction).
- **README.md**: Added benchmarks.md to Documentation Map and updated Future Directions to link to the published document.

#### HomeSecurityDevice Example — High-Priority Lane Demonstration
- **examples/HomeSecurityDevice/HomeSecurityDevice.ino**: Extended with MQTT gateway integration and two TelemetryPackage subclasses to demonstrate the high-priority telemetry lane (W32). `SecurityStatusTelemetryPackage` uses `enqueueTelemetryPayload()` for routine status updates (door closed, motion stopped, leak cleared). `SecurityAlertTelemetryPackage` uses `enqueueHighPriorityPayload()` for critical alerts (water leak, door opened), with `"priority": "high"` in the JSON envelope. The `on(Event)` handler clearly shows which events go to which queue.
- **tests/TelemetryPackage/test.cpp**: Added 3 golden tests (tests 16–18) for `SecurityStatusTelemetryPackage` (routine, no priority field), `SecurityAlertTelemetryPackage` with `markAsHighPriority()` (priority field present), and `SecurityAlertTelemetryPackage` without `markAsHighPriority()` (no priority field).

#### Schema Registry (W33)
- **TelemetryPackage per-package schema version**: Each TelemetryPackage subclass can declare its own schema version via `schemaVersionValue`. The `setVersionMetadata()` method now falls back to the global `Device::setTelemetrySchemaVersion()` only when the per-package version is not set, enabling safe telemetry schema evolution.
- **DiagnosticsTelemetryPackage**: Declares `static constexpr const char* SCHEMA_VERSION = "1.0"` and sets `schemaVersionValue` in the constructor.
- **Examples updated**: `SmartBandTelemetryPackage` (SmartBandMqtt), `SecurityStatusTelemetryPackage` and `SecurityAlertTelemetryPackage` (HomeSecurityDevice) now declare per-package schema versions.
- **tests/TelemetryPackage/test.cpp**: Added 2 new tests (tests 20–21): per-package schema version priority over global schema, and fallback to global schema when per-package is not set. Updated DiagnosticsTelemetryPackage golden test (test 19) to include schema version in expected JSON output.

#### Event Aggregator (W31)
- **src/EventAggregator.h/cpp**: New component for composite event pattern matching with time-windowed detection. Implements `EventHandler` to intercept events from sensors. Captures each event with its timestamp and sourceId in a fixed-size circular buffer (32 captured events, zero heap allocation). Supports up to 4 composite patterns with AND logic (all required events must occur within the time window). When a pattern matches, emits a composite event via the output handler.
- **EventAggregator::addAllMatchPattern()**: Three overloads — the original accepts event identifiers only (matches any source), the second accepts per-position source ID arrays, and the third adds a triggering condition function. Use `EventAggregator::ANY_SOURCE` for a position to match any source, or a specific pin number to require that exact source. This is essential when multiple sensors share the same event identifier (e.g., DATA_READ from LoadCellAmplifier vs. DhtSensor). All `addAllMatchPattern()` overloads use AND logic: all required events must occur within the time window.
- **EventAggregator::addAnyMatchPattern()**: Two overloads for OR logic — the first accepts event identifiers only, the second adds per-position source ID filtering. Unlike `addAllMatchPattern()` which requires all events to occur, `addAnyMatchPattern()` emits the composite event when **any one** of the listed events occurs within the time window. For example, "motion OR door opened within 10 seconds" triggers an alert when either event occurs. OR and AND patterns can coexist in the same aggregator, sharing the 4-pattern slot limit.
- **EventAggregator::PatternLogic**: Enum (`And` / `Or`) that determines the matching strategy for each pattern. `addAllMatchPattern()` sets `PatternLogic::And`, `addAnyMatchPattern()` sets `PatternLogic::Or`. The matching logic in `on(Event)` checks the pattern's logic type and applies the appropriate strategy.
- **EventAggregator::PatternCondition**: Function pointer type (`bool (*)(int compositeEventId)`) for triggering conditions. The developer defines a function that receives the composite event ID as context and checks sensor values at the moment a pattern matches (e.g., "is cargo bay empty?"). If the condition returns `false`, the composite event is suppressed even though the event identifiers matched. The compositeEventId allows a single condition function to handle multiple patterns differently. Pass `nullptr` to always emit. The condition runs on the interrupt context, so keep it short.
- **EventAggregator::ANY_SOURCE**: Public constant (value -1) for use in source ID filter arrays. Makes pattern definitions self-documenting and avoids magic numbers.
- **EventAggregator::DEVICE_SOURCE_ID**: Public constant (value 0) that is automatically attached as the `sourceId` of every composite event. Sensors use positive pin numbers as sourceId, so this constant distinguishes composite events from sensor events. Enables source-aware pattern matching in nested aggregators: the second aggregator can filter by `DEVICE_SOURCE_ID` to match only composite events from the first aggregator, not raw sensor events.
- **EventAggregator::removePattern()**: Removes a registered pattern by its composite event ID and frees the slot for reuse. Returns `true` if the pattern was found and removed, `false` if not found. After removal, the freed slot can be reused by a subsequent `addAllMatchPattern()` call. This enables dynamic rule management for the W29 Rules Engine.
- **EventAggregator::cleanupPatterns()**: Removes all registered patterns in one call so developers do not need to track and remove each pattern individually. Typically called from the Device destructor before `stopAsynchronousEngine()`. After this call, no composite events are emitted until new patterns are registered.
- **EventAggregator::setOutputHandler()**: Sets the `EventHandler` that receives composite events when patterns match.
- **examples/IntrusionDetector/IntrusionDetector.ino**: Example focused on **standard patterns** that always emit when event identifiers match within the time window. Demonstrates two composite patterns: intrusion alert (motion AND door opened within 10 seconds triggers buzzer and LED) and leak near entry (water leak AND door opened within 30 seconds triggers buzzer). Uses the basic `addAllMatchPattern()` overload without condition functions. Sensors dispatch events to the EventAggregator instead of directly to the Device, illustrating the event chaining pattern: sensors → EventAggregator → Device::on(Event). Demonstrates proper housekeeping with `cleanupPatterns()` in the destructor to remove registered patterns before stopping the asynchronous engine.
- **examples/ColdChainMonitor/ColdChainMonitor.ino**: IIoT example focused on **conditional patterns** where a triggering condition function determines whether the composite event is emitted. Demonstrates refrigerated truck cargo bay monitoring with three composite patterns: energy save (door opened AND weight change within 30 seconds, **only if** cargo bay is empty — uses `PatternCondition` to check LoadCellAmplifier weight), unauthorized access (door opened AND motion stopped within 60 seconds, standard pattern), and loading/unloading active (weight change AND door opened AND motion detected within 120 seconds, standard pattern). Uses source-aware patterns to distinguish DATA_READ from LoadCellAmplifier (pin 4) from other sensors sharing the same event identifier. Uses DoorWindowSensor, PassiveInfraredSensor, LoadCellAmplifier (hybrid scheduling), DhtSensor, RelayActuator, Buzzer, and Led. Demonstrates proper housekeeping with `cleanupPatterns()` in the destructor to remove registered patterns and clear the static condition function pointer before stopping the asynchronous engine.
- **examples/PharmaceuticalCleanRoom/PharmaceuticalCleanRoom.ino**: IIoT example focused on **nested EventAggregator chains** where composite events from one aggregator feed into another, forming a multi-level pattern detection chain. Demonstrates pharmaceutical clean room monitoring with 3 aggregator levels: Level 1 detects environmental deviations (pressure breach using AND pattern, environmental anomaly using OR pattern via `addAnyMatchPattern()`), Level 2 correlates deviations into contamination risks (pressure breach AND environmental anomaly), and Level 3 triggers batch quarantine. Uses `EventAggregator::DEVICE_SOURCE_ID` for source filtering between levels to match only composite events from upstream aggregators. Uses PthSensor (pressure, I2C), DhtSensor (temperature), GasSensor (VOC/particles), DoorWindowSensor, RelayActuator (HVAC lockdown), Buzzer, and Led. Demonstrates `removePattern()` for dynamic pattern removal and `cleanupPatterns()` on all aggregator levels in the destructor.
- **tests/EventAggregator/test.cpp**: 43 host tests covering single event patterns, two-event AND patterns within/outside time windows, three-event patterns, multiple patterns, captured event cleanup, circular buffer overflow, no patterns registered, pattern array full, invalid parameters, no output handler, reverse event order, time window boundary conditions, duplicate event identifiers, repeated pattern matches, shared events across patterns, source-aware exact match, source-aware wrong source rejection, source-aware wildcard, mixed exact and wildcard sources, two patterns with different sources on the same identifier, backward compatibility with the original API, condition function receives compositeEventId (emits), condition function returns false (suppresses), nullptr condition (always emits), condition-based sensor value check (practical ColdChainMonitor scenario), shared condition function handling multiple patterns via compositeEventId, remove existing pattern (no longer fires), remove non-existent pattern (returns false), remove and re-add pattern (slot reuse), remove middle pattern (array compaction), cleanup all patterns (removes all and allows re-registration), OR pattern single event triggers, OR pattern neither event fires, OR pattern with source filtering, OR pattern time window expiry, mixed AND and OR patterns coexisting, OR pattern array full, OR pattern remove and cleanup (slot reuse with AND pattern), nested aggregators basic chain (two aggregators chained via setOutputHandler), nested aggregators source filtering by DEVICE_SOURCE_ID (second aggregator matches only composite events, rejects sensor-source events), and nested aggregators three-level chain (three aggregators chained sequentially).

#### Remote Diagnostics (W41)
- **src/DiagnosticsTelemetryPackage.h/cpp**: New TelemetryPackage subclass that wraps `DiagnosticsSnapshot` for on-demand diagnostics telemetry. When the cloud sends a diagnostics command (commandId = 300), the device creates this package from `Device::collectDiagnostics()` and enqueues it via the standard telemetry queue. The JSON output contains all 14 health metrics (missedTicks, packetsEnqueued, highPriorityEnqueued, sendsSucceeded, freeHeapBytes, etc.) plus envelope metadata.
- **Device::handleInternalCommand(Command)**: New private method that handles framework-internal commands (diagnostics, commandId = 300) before dispatching to `handle()`. Returns `true` if consumed, `false` otherwise. This keeps `handle()` purely for developer-defined custom commands.
- **Device::handle(Command)**: Restored as a pure no-op override. Derived classes override this for their custom commands. Framework-internal commands are handled separately by `handleInternalCommand()` in `processPendingCommands()`, so the developer's `handle()` receives only custom commands.
- **Device::processPendingCommands()**: Now calls `handleInternalCommand()` first for each command. If not consumed, dispatches to `targetHandler->handle()` for developer-defined custom commands.
- **tests/TelemetryPackage/test.cpp**: Added golden test (test 19) for `DiagnosticsTelemetryPackage` verifying all 14 fields are present with correct values in the JSON output.

#### High-Priority Telemetry Lane (W32)
- **Device::enqueueHighPriorityPayload()**: Pushes a high-priority payload into a separate bounded FreeRTOS queue (4 slots). The network task checks this queue first (non-blocking) before polling the standard telemetry queue, ensuring critical alerts are transmitted before routine telemetry.
- **TelemetryPackage::markAsHighPriority()**: Marks a package as high-priority, adding `"priority": "high"` to the JSON envelope metadata for edge-side routing.
- **TelemetryPackage::isHighPriority()**: Accessor returning whether the high-priority flag is set.
- **DiagnosticsSnapshot extension**: New fields `highPriorityEnqueued`, `highPriorityDropped`, and `highPriorityQueueHighWaterMark` to monitor high-priority queue pressure and dropped alerts.

### Changed

#### Device Queue Management Refactoring
- **Device::assignEnvelopeMetadata()**: Extracted shared helper for timestamp, sequence, and version assignment, used by both `enqueueTelemetryPayload()` and `enqueueHighPriorityPayload()`.
- **Device::drainTelemetryQueue()**: Extracted static helper for queue drain-delete-null pattern in `stopAsynchronousEngine()`. Made `static` since it operates only on its parameter.

#### Examples — Destructor Safety
- **9 Device subclasses** replaced `= default` destructors with proper `stopAsynchronousEngine()` calls: BleVitalsTransmitter, ClimateDataUploader, EquipmentHealthMonitor, IndoorAirQualityDisplay, MqttTlsDevice, OtaUpdateDevice, PipeTemperatureMonitor, RemoteLedController, SmartBandMqtt. Ensures the engine stops before sensor/actuator member destruction.

#### StatusIndicator Example Refinement
- **examples/StatusIndicator/StatusIndicator.ino**: Added `start()` method for post-construction initialization (firmware version), replacing inline `Device::setFirmwareVersion()` in `setup()`. Added proper destructor calling `stopAsynchronousEngine()` before member destruction. Reorganized constants into Doxygen groups (Engine Configuration, Device Configuration, Serial Output Configuration, Hardware Configuration) and extracted firmware version as named constant. Simplified `setup()` to: serial init → construct → `start()` → log.

#### Quick Start Guide Enhancement
- **docs/quick-start.md**: Added `HomeSecurityDevice` to the Recommended Examples list with a description of the high-priority telemetry lane (`enqueueTelemetryPayload()` vs `enqueueHighPriorityPayload()`), directing users ready for advanced features to this example after completing the StatusIndicator onboarding.

#### JsonParser Framework Utility
- **src/JsonParser.h/cpp**: Native JSON parser for flat key-value payloads with zero heap allocation. Five static methods: `parseInt()`, `parseInt32()`, `parseFloat()`, `parseBool()`, and `parseString()`. Private `findJsonValue()` helper eliminates duplicated key-finding logic across all parse methods. Replaces file-scope static functions in MqttGatewayClient with a framework-wide utility accessible to developers building custom devices.
- **tests/JsonParser/test.cpp**: 20 host tests covering all five parse methods, null input handling, missing keys, buffer overflow protection, and the MQTT command format (parseInt + parseInt32).

#### MQTT QoS and Retained Messages (W34)
- **MqttGatewayClient**: Added configurable Quality of Service (QoS 0, 1, or 2) for both publish and subscribe operations, and retained message support for last-known-good telemetry. New constructor parameters `publishQos`, `subscribeQos`, and `retainedFlag` with backward-compatible defaults (QoS 0, non-retained). New setter/getter methods: `setPublishQoS()`, `getPublishQoS()`, `setSubscribeQoS()`, `getSubscribeQoS()`, `setRetained()`, `getRetained()`.
- **MqttGatewayClient**: Migrated from file-scope static JSON parser functions to `JsonParser::parseInt()` and `JsonParser::parseInt32()` for incoming MQTT command parsing.

#### TelemetryWriter Abstraction (W46)
- **src/TelemetryWriter.h**: Abstract interface for telemetry field serialization with pure virtual `field()` overloads for `int32_t`, `float`, `bool`, and `const char*`. Decouples telemetry packages from the JSON serialization format, enabling future format writers (CBOR, MessagePack, binary).
- **src/JsonTelemetryWriter.h/cpp**: Native `snprintf`-based JSON writer implementation using a 512-byte fixed stack buffer. Zero heap allocation per serialization, automatic comma handling, buffer overflow detection, and string escaping for special characters.
- **TelemetryPackage::serializeToBuffer()**: Single-call serialization pipeline that encapsulates envelope metadata, data fields, finalization, overflow check, and memcpy. Replaces the 6-step `JsonTelemetryWriter` manual sequence in each gateway client.

#### GasSensor Enum-to-String Utilities
- **GasSensor::gasTypeName()**: Static method returning human-readable names for all 11 `GasType` enum values (LPG, CH4, CO, Alcohol, Propane, H2, Smoke, NH3, Benzene, Toluene, Acetone).
- **GasSensor::operatingModeName()**: Static method returning display names for all 3 `OperatingMode` enum values (RAW, CALIBRATING, ACCURATE).

#### ArduinoJson Dependency Removed (W46)
- **TelemetryPackage.h**: Replaced `serialize(JsonDocument&)` with `write(TelemetryWriter&)` pure virtual method. Removed `toDocument()` method, simplified `toJsonString()` to use `JsonTelemetryWriter`. Eliminated `JsonDocument` type from the public API.
- **MqttGatewayClient.cpp**: Replaced `JsonDocument` with `serializeToBuffer()` for telemetry serialization. Replaced `deserializeJson()` with `JsonParser::parseInt()`/`JsonParser::parseInt32()` for command extraction. Removed `#include <ArduinoJson.h>`.
- **BleGatewayClient.cpp**: Replaced `JsonDocument` with `serializeToBuffer()` for telemetry serialization. Removed `#include <ArduinoJson.h>`.
- **HttpGatewayClient.cpp**: Replaced `JsonDocument` with `serializeToBuffer()` for telemetry serialization. Removed `#include <ArduinoJson.h>`.
- **library.properties**: Removed `ArduinoJson >=7.0.0` from `depends=`. Only MQTT >=2.5.3 remains as an external dependency.
- **library.json**: Removed `ArduinoJson >=7.0.0` from dependencies. Only MQTT >=2.5.3 remains.

#### API Improvements
- **GatewayClient.h**: Renamed parameter `abstractTelemetryPayload` to `telemetryPackage` in `sendTelemetryRecord()` declaration.
- **MqttGatewayClient.h/cpp**: Fixed `mqttMessageCallback` signature from `const String&` to `String&` to match MQTT Things library callback typedef.
- **TelemetryWriter/JsonTelemetryWriter**: Parameter naming convention `v` → `value` across all `field()` overloads, with "The [description]" Doxygen format.

#### Examples Migrated
- **SmartBandMqtt, BleVitalsTransmitter, AirQualityMonitor, CropMonitoringStation, ClimateDataUploader, MqttTlsDevice**: All 6 examples with TelemetryPackage subclasses migrated from `serialize(JsonDocument&)` to `write(TelemetryWriter&)`. Replaced `doc["key"] = value` with `writer.field("key", value)`.
- **Telemetry field type consistency**: Changed all `int` member variables in TelemetryPackage subclasses to `int32_t` to match `TelemetryWriter::field()` overloads and avoid platform-dependent ambiguity (where `int` differs from `int32_t` on some architectures like x86_64 Linux). Affected examples: **SmartBandMqtt** (`heartBeatsPerMinute`), **BleVitalsTransmitter** (`heartBeatsPerMinute`), **AirQualityMonitor** (`operatingMode`), **CropMonitoringStation** (`soilMoisture`). Developers using these examples as templates should update any custom `int` fields to `int32_t` when upgrading.
- **AirQualityMonitor**: Replaced custom `getGasTypeName()` function with `GasSensor::gasTypeName()`. Replaced hardcoded operating mode strings with `GasSensor::operatingModeName()`.

#### Tests Migrated
- **7 TelemetryPackage test subclasses** across 5 test files migrated to `write(TelemetryWriter&)` API.
- **15 golden tests** rewritten for insertion-order JSON output (envelope metadata fields first, then data fields).
- **tests/fakes/ArduinoJson.h** (253 lines) deleted. Replaced with `tests/fakes/TelemetryWriter.h` simple mock.

### Changed

#### CI Workflow — Coverage Job
- **.github/workflows/host-tests.yml**: Added a separate `coverage` job alongside the existing `host-tests` matrix job. The coverage job runs on Ubuntu only (gcov requires GCC), installs lcov and bc, and generates an HTML coverage report. The existing host-tests job continues to run with sanitizers on both Ubuntu and macOS, unchanged.

### Removed

- **ArduinoJson dependency**: Eliminated ~30-40 KB Flash usage and per-serialization heap allocation. The framework now has only one external dependency (MQTT >=2.5.3).

## [1.1.2] - 2026-11-17

### Added

#### PlatformIO Support
- **library.json**: PlatformIO Registry manifest for `lib_deps` installation. Includes name, version, dependencies (ArduinoJson >=7.0.0, MQTT >=2.5.3), frameworks (`arduino`), platforms (`espressif32`), keywords, author, license, and repository URL. Enables dual registry support alongside Arduino Library Manager.

#### Documentation
- **docs/quick-start.md**: Step-by-step onboarding guide with two parallel routes — Simulation (Wokwi, no hardware required) and Hardware (ESP32 board with three toolchain paths: Arduino IDE, VS Code with PlatformIO, CLion with ESP BSP). Covers prerequisites, installation, first project with StatusIndicator example, wiring, upload, and serial output verification.
- **docs/troubleshooting.md**: Comprehensive troubleshooting reference covering 10 issue categories: serial output, board detection, upload failures, Wi-Fi, BLE, MQTT, memory (stack/heap), sensors, scheduler, and telemetry queue. Each section uses symptom-cause-solution format with diagnostic code examples.
- **docs/migration.md**: Step-by-step migration guide with 5 patterns: `delay()` → events, `millis()` → scheduler, blocking sensors → non-blocking, globals → Device mediator, and monolithic `loop()` → separated concerns. Each pattern includes before/after code examples and a migration checklist.

### Changed

#### Documentation Structure
- **docs/improvements.md**: Pruned from internal planning document to adopter-facing summary (~210 lines). Removed interdependency graph, detailed implementation paths, and priority tables. Kept comparison tables, themed improvement summaries, and "deliberately out of scope" section.
- **docs/evolution.md**: Minor pruning of outdated content — removed W2 dependency inventory table and replaced W18 deferred code snippets with "superseded by W46" cross-reference.
- **docs/vision.md**: Moved to private storage (`.junie/private/`) to protect speculative Edge Framework details and timeline commitments from becoming public liabilities.
- **README.md**: Added Quick Start, Troubleshooting Guide, and Migration Guide rows to the Documentation table. Removed `docs/vision.md` reference.

## [1.1.1] - 2026-11-16

### Changed

#### Examples
- **WeatherStation**: Added adaptive power management — starts in `PowerMode::Periodic` and switches to `PowerMode::DeepSleep` when low battery is detected, with RTC-persisted low-battery state across reboots.
- **ClimateDataUploader**: Added `BearerTokenParser` utility class for isolated token extraction, improved error handling in `start()` method with TelemetryBuffer initialization check and gateway client cleanup on token failure.
- **All examples**: Added `Device::setFirmwareVersion()` with realistic manufacturer-style model numbers and log prefix constants for consistent serial output across all 23 examples.

### Added

#### Community Standards
- **ACCESSIBILITY.md**: Accessibility statement, reporting guidance, and known limitations following W3C and Open Source Guides best practices.
- **CODE_OF_CONDUCT.md**: Contributor Covenant Code of Conduct v2.0 with GitHub issue-based enforcement.
- **SECURITY.md**: Security policy and vulnerability reporting process with realistic response times for a solo-maintained project.
- **.github/ISSUE_TEMPLATE/**: Standardized issue templates — bug report, feature request, and accessibility report.
- **.github/pull-request-template.md**: Standardized PR template with pre-submission checklist from CONTRIBUTING.md.
- **CONTRIBUTING.md**: Updated Code of Conduct link to local `CODE_OF_CONDUCT.md` file.
- **README.md**: Added Project Standards section linking to all community standards files.

#### CI Workflows
- All three workflows (host-tests, compile-examples, lint) now trigger on `feature/*` branches for early error detection on feature branches.

#### Testing — 11 New Test Suites (36 → 47 total)
- **Led test**: Digital LED on/off/toggle commands, state tracking, parent propagation
- **PassiveInfraredSensor test**: PIR motion detection, motion started/stopped events, propagation suppression
- **PulseSensor test**: Heart rate detection from analog signal, moving average filter, threshold events
- **Hd44780Driver test**: HD44780 4-bit I²C protocol, initialization sequence, command/data writes
- **Ssd1306Driver test**: SSD1306 I²C protocol, display initialization, frame buffer commands
- **WiFiConnectivityDriver test**: Wi-Fi STA connection, reconnection retry, status enumeration, signal strength
- **BmeEnvironmentalSensor test**: Bosch BME280/BME680 compensation formulas, calibration data parsing, shared base class
- **UltrasonicSensor test**: HC-SR04 distance measurement, trigger/echo timing, change detection
- **LoadCellAmplifier test**: HX711 load cell amplifier, weight calculation, tare, power down/up cycle
- **GeolocationSensor test**: GPS NMEA parsing, GGA/RMC sentences, coordinate conversion, location change detection
- **Device test**: Device base class, engine initialization, scheduler integration, telemetry pipeline
- Added `HardwareSerial` fake class to `tests/fakes/Arduino.h` with `feed()`, `clear()`, `read()`, `available()`, and `peek()` methods for serial sensor testing
- Added `SERIAL_8N1` constant to `tests/fakes/Arduino.h` for GPS serial configuration
- Created `tests/fakes/HardwareSerial.h` stub for `GeolocationSensor.h` include compatibility

### Fixed

- **GeolocationSensor**: Restored `transmitGpioPin` field and `getTransmitGpioPin()` getter that identify the UART TX GPIO pin for event source identification; previously removed due to `-Wunused-private-field` warning
- **EngineConfig test directory**: Renamed from `tests/EngineConfig/` to `tests/EngineConfiguration/` to match the actual struct name `EngineConfiguration`
- **LoadCellAmplifier test**: Fixed infinite loop in power down/up test by setting `fakeDigitalReadValue = LOW` before `tare()` call to simulate HX711 data-ready state
- **CMakeLists.txt**: Added `CMAKE_RUNTIME_OUTPUT_DIRECTORY`, `CMAKE_ARCHIVE_OUTPUT_DIRECTORY`, and `CMAKE_LIBRARY_OUTPUT_DIRECTORY` settings to organize build artifacts within the build folder structure
- **CMakeLists.txt**: Updated stale references from `test_EngineConfig` to `test_EngineConfiguration` (target name, source path, `add_test` name, `run_tests` DEPENDS list)
- **tests/run_all.sh**: Added missing test suites (PowerManager, TelemetryBuffer, OtaUpdateHandler, GeolocationSensor) that existed as test directories and CMake targets but were never executed by the runner

### Project Structure

- **Build folder consolidation**: Consolidated all build artifacts under single `build/` directory with subdirectories (`build/arduino/`, `build/test/`) instead of separate `cmake-build-*` directories at root level
- **.gitignore cleanup**: Removed redundant `/cmake-build-debug/`, `/cmake-build-arduino/`, `/cmake-build-test/` lines; kept single `/build/` entry covering all build output
- **Logo optimization**: Resized `assets/branding/logo.png` from 756×707 (589 KB) to 214×200 (64 KB), rendered at 100px height in README per Yegor256 "Elegant READMEs" guideline — logo is a subtle branding accent, not the dominant visual
- **README.md**: Updated logo `<img>` tag from `width="400" height="374"` to `height="100"` only, letting browser scale width proportionally

### Documentation

- Updated test count from 36 to 47 in README.md, docs/improvements.md, docs/user-stories.md, docs/vision.md, docs/adrs.md, and docs/doxygen-mainpage.md
- Added all 11 new test suite entries to README.md Testing table
- Verified 51 source headers and 23 examples counts are consistent across all documentation
- Updated README.md Future Directions section to remove outdated v1.1.0 release notes, as the project is now at v1.1.1
- Updated docs/vision.md Current Status section to include v1.1.1 release
- Updated docs/Doxyfile `PROJECT_NUMBER` to 1.1.1 and the version in docs/doxygen-mainpage.md to reflect v1.1.1
- Updated SECURITY.md supported versions table to support only the latest version (1.1.1), dropping 1.0.0 and 1.1.0 — standard practice for solo-maintained projects

## [1.1.0] - 2026-11-15

### Added

#### Core Framework

- **Firmware version in telemetry** (W27): Optional `schemaVersion` and `firmwareVersion` fields in telemetry envelope metadata. Set via `Device::setFirmwareVersion()` and `Device::setTelemetrySchemaVersion()`. Fields are omitted from JSON output when not set (zero overhead).
- **Power manager & battery monitoring** (W26): `PowerManager` singleton for battery voltage sensing via ADC with configurable voltage divider ratio, battery level percentage calculation, and low-battery threshold alerts.
- **Telemetry buffering with LittleFS** (W25): `TelemetryBuffer` for offline persistence when network is unavailable. Failed sends are stored as JSON files in LittleFS with FIFO eviction and replay. Uses monotonic index strategy — no O(n) file shifting, reducing Flash wear and improving performance. Added `onEviction()` callback for monitoring evictions, `getMaxPacketsCount()` getter, and unit-in-name convention for parameters.
- **OTA update support** (W28): `OtaUpdateHandler` for Over-The-Air firmware updates via Wi-Fi. Wraps ArduinoOTA with hostname configuration, password protection, and progress/error callbacks. Added `OtaStartCallback`/`OtaEndCallback` type aliases, `getHostname()` getter, and clarified `setPassword()` timing.

### Testing

- 36 host test suites covering all sensors, actuators, scheduler, MQTT, BLE, HTTP, telemetry, and new v1.1.0 components
- **TelemetryBuffer test**: 22 tests covering initialization, store, retrieve (FIFO), capacity eviction, eviction callback, getMaxPacketsCount(), and constants
- **PowerManager test**: 18 tests covering initialization, voltage calculation, battery level, low-battery detection, custom thresholds, and edge cases
- **OtaUpdateHandler test**: 21 tests covering initialization, status tracking, all callback type aliases (OtaProgressCallback, OtaErrorCallback, OtaStartCallback, OtaEndCallback), getHostname(), re-initialization safety, and edge cases
- **TelemetryPackage firmware version tests**: 5 new tests for optional schemaVersion and firmwareVersion fields in telemetry envelope metadata

### Documentation

- Updated README.md with Arduino Library Manager badge, installation instructions, and new component documentation (PowerManager, TelemetryBuffer, OtaUpdateHandler, firmware version)
- Updated class diagram (PlantUML) to cover all 51 source headers including new v1.1.0 components
- Updated `keywords.txt` with all new component keywords (PowerManager, TelemetryBuffer, OtaUpdateHandler, and related methods and types)
- Updated `docs/improvements.md` with W25–W28 marked Complete and detailed design sections
- Updated `docs/vision.md` with v1.1.0 features listed as released
- **Examples updated**: `WeatherStation` extended with `PowerManager` for battery monitoring. `ClimateDataUploader` extended with `TelemetryBuffer` for offline persistence. New `OtaUpdateDevice` example demonstrates OTA firmware updates with hostname, password, and all callback types.
- Git Flow workflow adopted: `main` holds releases, `develop` is the integration branch, feature branches merge into `develop`

## [1.0.0] - 2026-11-10

### Milestone

- **Arduino Library Manager Approval** (2026-11-14): The Modest IoT Nano Framework has been officially approved by Arduino and is now available in the Arduino Library Manager for easy installation directly from the Arduino IDE.

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
- **WiFiConnectivityDriver**: Wi-Fi connection management with status enumeration, disconnect, IP address reporting, signal strength (RSSI), connection timeout, and `createSecureSocket()` builder method for TLS/SSL socket creation with optional CA certificate pinning
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
- **MqttTlsDevice**: MQTT over TLS example using `WiFiConnectivityDriver::createSecureSocket()` builder method; demonstrates encrypted telemetry with certificate options and memory overhead guidance
- Replaced banner-style comments (`// ---`) with Doxygen `@name` groups across all 22 examples for consistency
- Eliminated magic numbers in all examples: added `SERIAL_BAUD_RATE` (115200) and `MAIN_LOOP_DELAY_IN_MILLISECONDS` (60000) named constants
- Replaced `#define` pin assignments with `static const` variables where appropriate (HomeSecurityDevice, RfidAccessController)
- Added named constants for sampling intervals, queue depths, debounce times, and thresholds (AirConditioningDevice, AirQualityMonitor, HomeSecurityDevice)
- Moved `#define` constants to proper locations with Doxygen comments (ClimateDataUploader)
- RemoteLedController: Updated MQTT callback to parse JSON payloads `{"command": id, "payload": value}` and demonstrate `ADJUST_BRIGHTNESS_COMMAND`
- **Naming convention audit**: Standardized all 22 examples — replaced `_MS` suffix with `_IN_MILLISECONDS` (AirConditioningDevice, AirQualityMonitor, PipeTemperatureMonitor), replaced `#define` with `static const` for numeric constants (8 examples), standardized connectivity driver variable to `transportDriver` (7 examples), gateway client variable to `gatewayClient` (7 examples), and device application variable to `{name}Application` (7 examples)
- **JSON_KEY constants for telemetry**: Added `JSON_KEY_*` constants to all examples with telemetry serialization (AirQualityMonitor, CropMonitoringStation, MqttTlsDevice, SmartBandMqtt) and to `MqttGatewayClient` (`JSON_KEY_COMMAND_IDENTIFIER`, `JSON_KEY_COMMAND_PAYLOAD`) — eliminating magic strings in JSON serialization and parsing
- **Examples onboarding quality improvements**: Added "Modest IoT Nano Framework aspects illustrated" section to all 14 examples that lacked it (TogglableLedDevice, SmartBandMqtt, AirConditioningDevice, AirQualityMonitor, CropMonitoringStation, DriverAssistantDevice, HomeSecurityDevice, OutdoorSecurityLightController, PerimeterMonitor, RfidAccessController, SmartGreenhouseDevice, SmartScaleDevice, WaterFlowMonitor, WeatherStation); standardized Doxygen file headers with grounded real-world device purposes; added `@runs_on{sampler}` tags to all `on()` and `handle()` methods; standardized application instance variable naming to `{name}Application` pattern; fixed ClimateDataUploader JSON_KEY placement (moved inside `@name` group after includes)

#### Documentation
- **ConfigurationStore** documented in CHANGELOG.md Core Framework section with full API description
- **Secure Communication** section in README.md: documents MQTT over TLS (port 8883) and HTTP over TLS with `WiFiClientSecure`, certificate options table, and memory overhead guidance
- Comprehensive documentation audit aligning all docs (README, vision, improvements, CHANGELOG) with actual codebase capabilities
- Added BmeEnvironmentalSensor documentation across all documentation files
- Added Gateway Client Enhancements section documenting BleGatewayClient, HttpGatewayClient, WiFiConnectivityDriver, and BleConnectivityDriver
- Corrected test suite count from 26 to 27, then to 28, and finally to 29 host test suites
- Updated Tier 4 status from "Pending" to "Complete" in improvements tracking
- Removed DHT sensor library from README dependencies (replaced with native implementation)
- Fixed source header count from 47 to 48
- Added Pre-Release Readiness Assessment (W20–W29) to vision.md and improvements.md with blocking and deferred items
- Updated improvements.md comparison table reflecting current capabilities and planned features
- Complete rewrite of keywords.txt (276 → 419 lines) with all 46 source headers, drivers, methods, and nested types
- Enhanced Doxygen documentation: mainpage with key features and architecture overview, 5 new subgroup sections, and wildcard build directory exclusion
- Example enhancement plan: renamed ClassicBlinker → StatusIndicator, HttpTelemetryDemo → ClimateDataUploader; added 5 new examples (BleVitalsTransmitter, RemoteLedController, IndoorAirQualityDisplay, PipeTemperatureMonitor, EquipmentHealthMonitor) with grounded real-world purposes and framework aspects illustrated sections
- Fixed class diagram: marked BmeEnvironmentalSensor as abstract; moved Ssd1306Driver and OledDisplay from Sensors to Actuators package
- Fixed ADR count in adrs.md overview from 29 to 30
- Added new command identifiers to keywords.txt: `DRIVE_TO_ANGLE_COMMAND_IDENTIFIER`, `ADJUST_BRIGHTNESS_COMMAND_IDENTIFIER`, `PLAY_FREQUENCY_COMMAND_IDENTIFIER`
- Updated README.md: Command description now mentions `int32_t` payload, Identifier Registry expanded with payload-based commands, test suites table added `HttpGatewayClient` and `Buzzer`, Future Directions marked W23 complete
- Updated vision.md: Pre-Release tier status shows W20–W24 complete; source header count updated to 48, test suite count updated to 33
- Updated improvements.md: W20–W24 marked complete, comparison table updated, test counts updated to 33
- Updated keywords.txt: Added ConfigurationStore class and all public methods
- Updated README.md: Added ConfigurationStore to Components section, updated Future Directions, updated last updated date
- Updated README.md: Added Secure Communication section with MQTT/HTTP TLS examples, certificate options table, and MqttTlsDevice reference; example count updated to 22; Future Directions marked W22 complete
- Updated README.md: Secure Communication section now shows `createSecureSocket()` as the recommended TLS pattern; MqttTlsDevice example updated to use the builder method
- Updated keywords.txt: Added `createSecureSocket` to connectivity drivers section
- Updated vision.md: W24 marked complete in Pre-Release tier status

### Examples (22 total)
- **MqttTlsDevice**: Secure environmental monitor with encrypted MQTT (TLS) using `createSecureSocket()` builder method
- **SmartGreenhouseDevice**: Updated servo control from two-step `setTargetAngleBufferInDegrees` + `DRIVE_TO_BUFFERED_ANGLE_COMMAND` to single `DRIVE_TO_ANGLE_COMMAND` with payload angle
- **HomeSecurityDevice**: Added missing `#include <Arduino.h>` for consistency with other examples
- StatusIndicator (renamed from ClassicBlinker), TogglableLedDevice, SmartBandMqtt, AirConditioningDevice
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
- 33 host test suites covering all sensors, actuators, scheduler, MQTT, BLE, HTTP, and telemetry
- **TimeSynchronizationDriver test**: 8 tests covering NTP initialization, timestamp fallback, sync status, RTC save/restore, and full sync cycle simulation
- **GasSensor test**: 32 tests covering constructor, static methods, calibration, all operating modes, PPM estimation, and event handling
- **Button test**: 14 tests covering debounce logic, press detection, timing, and event emission
- **WeatherStation example**: Updated with NTP initialization and RTC time restore for deep-sleep time persistence
- **HttpGatewayClient test**: HTTP methods (GET/POST/PUT/DELETE), auth headers, response body handling, statistics tracking
- **Fake HTTPClient.h**: Mock HTTP client for host testing with configurable responses
- **Command payload tests** (W23): 6 new tests in CommandMarshaling suite for payload construction, equality, CommandEntry carry-through, and negative values
- **ServoMotor payload tests**: 6 tests for `DRIVE_TO_ANGLE_COMMAND` with payload angle and `validateOrAdjustPhysicalAngle()` static method
- **RgbLed payload tests**: 5 tests for `ADJUST_BRIGHTNESS_COMMAND` and `adjustColorSetIntensity()` direct method call
- **Buzzer test**: 10 new tests covering all commands (TURN_ON, TURN_OFF, PLAY_TONE, PLAY_FREQUENCY) with tone/noTone mock
- **ConfigurationStore test**: 84 tests covering namespace isolation, type-safe getters/setters, default fallbacks, read-only mode, schema validation, setValue() overloads, and error handling
- CI workflows: host tests, ESP32 compilation, linting
- AddressSanitizer and UndefinedBehaviorSanitizer enabled in tests

### License
- **MIT → Apache 2.0**: Replaced MIT License with Apache License 2.0 for explicit trademark protection (prevents "Modest IoT" rebranding), explicit patent grant, mandatory change documentation, and NOTICE file requirement
- **NOTICE file**: Created `NOTICE` file with author attribution that must be preserved in all distributions
- **Copyright headers**: Added `@copyright (c) 2025-2026 Angel Velasquez` and `@license Apache-2.0` to all 86 source files (48 headers + 38 implementations)
- **library.properties**: Added `license=Apache-2.0` field for Arduino Library Manager

### Documentation
- 30 Architecture Decision Records (ADRs)
- Doxygen documentation with API navigation tree, key features section, and `@runs_on` threading tags
- Class diagram (PlantUML) covering all 48 source headers
- User stories and improvement tracking with pre-release readiness assessment (W20–W29)
- `keywords.txt` comprehensive rewrite: 468 lines with all 48 source headers, drivers, methods, and nested types

### Technical Details
- ESP32 Arduino core 3.x compatible
- FreeRTOS-based task scheduling
- C++17 standard
- Zero magic numbers in framework source - all constants use full-word suffixes (_IN_HERTZ, _IN_MILLISECONDS, _IN_MICROSECONDS)
- Examples follow the same convention: all serial baud rates, loop delays, and timing values are named constants
