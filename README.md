# Modest IoT Nano-framework (C++ Edition)

**Version**: 0.1  
**Author**: Angel Velasquez  
**Date**: March 23, 2025

## Overview

Modest IoT Nano-framework (C++ Edition) is a lightweight, object-oriented, event-driven, and CQRS-inspired framework designed for Internet of Things (IoT) projects on resource-constrained devices like the ESP32. Implemented in C++, it provides a modular software architecture for building IoT applications, emphasizing simplicity, reusability, and separation of concerns. This framework includes core components for handling events, commands, sensors, actuators, and devices, making it ideal for small-scale IoT prototyping on Arduino-compatible platforms.
This repository contains the C++ Edition of the Modest IoT Nano-framework, packaged as an Arduino library. A MicroPython Edition is planned as a separate distribution (not an Arduino library) for MicroPython-compatible devices. For practical demonstrations of this C++ Edition, see example projects like [`ToggableLedDevice-Example-Cpp`](https://github.com/avelasquezn/ToggableLedDevice-Example-Cpp).

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

## Getting Started

### Prerequisites
- **Software**: Arduino IDE with ESP32 board support installed.

### Installation
1. **Download**: Clone or download this repository from [avelasquezn/Modest-IoT-Nano-framework-Cpp](https://github.com/avelasquezn/Modest-IoT-Nano-framework-Cpp).
2. **Install as Library**:
    - Copy the `ModestIoT/` folder to your Arduino libraries directory (e.g., `~/Arduino/libraries/` on Linux/Mac, `Documents\Arduino\libraries\` on Windows).
    - Alternatively, in the Arduino IDE: Sketch > Include Library > Add .ZIP Library, select a zipped `ModestIoT/` folder.
3. **Verify Structure**: The library should be organized as:
```plaintext
   Modest-IoT-Nano-framework-Cpp/
   ├── ModestIoT/
   │   ├── library.properties
   │   ├── ModestIoT.h
   │   ├── EventHandler.h
   │   ├── CommandHandler.h
   │   ├── Sensor.h
   │   ├── Sensor.cpp
   │   ├── Button.h
   │   ├── Button.cpp
   │   ├── Actuator.h
   │   ├── Actuator.cpp
   │   ├── Led.h
   │   ├── Led.cpp
   │   ├── Device.h
   │   └── Device.cpp
```
3. **Integration**: Copy the `framework/` folder into your Arduino project directory, or install it as a library in the Arduino IDE (place in `~/Arduino/libraries/ModestIoT/` and rename `framework/` to `ModestIoT/`).

### Usage
Include the library in your sketch:
```cpp
#include <ModestIoT.h> 
```
- Extend Device to create custom IoT devices.
- Use Button and Led as examples of concrete sensor and actuator implementations, or derive your own from Sensor and Actuator.
- Handle events and commands via the EventHandler and CommandHandler interfaces.
For a complete example, refer to ToggableLedDevice-Example-Cpp.
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