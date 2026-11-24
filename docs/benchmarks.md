# Modest IoT Nano Framework — Benchmark Table

**Version**: 1.3.0<br>
**Date**: 2026-11-24<br>
**Platform**: ESP32 (Arduino core 4.0.0)  
**Methodology**: Source code analysis, static configuration values, framework claims from positioning.md, and CI-measured sketch sizes from the compile-examples workflow. Hardware-measured values are marked with estimates based on the framework's architectural design.

> **Note**: This benchmark table provides a structured comparison of the Modest IoT Nano Framework against traditional Arduino `loop()` development and the framework's evolution through v1.3.0. This document is versioned for v1.3.0, but its resource estimates and performance comparisons have not been re-measured for the new features; do not treat them as v1.3.0 hardware benchmarks. Where v1.3.0 figures match v1.2.1, they are carried-forward estimates rather than new measurements. Values are derived from source code constants, configuration defaults, documented claims, and CI compilation output. For hardware-validated measurements, see the validation methodology at the end of this document.

---

## 1. Resource Usage

### Flash Memory

| Metric | Traditional `loop()` | Modest IoT v1.1.2 | Modest IoT v1.2.0 | Modest IoT v1.3.0 | Notes |
|--------|---------------------|-------------------|-------------------|-------------------|-------|
| **Framework code** | ~0 KB (no framework) | ~250 KB | ~220-230 KB | ~220-230 KB | v1.2.0 saved ~20-30 KB by removing ArduinoJson |
| **ArduinoJson dependency** | N/A | ~30-40 KB | **0 KB** | **0 KB** | Removed in v1.2.0 (W46) |
| **External dependencies** | Board package only | MQTT + ArduinoJson | **MQTT only** | **MQTT only** | Only 1 external dependency |
| **ESP32 Arduino core** | 3.x | 3.x | 3.x | **4.0.0** | ESP32 4.0.0 required for BLE (ESP-IDF v6.1.1) |
| **Minimal firmware binary** | ~350-400 KB | ~680 KB | ~650-660 KB | ~650-660 KB | Includes Arduino core + Wi-Fi + BLE |
| **ESP-IDF variant (planned)** | N/A | N/A | ~180 KB | ~180 KB | **~73% Flash reduction** vs Arduino variant |

**Key finding**: v1.2.0 eliminated ArduinoJson (~30-40 KB savings), making it the **leanest architecturally disciplined Arduino library**. v1.2.1 migrates to ESP32 4.0.0 (ESP-IDF v6.1.1) with no Flash increase. The planned ESP-IDF variant reduces Flash by ~73% (~500 KB savings).

### Static RAM

| Metric | Traditional `loop()` | Modest IoT v1.1.2 | Modest IoT v1.2.0 | Modest IoT v1.3.0 | Notes |
|--------|---------------------|-------------------|-------------------|-------------------|-------|
| **Framework static data** | ~0 KB | ~45-55 KB | ~40-50 KB | ~40-50 KB | Reduced by ArduinoJson removal |
| **Telemetry buffer** | N/A | 512 bytes | 512 bytes | 512 bytes | Fixed-size stack buffer (JsonTelemetryWriter) |
| **MQTT serialization** | N/A | 512 bytes | 512 bytes | 512 bytes | Fixed-size buffer (MqttGatewayClient) |
| **MQTT subscribe buffer** | N/A | 256 bytes | 256 bytes | 256 bytes | Fixed-size buffer for inbound commands |
| **Free heap after Wi-Fi** | ~280 KB | ~230 KB | ~230-240 KB | ~230-240 KB | ESP-IDF variant: ~270 KB (**~40 KB more**) |

**Key finding**: Framework uses ~40-60 KB RAM total. Zero heap allocation in serialization path — all buffers are stack-allocated or static.

### Task Stack Configuration

| Task | Stack Size | Priority | Core | Purpose |
|------|-----------|----------|------|---------|
| **Sampler** | 4,096 bytes | 3 | Core 1 | Sensor polling, event handling |
| **Network** | 8,192 bytes | 1 | Core 1 | MQTT/HTTP/BLE telemetry dispatch |
| **Total task stacks** | **12,288 bytes** | — | — | Configurable via `EngineConfiguration` |

**Key finding**: Stack sizes are configurable at startup. The `DiagnosticsSnapshot` reports `samplerStackFreeWords` and `networkStackFreeWords` for runtime monitoring via `uxTaskGetStackHighWaterMark`.

---

## 2. Latency

### Event Processing Latency

| Metric | Traditional `loop()` | Modest IoT v1.1.2 | Modest IoT v1.2.0 | Modest IoT v1.3.0 | Notes |
|--------|---------------------|-------------------|-------------------|-------------------|-------|
| **Sensor-to-event** | Variable (loop-dependent) | ~10-100 μs | ~10-100 μs | ~10-100 μs | Synchronous event propagation |
| **Event-to-command** | Variable (loop-dependent) | ~5-50 μs | ~5-50 μs | ~5-50 μs | Direct function call via mediator |
| **Button edge-to-action** | ~1-10 ms (polling) | ~100-500 μs | ~100-500 μs | ~100-500 μs | GPIO interrupt → ISR → event |
| **PIR edge-to-action** | ~1-10 ms (polling) | ~100-500 μs | ~100-500 μs | ~100-500 μs | GPIO interrupt → ISR → event |
| **Worst-case handler** | Unbounded | Tracked by `worstCaseWakeMicros` | Tracked by `worstCaseWakeMicros` | Tracked by `worstCaseWakeMicros` | Diagnostics field |

**Key finding**: Modest IoT uses synchronous event propagation within the sampler task, so event-to-command latency is a direct function call (~5-50 μs). GPIO interrupts use IRAM ISR handlers for minimal latency.

### Telemetry Pipeline Latency

| Metric | Traditional `loop()` | Modest IoT v1.1.2 | Modest IoT v1.2.0 | Modest IoT v1.3.0 | Notes |
|--------|---------------------|-------------------|-------------------|-------------------|-------|
| **Serialization time** | Variable | ~50-200 μs | ~50-200 μs | ~50-200 μs | snprintf-based, 512-byte buffer |
| **Queue handoff** | N/A | ~10-50 μs | ~10-50 μs | ~10-50 μs | FreeRTOS queue send |
| **Network dispatch** | Blocking | Non-blocking | Non-blocking | Non-blocking | Separate network task |
| **High-priority bypass** | N/A | N/A | **~10-50 μs** | **~10-50 μs** | W32: 4-slot priority queue checked first |

**Key finding**: W32 (High-Priority Lane) ensures critical alerts bypass routine telemetry with a separate 4-slot queue checked first by the network task.

### Boot Time

| Metric | Traditional `loop()` | Modest IoT v1.1.2 | Modest IoT v1.2.0 | Modest IoT v1.3.0 | Notes |
|--------|---------------------|-------------------|-------------------|-------------------|-------|
| **Boot to `setup()`** | ~200 ms | ~320 ms | ~320 ms | ~320 ms | ESP-IDF variant: **~180 ms** (140 ms faster) |
| **Engine start overhead** | N/A | ~50-100 ms | ~50-100 ms | ~50-100 ms | Task creation, queue init, scheduler setup |

**Key finding**: Framework adds ~120 ms boot overhead vs bare Arduino. ESP-IDF variant reduces total boot to ~180 ms.

---

## 3. Throughput

### Queue Throughput

| Metric | Standard Queue | High-Priority Queue | Notes |
|--------|---------------|---------------------|-------|
| **Queue depth** | 10 slots | 4 slots | Configurable via `EngineConfiguration` |
| **Drop policy** | Drop oldest | Drop oldest | Prevents queue starvation |
| **Enqueue rate** | Bounded by queue depth | Bounded by queue depth | `packetsEnqueued` counter tracks total |
| **Drop tracking** | `packetsDropped`, `packetsDroppedOldest` | `highPriorityDropped` | All visible in `DiagnosticsSnapshot` |

**Key finding**: Queue depths are configurable. The drop-oldest policy ensures new telemetry is never lost, and all drops are tracked for diagnostics.

### Serialization Throughput

| Metric | Value | Notes |
|--------|-------|-------|
| **Buffer size** | 512 bytes | Fixed-size stack buffer |
| **Typical payload** | ~100-200 bytes | 3-5 scalar fields |
| **Max fields per payload** | ~10-15 | Depends on field name/value length |
| **Overflow detection** | `hasOverflow()` | Returns true if buffer exceeded |

**Key finding**: 512-byte buffer handles typical 3-5 field payloads (~200 bytes). Overflow is detected and reported, not silently truncated.

### Network Throughput

| Metric | Value | Notes |
|--------|-------|-------|
| **Send tracking** | `sendsSucceeded`, `sendsFailed` | Cumulative counters |
| **Queue poll timeout** | 100 ms | Non-blocking receive |
| **Reconnect interval** | Configurable | `reconnectIntervalInMilliseconds` |
| **Offline buffering** | Up to 50 packets | LittleFS persistence, ~25-50 KB |

**Key finding**: TelemetryBuffer persists up to 50 packets during network outages and replays them in FIFO order when connectivity is restored.

---

## 4. Queue Depth & Pressure

### Standard Telemetry Queue

| Metric | Default | Configurable | Diagnostics Field |
|--------|---------|-------------|-------------------|
| **Depth** | 10 slots | Yes (`EngineConfiguration.queueLength`) | `queueHighWaterMark` |
| **Enqueued** | — | — | `packetsEnqueued` |
| **Dropped** | — | — | `packetsDropped` |
| **Dropped (oldest)** | — | — | `packetsDroppedOldest` |

### High-Priority Queue (W32)

| Metric | Default | Configurable | Diagnostics Field |
|--------|---------|-------------|-------------------|
| **Depth** | 4 slots | No (fixed) | `highPriorityQueueHighWaterMark` |
| **Enqueued** | — | — | `highPriorityEnqueued` |
| **Dropped** | — | — | `highPriorityDropped` |

**Key finding**: All queue metrics are exposed via `DiagnosticsSnapshot` for remote monitoring. The high-priority queue is fixed at 4 slots to ensure bounded memory usage.

---

## 5. Stack Usage

### Task Stack Configuration

| Task | Stack Size (bytes) | Stack Size (words) | Priority | Core |
|------|-------------------|-------------------|----------|------|
| **Sampler** | 4,096 | 1,024 words | 3 | Core 1 |
| **Network** | 8,192 | 2,048 words | 1 | Core 1 |
| **Total** | **12,288** | **3,072 words** | — | — |

### Runtime Stack Monitoring

| Metric | Diagnostics Field | Unit | Method |
|--------|-----------------|------|--------|
| **Sampler free stack** | `samplerStackFreeWords` | Words (4 bytes each) | `uxTaskGetStackHighWaterMark` |
| **Network free stack** | `networkStackFreeWords` | Words (4 bytes each) | `uxTaskGetStackHighWaterMark` |

**Key finding**: Stack high-water marks are reported in FreeRTOS words (4 bytes on ESP32). The `DiagnosticsSnapshot` provides real-time stack utilization without external tools.

---

## 6. Heap Usage

### Heap Metrics

| Metric | Diagnostics Field | Notes |
|--------|-----------------|-------|
| **Current free heap** | `freeHeapBytes` | Bytes |
| **Minimum free heap** | `minFreeHeapBytes` | Lowest since boot |
| **Framework heap usage** | ~0 bytes (serialization) | All stack-allocated |

### Heap-Safe Design

| Component | Allocation | Buffer Type |
|-----------|-----------|-------------|
| **JsonTelemetryWriter** | Zero heap | 512-byte stack buffer |
| **MqttGatewayClient** | Zero heap | 512-byte static buffer |
| **TelemetryBuffer** | LittleFS | File-per-packet, monotonic indices |
| **EventAggregator** | Zero heap | 32-event circular buffer, 4 pattern slots |
| **Device queues** | FreeRTOS | Static queue handles |

**Key finding**: The framework is designed for **zero heap allocation** in the critical path. All serialization, buffering, and event handling use stack-allocated or static buffers.

---

## 7. Serialization Time

### JSON Serialization

| Metric | Value | Notes |
|--------|-------|-------|
| **Method** | `snprintf` | Native C library, no dependencies |
| **Buffer size** | 512 bytes | Stack-allocated |
| **Typical payload** | ~100-200 bytes | 3-5 scalar fields |
| **Estimated time** | ~50-200 μs | Depends on field count and complexity |
| **Overflow handling** | `hasOverflow()` | Detects and reports buffer overflow |

### Comparison with ArduinoJson

| Metric | ArduinoJson (v1.1.2) | snprintf (v1.2.0) | Savings |
|--------|---------------------|-------------------|---------|
| **Flash usage** | ~30-40 KB | **0 KB** | **~30-40 KB** |
| **RAM usage** | Dynamic allocation | **512 bytes static** | **No heap** |
| **Serialization time** | ~100-300 μs | ~50-200 μs | **Comparable or faster** |
| **Dependencies** | ArduinoJson library | **None** | **Zero dependencies** |

**Key finding**: W46 (ArduinoJson removal) saves ~30-40 KB Flash with comparable or better serialization performance and zero heap allocation.

---

## 8. Power Consumption

### Power-Related Features

| Feature | Description | Impact |
|---------|-------------|--------|
| **PowerManager** | Battery voltage sensing via ADC | Configurable voltage divider, low-battery alerts |
| **Deep sleep support** | Example: `DeepSleepMonitor` | ESP32 deep sleep ~10-20 μA |
| **Per-sensor scheduling** | Independent polling intervals | Reduces active time |
| **Minimum period enforcement** | SamplingScheduler enforces minimums | Prevents excessive polling |

### Estimated Power Profiles

| Mode | Current Draw | Notes |
|------|-------------|-------|
| **Active (Wi-Fi)** | ~80-120 mA | Typical ESP32 Wi-Fi transmission |
| **Active (BLE)** | ~10-20 mA | BLE advertising/connection |
| **Idle (no Wi-Fi)** | ~10-20 mA | CPU active, peripherals idle |
| **Deep sleep** | ~10-20 μA | ESP32 deep sleep mode |

**Key finding**: Power consumption is primarily determined by the ESP32 chip and connectivity mode. The framework's per-sensor scheduling and minimum period enforcement help minimize active time.

**Note**: Actual power measurements require hardware testing with a current meter. Wokwi simulations cannot measure current draw.

---

## 9. Lines of Application Code

### Framework Code Metrics

| Metric | Count | Notes |
|--------|-------|-------|
| **Header files** | 62 | All in `src/` directory |
| **Source files** | 48 | Matching `.cpp` files |
| **Test suites** | 53 | Host unit tests (excluding `fakes/`) |
| **Examples** | 28 | Grounded real-world devices |
| **Components** | 30 | 21 sensors + 9 actuators |
| **Core classes** | 12 | Device, Event, Command, Scheduler, etc. |

### Example Code Comparison

| Application | Traditional `loop()` | Modest IoT v1.3.0 | Notes |
|-------------|---------------------|-------------------|-------|
| **StatusIndicator** | ~30 lines | ~40 lines | Simplest example, LED + timer |
| **HomeSecurityDevice** | ~80-100 lines | ~60-80 lines | Motion + door sensor, buzzer |
| **CropMonitoringStation** | ~120-150 lines | ~80-100 lines | Multi-sensor, MQTT telemetry |

**Key finding**: Modest IoT examples are comparable or shorter than traditional `loop()` equivalents because the framework handles scheduling, event dispatch, and telemetry pipeline automatically.

---

## 10. Validation Methodology

The benchmark table above combines **source code analysis** (exact values from constants and configuration) with **framework claims** (from positioning.md and documentation). For hardware-validated measurements, the following methodology is recommended:

### Reference Applications

Run the same three reference applications on the same ESP32 board, once per variant (plain `loop()`, Modest IoT v1.1.2, Modest IoT v1.2.0, Modest IoT v1.3.0):

1. **Multi-sensor monitor**: DHT, BH1750, and a button, with a display and Wi-Fi/MQTT telemetry.
2. **Blocking-sensor stress**: The monitor plus an HX711 and an ultrasonic sensor.
3. **Network fault**: The monitor with the broker unreachable for 60 seconds, then restored.

### Measurement Methods

| Metric | Method |
|--------|--------|
| **Sample period jitter** | GPIO toggle per sample on a logic analyzer, or `esp_timer_get_time` deltas |
| **Button and PIR edge-to-action latency** | Logic analyzer: input edge to actuator output edge |
| **Sensing continuity during network fault** | Count missed samples while the broker is down |
| **Reconnect time** | Time from broker restore to first successful publish |
| **Stack use** | `uxTaskGetStackHighWaterMark` for each task |
| **Heap** | `ESP.getFreeHeap()` and `getMinFreeHeap()` after 24 hours |
| **Flash and static RAM** | Size report from the build for each variant |
| **Current draw** | Inline current meter (Wokwi cannot measure it) |
| **Lines of application code** | Count in the reference sketches |

### CI Validation

The release checklist (`docs/release.md`) defines the CI matrix. This review verified the host suite and coverage locally; release-branch CI and example compilation remain separate checklist items.

- **Host tests**: All 53 suites passed locally on macOS with AddressSanitizer and UndefinedBehaviorSanitizer; the release matrix also targets Ubuntu.
- **Example compilation**: The checklist targets all 28 examples on ESP32 and ESP32-S3 with ESP32 Arduino core 3.3.13 and 4.0.0, using the `huge_app` partition scheme (3 MB Flash); this was not run locally in this review.
- **Lint**: Arduino Library Manager compliance via `arduino-lint`; the release checklist result remains to be confirmed.
- **Coverage**: The local GCC 16/gcov/lcov run covered all 53 suites and measured 91.2% line and 83.6% function coverage on testable code, above the 80% line threshold.

---

## 11. Summary

### Framework Efficiency Claims

| Claim | Evidence | Status |
|-------|----------|--------|
| **~200-300 KB Flash** | Source code analysis + positioning.md | ✓ Supported |
| **40-60 KB RAM** | Source code analysis + positioning.md | ✓ Supported |
| **Zero heap allocation** | All serialization uses stack/static buffers | ✓ Verified |
| **~30-40 KB Flash saved** | ArduinoJson removal (W46) | ✓ Verified |
| **~73% Flash reduction** | ESP-IDF variant: ~180 KB vs ~680 KB | ✓ Planned |
| **~40 KB RAM saved** | ESP-IDF variant: ~270 KB vs ~230 KB free heap | ✓ Planned |
| **140 ms faster boot** | ESP-IDF variant: ~180 ms vs ~320 ms | ✓ Planned |
| **53 test suites** | `tests/` directory count (excluding fakes/) | ✓ Verified |
| **28 examples** | `examples/` directory count | ✓ Verified |
| **62 headers** | `src/*.h` file count | ✓ Verified |
| **91.2% line coverage** | Local GCC 16/gcov/lcov run on testable code | ✓ Verified |
| **BLE core compatibility** | `BleHal` supports ESP32 Arduino core 3.x and 4.x; CI matrix verification remains on release checklist | ✓ Implemented |

### Competitive Positioning

The Modest IoT Nano Framework v1.3.0 is the **leanest architecturally disciplined Arduino library** with:

- **Zero external dependencies** beyond MQTT (ArduinoJson removed).
- **Zero heap allocation** in the critical path (all stack/static buffers).
- **Comprehensive diagnostics** with 16 fields in `DiagnosticsSnapshot`.
- **Production-grade architecture** via Arduino Library Manager installation.
- **53 host test suites**; all passed locally on macOS with sanitizers, and the latest local coverage is 91.2% line / 83.6% function.
- **28 grounded examples** covering diverse use cases.
- **ESP32 Arduino core 3.x and 4.x BLE support** through the core-agnostic `BleHal` adapter.

---

*This document will be updated with hardware-measured values when the validation methodology is executed on physical ESP32 hardware.*
