# Modest IoT Nano-framework (C++ Edition)

**Version**: 0.2.0  
**Author**: Angel Velasquez  

**Date created**: 2025-03-23

**Last updated**: 2026-06-04

## Overview

Modest IoT Nano-framework (C++ Edition) is a lightweight, object-oriented, event-driven, and CQRS-inspired Arduino library for resource-constrained IoT projects such as ESP32-based devices. It provides a small set of reusable abstractions for events, commands, sensors, actuators, and complete devices, making it easy to assemble small IoT applications with clear separation of concerns.

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
- **`DhtSensor`**: A concrete sensor for reading temperature and humidity from a DHT module.
- **`LcdDisplay`**: A concrete actuator for driving a 16x2 I2C character LCD.

## Documentation

The codebase now includes code-level Doxygen comments in both header files and implementation files. These comments document:

- public APIs and their expected behavior,
- template-method extension points used by subclasses,
- predefined events and commands, and
- internal helper routines and worker loops.

If you generate API documentation with Doxygen, point it at the `src/` directory to include the inline documentation in the rendered output.

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
- Use `Button`, `Led`, `DhtSensor`, and `LcdDisplay` as building blocks inside your device subclass, or derive your own from `Sensor` and `Actuator`.
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
This project is licensed under the Creative Commons Attribution-NoDerivatives 4.0 International (CC BY-ND 4.0):

- **Allowed**: Use and redistribute the framework in its original, unmodified form for any purpose, with credit to Angel Velasquez.
- **Not Allowed**: Modify, adapt, or create derivative works from the framework.
- **Full License**: CC BY-ND 4.0 Legal Text.
Note: CC BY-ND 4.0 ensures the framework’s original design is preserved while allowing broad usage.
## Contributing
Due to the CC BY-ND 4.0 license, modifications or extensions to the framework are not permitted. However, you’re welcome to:
- Use the framework in your projects.
- Share feedback or suggestions with the author via GitHub.
- Create separate example projects that leverage the framework unchanged.
## Future Directions
- **Version 0.2**: Potential additions like new sensor/actuator types (e.g., temperature sensors, motors).
- **MicroPython Edition**: A planned port to MicroPython in a separate repository (e.g., Modest-IoT-Nano-framework-MicroPython).
## Contact
For questions, feedback, or support, contact Angel Velasquez via GitHub:
- **GitHub**: avelasquezn
---
Modest IoT Nano-framework (C++ Edition): A simple, modular foundation for your IoT projects.