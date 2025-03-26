# User Stories for ModestIoT (C++ Edition) Arduino Library

This document contains user stories for the `ModestIoT` (C++ Edition) Arduino library, designed for IoT Developers creating applications on resource-constrained devices like the ESP32. Each story includes acceptance criteria, using the **Given-When-Then** format where applicable, covering main functional and non-functional requirements that drove to the design and implementation of the framework.

## IoT Developer Stories

### US01: Handle Events for Sensors
- **As an** IoT Developer, **I want** a framework that defines and propagates events from sensors, **so that** I can build reactive IoT devices that respond to input changes without manual polling.
- **Acceptance Criteria**:
    - **Given** I include `<ModestIoT.h>` and instantiate a `Sensor` subclass (e.g., `Button`), **when** an event is triggered (e.g., `BUTTON_PRESSED_EVENT`), **then** it is propagated to a registered `EventHandler` without requiring me to check the sensor state manually.
    - The framework provides an `Event` structure and `EventHandler` interface that I can extend for custom events.
    - Event handling is lightweight, using minimal memory (e.g., a single integer ID per event).

### US02: Issue Commands to Actuators
- **As an** IoT Developer, **I want** a framework that defines and executes commands for actuators, **so that** I can control output devices in a structured, reusable way.
- **Acceptance Criteria**:
    - **Given** I instantiate an `Actuator` subclass (e.g., `Led`) with a pin, **when** I issue a command (e.g., `TOGGLE_LED_COMMAND`), **then** the actuator updates its state (e.g., LED toggles) and propagates the command to a registered `CommandHandler`.
    - The framework provides a `Command` structure and `CommandHandler` interface that I can extend for custom commands.
    - Command execution is deterministic, ensuring consistent actuator behavior.

### US03: Combine Inputs and Outputs in Devices
- **As an** IoT Developer, **I want** an abstraction to combine sensors and actuators into a single device, **so that** I can create cohesive IoT units with minimal boilerplate code.
- **Acceptance Criteria**:
    - **Given** I extend the `Device` class, **when** I implement `on(Event)` and `handle(Command)`, **then** I can link sensor events to actuator commands within the same object (e.g., button press toggles an LED).
    - The `Device` interface inherits both `EventHandler` and `CommandHandler`, requiring no additional interfaces for basic device functionality.
    - The abstraction supports at least one sensor and one actuator per device instance.

### US04: Modular and Reusable Components
- **As an** IoT Developer, **I want** a modular framework with reusable sensor and actuator classes, **so that** I can apply the same components across multiple IoT projects with different configurations.
- **Acceptance Criteria**:
    - **Given** I use `Button` and `Led` classes, **when** I change their pin assignments (e.g., GPIO 25 for button, GPIO 33 for LED), **then** they function correctly without modifying the framework code.
    - **Given** I create a new project, **when** I include `<ModestIoT.h>` and instantiate `Button` and `Led`, **then** I can build a different device (e.g., LED blinker) using the same classes.
    - The framework’s `Sensor` and `Actuator` base classes allow me to derive custom components without altering existing ones.

### US05: Lightweight and Efficient Design
- **As an** IoT Developer, **I want** a framework that is lightweight and efficient, **so that** it runs effectively on resource-constrained microcontrollers like the ESP32.
- **Acceptance Criteria**:
    - **Given** I compile a sketch using the framework on an ESP32, **when** I check the binary size, **then** it adds less than 10KB to a minimal sketch (excluding Arduino core).
    - **Given** I use the framework in an interrupt-driven sketch, **when** I measure execution time for event/command handling, **then** it completes within 1ms on an ESP32 at 240MHz.
    - The framework avoids dynamic memory allocation (e.g., `new`/`delete`) in its core components to prevent heap fragmentation.

---

*Created: March 23, 2025.*
*Last Updated: March 26, 2025.*