# ModestIoT Nano-Framework (C++ Edition)

**Version**: 1.0.0  
**Author**: Angel Velasquez  

**Date created**: 2025-03-23

**Last updated**: 2026-06-05

## Overview

ModestIoT Nano-Framework (C++ Edition) is a lightweight, object-oriented, event-driven, and CQRS-inspired Arduino library for resource-constrained IoT projects such as ESP32-based devices. It provides a small set of reusable abstractions for events, commands, sensors, actuators, and complete devices, making it easy to assemble small IoT applications with clear separation of concerns.

This repository contains the C++ edition of the framework. A MicroPython edition is planned as a separate distribution for MicroPython-compatible devices. For a practical usage example, see [`ToggableLedDevice-Example-Cpp`](https://github.com/avelasquezn/ToggableLedDevice-Example-Cpp).

## Key Characteristics
- **Object-Oriented**: Built with C++ classes, inheritance, and encapsulation for maintainable code.
- **Event-Driven**: Uses an `Event`/`EventHandler` system for asynchronous, reactive behavior.
- **CQRS-Inspired**: Separates `Command` (control) and `Event` (notification) pathways, mirroring Command Query Responsibility Segregation principles.

## Components
- **`Event` & `EventHandler`**: Define and handle events (e.g., sensor triggers).
- **`Command` & `CommandHandler`**: Issue and process commands (e.g., actuator control).
- **`Sensor`**: Abstract base class for input devices, with event propagation.
- **`Actuator`**: Abstract base class for output devices, with command handling.
- **`Device`**: Abstract base class combining event and command handling for complete IoT devices.
- **`Button`**: A concrete sensor class for detecting button presses and generating events.
- **`Led`**: A concrete actuator class for controlling an LED with toggle, on, and off commands.
- **`RgbLed`**: A concrete actuator for a PWM-driven RGB LED with color (per-channel 0-255 intensity), on, off, and toggle commands, including common-anode support and predefined colors (`RgbLed::RED`, `ORANGE`, ...).
- **`DhtSensor`**: A concrete sensor for reading temperature and humidity from a DHT module.
- **`LcdDisplay`**: A concrete actuator for driving a 16x2 I2C character LCD.

## Documentation

Documentation is split between architecture artifacts in `docs/` and code-level API documentation in `src/`.

### Design and Architecture (`docs/`)

| Artifact                                             | Description                                                                                                                                 |
|------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------|
| [`docs/class-diagram.puml`](docs/class-diagram.puml) | PlantUML class diagram of the current framework structure (inheritance, associations, and the `CustomDevice : Device` composition pattern). |
| [`docs/user-stories.md`](docs/user-stories.md)       | Product-level user stories and acceptance criteria that motivated the framework design.                                                     |

### API and Code-Level Reference (`src/`)

The source files in `src/` include Doxygen comments in both headers and implementation files. These comments document:

- public APIs and their expected behavior,
- template-method extension points used by subclasses,
- predefined events and commands, and
- internal helper routines and worker loops.

To generate browsable API docs locally, use the provided Doxygen configuration:

```sh
doxygen docs/Doxyfile
```

Then open `docs/build/html/index.html` in your browser.

### Maintenance Guidelines

- Keep `docs/class-diagram.puml` synchronized with changes in class names, inheritance, and key associations.
- Treat header declarations and inline Doxygen comments as the source of truth for API contracts.
- Update documentation in the same change set whenever behavior or public interfaces change.

## Getting Started

### Prerequisites
- **Software**: Arduino IDE with ESP32 board support installed.

### Installation
1. **Download**: Clone or download this repository from [avelasquezn/Modest-IoT-Nano-framework-Cpp](https://github.com/avelasquezn/Modest-IoT-Nano-framework-Cpp).
2. **Install as Library**:
    - Copy the repository contents, or at minimum the `src/` folder and `src/library.properties`, to your Arduino libraries directory (for example, `~/Arduino/libraries/ModestIoT/` on macOS/Linux or `Documents\Arduino\libraries\ModestIoT\` on Windows).
    - Alternatively, in the Arduino IDE: Sketch > Include Library > Add .ZIP Library, then select a zipped copy of the library folder.
3. **Verify Structure**: The library should be organized as:
   ```plaintext
      Modest-IoT-Nano-framework-Cpp/
      ├── README.md
      ├── src/
      │   ├── library.properties
      │   ├── ModestIoT.h
      │   ├── EventHandler.h
      │   ├── CommandHandler.h
      │   ├── Sensor.h
      │   ├── Sensor.cpp
      │   ├── Actuator.h
      │   ├── Actuator.cpp
      │   ├── Device.h
      │   ├── Device.cpp
      │   ├── Button.h
      │   ├── Button.cpp
      │   ├── Led.h
      │   ├── Led.cpp
      │   ├── RgbLed.h
      │   ├── RgbLed.cpp
      │   ├── DhtSensor.h
      │   ├── DhtSensor.cpp
      │   ├── LcdDisplay.h
      │   └── LcdDisplay.cpp
   ```
4. **Integration**: Include the library from your sketch with `#include <ModestIoT.h>` after the folder has been installed in your Arduino libraries directory.

### Usage
Include the library in your sketch:
```cpp
#include <ModestIoT.h> 
```
- Extend `Device` to create a custom IoT device that owns and coordinates its sensors and actuators.
- Use `Button`, `Led`, `RgbLed`, `DhtSensor`, and `LcdDisplay` as building blocks inside your device subclass, or derive your own from `Sensor` and `Actuator`.
- Have your sketch instantiate the custom device and interact with that device object; the device is responsible for wiring events and commands through `EventHandler` and `CommandHandler`.
- For a complete example, refer to `ToggableLedDevice-Example-Cpp`.

### Example usage

```cpp
#include <ModestIoT.h>

// Sketch-level code should interact with a custom Device subclass.
// The device owns and wires the components together internally.
// CustomDevice device;
```

The framework is designed so that sensors emit events and actuators consume commands through the base handler interfaces, with the sketch operating on the custom `Device` object as the composition root.

## Notes

- The library targets Arduino-compatible ESP32 projects.
- Some example components depend on external Arduino libraries such as `DHT` and `LiquidCrystal_I2C`.
- If you are generating documentation, the Doxygen comments in `src/*.h` and `src/*.cpp` provide the canonical code-level reference.
## License
This project is licensed under the **MIT License**.

- **Allowed**: Use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the framework, with attribution.
- **Full License**: See [`LICENSE.md`](LICENSE.md) for the complete terms.

## Contributing
Contributions, improvements, and extensions are welcome under the MIT License. You're welcome to:
- Use the framework in your projects.
- Submit feedback, bug reports, or feature suggestions via GitHub.
- Fork and adapt the framework for your own needs (with attribution).
## Future Directions
- **Version 0.2**: Potential additions like new sensor/actuator types (e.g., temperature sensors, motors).
- **MicroPython Edition**: A planned port to MicroPython in a separate repository (e.g., Modest-IoT-Nano-framework-MicroPython).
## Contact
For questions, feedback, or support, contact Angel Velasquez via GitHub:
- **GitHub**: avelasquezn
---
ModestIoT Nano-Framework (C++ Edition): A simple, modular foundation for your IoT projects.