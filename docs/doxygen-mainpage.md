# Modest IoT Nano Framework (C++ Edition)

This API reference documents the public abstractions and built-in components in
`Modest-IoT-Nano-Framework` (v1.3.0).

> **Note**: The repository and directory names use hyphens (`Modest-IoT-Nano-Framework`) as a file system convention. The display name is "Modest IoT Nano Framework" (with spaces).

## Architecture Overview

The framework follows an event-driven, CQRS-inspired model adapted for resource-constrained
ESP32 microcontrollers:

- **Sensors** emit `Event` objects through `EventHandler`.
- **Actuators** consume `Command` objects through `CommandHandler`.
- `Device` orchestrates asynchronous sampling, scheduling, and telemetry dispatch
  across two FreeRTOS tasks (sampler and network).
- `TelemetryPackage` provides a serialization contract for transport clients.
- `SamplingScheduler` manages per-slot countdown-based scheduling with tickless sleep support.
- `ModestLog` provides zero-overhead compile-time logging with tag and level filtering.
- `EventAggregator` recognizes time-windowed composite event patterns, and `RulesEngine` maps matches to actuator commands.
- `ChangeFilter<T>` suppresses insignificant per-sensor telemetry changes without dynamic allocation.
- `AiMicroModelController` and `EmlearnController` provide optional on-device inference; `ImuSensor` supports MPU6050 vibration sensing.

## Main API Entry Point

For sketch-level use, include:

```cpp
#include <ModestIoT.h>
```

This umbrella header imports all 62 source headers: core engine contracts, connectivity
adapters, gateway clients, and 19 built-in sensors plus 7 actuators.

## Module Navigation

- @ref core_module — Device, Sensor, Actuator, Event, Command, TelemetryPackage, SamplingScheduler, EventAggregator, RulesEngine, ChangeFilter, and ML inference
- @ref sensors_module — 19 sensor implementations (environmental, security, biometric, etc.)
- @ref actuators_module — 7 actuator implementations (LED, display, servo, buzzer, etc.)
- @ref connectivity_module — Wi-Fi and BLE transport drivers
- @ref gateway_module — MQTT, HTTP, and BLE NUS telemetry gateways

## Key Features

- **Asynchronous engine**: Two FreeRTOS tasks isolate hardware I/O from network operations.
- **Per-slot scheduler**: Each sensor declares its own sampling period; tickless sleep reduces power.
- **Interrupt-driven sensors**: GPIO triggers are marshaled to the sampler task for safe execution.
- **Cross-task commands**: `Device::enqueueCommand()` enables cloud-to-device control via MQTT subscribe.
- **Fault detection**: Configurable consecutive-failure threshold per sensor instance.
- **Diagnostics**: `Device::collectDiagnostics()` reports 16 stack, heap, queue, send, and timing metrics; `worstCaseWakeMicros` is currently a placeholder emitted as `0` because measurement is not implemented.
- **Automation and filtering**: Time-windowed event aggregation, fixed-capacity rules, and per-slot deadband-based telemetry suppression.
- **On-device sensing and inference**: MPU6050 accelerometer/gyroscope statistics with an optional emlearn-backed inference adapter.
- **Host testing**: Platform-independent core compiles natively for unit testing (53 test suites).

## Documentation Conventions

- Header files are the source of truth for API contracts and extension points.
- `@brief`, `@param`, and `@return` are used for stable public behavior.
- `@runs_on` tags document the FreeRTOS task context for each method.
- Internal constants are documented only when they affect integration behavior.

## Generating The Docs

Run from the repository root:

```sh
doxygen docs/Doxyfile
```

Generated output is written to `docs/build/html/index.html`.
