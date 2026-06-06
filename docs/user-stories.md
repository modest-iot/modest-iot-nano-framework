# User Stories for ModestIoT Nano-Framework

This document contains user stories for the `ModestIoT` Nano-Framework, designed for IoT Developers creating applications on resource-constrained devices like the ESP32. Each story includes acceptance criteria, using the **Given-When-Then** format where applicable, covering main functional and non-functional requirements that drove to the design and implementation of the framework.

## IoT Developer Stories

### TS01: Handle Events for Sensors
- **As an** IoT Developer, **I want** a framework that defines and propagates sensor-originated events, **so that** I can build reactive IoT behavior without wiring ad-hoc polling logic.
- **Acceptance Criteria**:
    - **Given** a sensor input changes, **when** the framework captures that change, **then** an event is emitted and routed to a configured consumer.
    - **Given** no consumer is configured, **when** an event is emitted, **then** the framework remains stable and does not crash.
    - Event metadata is lightweight and deterministic enough for constrained devices.

### TS02: Issue Commands to Actuators
- **As an** IoT Developer, **I want** a framework that defines and executes commands for outputs, **so that** I can control actuators in a structured, reusable way.
- **Acceptance Criteria**:
    - **Given** a valid command is issued, **when** the framework dispatches it, **then** the target output state changes consistently.
    - **Given** a command has an upstream propagation target, **when** it is processed, **then** it can be forwarded after local execution.
    - Command handling is deterministic and safe under repeated invocations.

### TS03: Combine Inputs and Outputs in Devices
- **As an** IoT Developer, **I want** a unified device abstraction that combines event and command flows, **so that** I can build cohesive IoT units with minimal boilerplate.
- **Acceptance Criteria**:
    - **Given** a custom device implementation, **when** input events are received, **then** the device can map them into output commands.
    - **Given** a single device instance owns multiple inputs and outputs, **when** it runs, **then** it acts as the composition root for orchestration.
    - The abstraction supports scaling from simple one-input/one-output cases to multi-component devices.

### TS04: Modular and Reusable Components
- **As an** IoT Developer, **I want** modular and reusable component contracts, **so that** I can reuse capabilities across projects with different hardware mappings.
- **Acceptance Criteria**:
    - **Given** components are configured with different physical mappings, **when** the application starts, **then** behavior remains correct without framework source changes.
    - **Given** a new project, **when** the same component contracts are reused, **then** a different device behavior can be assembled quickly.
    - Developers can add new component types by extending documented contracts without modifying existing implementations.

### TS05: Lightweight and Efficient Design
- **As an** IoT Developer, **I want** a lightweight and efficient framework, **so that** it performs well on resource-constrained microcontrollers.
- **Acceptance Criteria**:
    - **Given** a minimal application that uses core capabilities, **when** it is built, **then** framework overhead remains bounded and documented.
    - **Given** typical event/command workloads, **when** execution latency is measured, **then** timing remains within documented real-time targets.
    - Memory behavior is predictable under sustained operation and does not degrade over time.

### TS06: Manage Connectivity Lifecycle
- **As an** IoT Developer, **I want** connectivity to be abstracted behind a transport contract, **so that** link setup and health checks are portable across networks.
- **Acceptance Criteria**:
    - **Given** no active network link, **when** a connection is requested, **then** the framework initiates a non-blocking or bounded connection workflow.
    - **Given** a degraded or lost link, **when** connectivity is checked, **then** the framework reports not-ready state accurately.
    - **Given** a healthy link, **when** transmission readiness is queried, **then** the framework reports ready state consistently.

### TS07: Publish Telemetry to Edge Services
- **As an** IoT Developer, **I want** telemetry payload publication to be protocol-agnostic at the application level, **so that** I can switch transport strategies without changing device business logic.
- **Acceptance Criteria**:
    - **Given** a telemetry payload object, **when** publication is requested, **then** it is serialized to a transportable representation and sent through the selected gateway.
    - **Given** transient publication failure, **when** a publish attempt occurs, **then** the framework returns a clear success/failure result without crashing.
    - **Given** both request/response and broker-style integrations, **when** the same payload contract is used, **then** both publication paths are supported.

### TS08: Orchestrate Asynchronous Sampling and Dispatch
- **As an** IoT Developer, **I want** asynchronous sampling and dispatch orchestration, **so that** hardware polling and network publication do not block each other.
- **Acceptance Criteria**:
    - **Given** multiple registered input tasks, **when** the scheduler runs, **then** work is processed in a bounded round-robin sequence.
    - **Given** telemetry items are produced faster than they are transmitted, **when** buffering occurs, **then** queue behavior follows documented capacity rules.
    - **Given** long-running operation, **when** background workers execute, **then** input capture and dispatch remain isolated and responsive.

### TS09: Handle Data Quality and Failure Conditions
- **As an** IoT Developer, **I want** explicit handling for invalid or duplicate measurements, **so that** downstream logic receives meaningful signals instead of noisy data.
- **Acceptance Criteria**:
    - **Given** an invalid sensor reading, **when** processing occurs, **then** propagation is suppressed or marked as invalid per policy.
    - **Given** unchanged readings where change-only propagation is enabled, **when** sampling repeats, **then** duplicate events are suppressed.
    - **Given** operational errors, **when** they occur, **then** the framework fails safely and preserves runtime stability.

### TS10: Extend Framework Safely
- **As an** IoT Developer, **I want** stable extension points for inputs, outputs, telemetry payloads, and transports, **so that** I can customize behavior without forking the framework.
- **Acceptance Criteria**:
    - **Given** a new custom component implementation, **when** it adheres to framework contracts, **then** it integrates without changing existing modules.
    - **Given** a custom telemetry payload shape, **when** it is serialized through the payload contract, **then** gateways can publish it without special-case logic.
    - **Given** a new connectivity backend, **when** it implements the transport contract, **then** higher-level application code remains unchanged.

---

*Created: March 23, 2025.*
*Last Updated: June 6, 2026.*
