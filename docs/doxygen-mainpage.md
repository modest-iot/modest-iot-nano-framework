# Modest IoT Nano-framework (C++ Edition)

This API reference documents the public abstractions and built-in components in
`Modest-IoT-Nano-Framework`.

## Architecture Overview

The framework follows an event-driven, CQRS-inspired model:

- Sensors emit `Event` objects through `EventHandler`.
- Actuators consume `Command` objects through `CommandHandler`.
- `Device` orchestrates asynchronous sampling, scheduling, and telemetry dispatch.
- `TelemetryPackage` provides a serialization contract for transport clients.

## Main API Entry Point

For sketch-level use, include:

```cpp
#include <ModestIoT.h>
```

This umbrella header imports core engine contracts, connectivity adapters, and
available built-in peripherals.

## Module Navigation

- @ref core_module
- @ref sensors_module
- @ref actuators_module
- @ref connectivity_module
- @ref gateway_module

## Documentation Conventions

- Header files are the source of truth for API contracts and extension points.
- `@brief`, `@param`, and `@return` are used for stable public behavior.
- Internal constants are documented only when they affect integration behavior.

## Generating The Docs

Run from the repository root:

```sh
doxygen docs/Doxyfile
```

Generated output is written to `docs/build/html/index.html`.

