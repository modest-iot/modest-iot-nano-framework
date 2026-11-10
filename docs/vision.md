# Modest IoT Dual-Framework Vision

**Version**: 1.0.0  
**Author**: Angel Velasquez  
**Date**: 2026-11-10

---

## Executive Summary

The Modest IoT ecosystem consists of two complementary frameworks:

1. **Modest IoT Nano Framework** — ESP32 microcontroller application framework (C++ Arduino library)
2. **Modest IoT Edge Framework** — Raspberry Pi/edge server services (Python with SQLite)

Together, they form a complete IoT stack: Nano handles deterministic sensor polling, actuator control, and connectivity on resource-constrained microcontrollers, while Edge provides data aggregation, local intelligence, cloud relay, and device management on always-on edge servers.

---

## 1. Modest IoT Nano Framework (ESP32)

### Current Status

**Tiers 0–4 Complete, Tier 5 Nearly Complete** (as of 2026-11-10):

| Tier                                    | Items   | Status          |
|-----------------------------------------|---------|-----------------|
| **Tier 0**: Compilation & Hygiene       | W1–W4   | **Complete** ✓ |
| **Tier 1**: Test Consolidation & CI     | W5      | **Complete** ✓ |
| **Tier 2**: Reliability & Footprint     | W6–W10  | **Complete** ✓ |
| **Tier 3**: Core Scheduling & Telemetry | W8, W11 | **Complete** ✓ |
| **Tier 4**: Capabilities                | W12–W15 | **Complete** ✓   |
| **Tier 5**: Optimization & Power        | W16–W19 | **Complete** (W16 ✓, W17 ✓, W19 ✓, W18 deferred to v1.1.0) |

### What's Delivered

- **42 source headers** with full Doxygen documentation and `@runs_on` threading tags.
- **15 example sketches** covering diverse use cases (smart band, air quality, RFID access, smart greenhouse, weather station, crop monitoring, home security, etc.).
- **25 host unit test suites** with shared mocks, Address/Undefined sanitizers, and 100% pass rate.
- **3 CI workflows**: host tests, compile examples, lint.
- **Per-slot `SamplingScheduler`** with phase-aligned re-arming and minimum period enforcement.
- **`EngineConfig`** for tunable FreeRTOS task stacks, priorities, and core pinning.
- **`BleConnectivityDriver`** with Nordic UART Service (NUS) supporting all ESP32 variants.
- **Zero-cost `MODEST_LOG_*`** logging with compile-time level filtering.
- **`DiagnosticsSnapshot`** with 12 fields: stack high-water marks, heap stats, queue counters, send statistics.
- **Sensor fault events** (IDs 22/23) with debounced threshold state machine.
- **Telemetry pipeline** with `unique_ptr` ownership, drop-oldest policy, bounded serialization, envelope metadata.
- **MQTT keep-alive** with rate-limited reconnection backoff.
- **29 Architecture Decision Records** documenting all key architectural choices, including DDD adoption (ADR-023).

### Remaining Work: Tier 4 (Capabilities) — **Complete** ✓

All Tier 4 items (W12–W15) are implemented. The framework now supports bidirectional MQTT communication, interrupt-driven sensors, and non-blocking HX711/ultrasonic operation.

### Remaining Work: Tier 5 (Optimization & Power)

| ID      | Work Item                            | Priority | Complexity | Description                                                              |
|---------|--------------------------------------|----------|------------|--------------------------------------------------------------------------|
| **W16** | Tickless scheduling and light sleep  | Medium   | High       | **Complete** ✓ — `ticksUntilNextDue()`, PowerMode enum, ESP32 light sleep.   |
| **W17** | Deep-sleep cycle and battery example | Low      | High       | **Complete** ✓ — PowerMode::DeepSleep, enterDeepSleep(), reportWakeReason(), RTC_DATA_ATTR, WeatherStation example.  |
| **W18** | Telemetry pool and writer interface  | Medium   | High       | Static packet pool, `TelemetryWriter` abstraction for format decoupling. |
| **W19** | Native servo and LCD drivers         | Low      | Medium     | **Complete** ✓ — ServoMotor native LEDC, Hd44780Driver + CharacterLcdDisplay native I2C, library.properties updated.    |

### Component Expansion Tier 1 — **Complete** ✓

Zero-dependency components added for broader IoT coverage:

| Component                | Type     | Interface        | Use Case                                                                |
|--------------------------|----------|------------------|-------------------------------------------------------------------------|
| **RelayActuator**        | Actuator | `digitalWrite()` | AC/DC load control for smart home, greenhouse pumps, industrial relays  |
| **SoilMoistureSensor**   | Sensor   | `analogRead()`   | Smart greenhouse and agriculture monitoring                             |
| **WaterFlowSensor**      | Sensor   | Interrupt + ADC  | Water consumption tracking, leak detection, smart irrigation            |
| **WeatherStation**       | Example  | Deep Sleep       | Battery-powered weather monitoring with DHT22 and BH1750               |
| **CropMonitoringStation**| Example  | Tickless + MQTT  | Multi-sensor agricultural monitoring with MQTT telemetry               |

New examples: `SmartGreenhouseDevice` (Soil Moisture + DHT + Relay + ServoMotor), `WaterFlowMonitor` (YF-S201 hybrid scheduling), `WeatherStation` (deep sleep with DHT22 + BH1750), `CropMonitoringStation` (tickless scheduling with MQTT telemetry).

### Component Expansion Tier 2 — **Complete** ✓

Components for weather stations, home security, and industrial monitoring:

| Component                | Type     | Interface        | Use Case                                                                |
|--------------------------|----------|------------------|-------------------------------------------------------------------------|
| **PthSensor**            | Sensor   | I²C (BME280)     | Temperature, humidity, pressure for weather stations and air quality    |
| **WaterLeakSensor**      | Sensor   | `digitalRead()`  | Water leak detection for home security and industrial monitoring        |
| **DoorWindowSensor**     | Sensor   | `digitalRead()`  | Door/window open/close detection for home security and perimeter        |

New examples: `HomeSecurityDevice` (Water Leak + Door/Window + PIR + Buzzer), `WeatherStation` updated with PthSensor.

### Component Expansion Tier 3 — **In Progress**

Popular IoT components for maximum v1.0.0 adoption:

| Component                | Type     | Interface        | Use Case                                                                | Status |
|--------------------------|----------|------------------|-------------------------------------------------------------------------|--------|
| **OneWireTemperatureSensor** | Sensor | GPIO (1-Wire)    | High-accuracy temperature for weather stations and aquariums            | **Complete** |
| **OledDisplay**          | Actuator | I²C (SSD1306)    | OLED display for portable devices and dashboards                        | **Complete** |
| **Ssd1306Driver**        | Driver   | I²C (SSD1306)    | SSD1306 protocol driver for OledDisplay                                 | **Complete** |
| **EnvironmentalGasSensor** | Sensor | I²C (BME680)     | Air quality monitoring with VOC and gas resistance                      | Pending |

New examples: `OneWireTemperatureMonitor` (DS18B20 multi-sensor), `OledWeatherDisplay` (OLED + PthSensor), `AirQualityMonitorV2` (BME680 + MQTT).

### Nano Framework Completion Roadmap

**Phase 1: Tier 4 Completion** (Estimated: 1–2 weeks)

1. **W12: `Device::enqueueCommand` command marshaling** — **Complete**: Command queue consumed by sampler task with `CommandAvailable` bit-flag notification. Enables cloud-to-device commands.
2. **W13: Interrupt triggers** — **Complete**: Button/PIR converted to interrupt-driven, WaterFlowSensor added as hybrid sensor with `appendToBothSchedulers()`. All examples migrated.
3. **W14: Non-blocking sensors** — **Complete**: LoadCellAmplifier uses hybrid scheduling with interrupt-driven data-ready detection (appendToBothSchedulers). UltrasonicSensor uses GPIO interrupt on echo pin with esp_timer_get_time() for nanosecond precision. SmartScaleDevice example migrated.
4. **W15: MQTT subscribe commands** — **Complete**: Subscribe topic wired to `MqttGatewayClient`, JSON parsing extracts `commandId`, commands delivered via `Device::enqueueCommand()`. Legacy constructor preserves backward compatibility.

**Phase 2: Tier 5 Completion** (Estimated: 2–3 weeks)

1. **W16: Tickless scheduling** — Implement `ticksUntilNextDue()` in `SamplingScheduler`. Integrate with ESP32 light sleep API.
2. **W17: Deep sleep example** — **Complete** ✓: PowerMode::DeepSleep, enterDeepSleep(), reportWakeReason(), WeatherStation example with DHT22 + BH1750.
3. **W18: Telemetry pool** — Static packet pool with placement-new. `TelemetryWriter` interface for format decoupling.
4. **W19: Native drivers** — **Complete** ✓: ServoMotor native LEDC, CharacterLcdDisplay native I2C.

**Phase 3: Library Release** (Estimated: 1 week)

1. Tag `v1.0.0` with `CHANGELOG.md`.
2. Submit to Arduino Library Manager.
3. Create AI-assisted development guide.

### Component Expansion Strategy

**Goal**: Expand the Nano Framework component library to cover the top 3 IoT project categories (smart home, agriculture, weather/security) with minimal new dependencies, enabling adoption-ready examples.

**Tier 1: Zero-Dependency Components** (P0, **Complete** ✓)

Components requiring only `digitalRead()` or `analogRead()` — no external libraries:

| Component                | Type     | Interface        | Enables                                                                | Status |
|--------------------------|----------|------------------|------------------------------------------------------------------------|--------|
| **RelayActuator**        | Actuator | `digitalWrite()` | AC/DC load control for smart home, greenhouse pumps, industrial relays | ✓      |
| **SoilMoistureSensor**   | Sensor   | `analogRead()`   | Smart greenhouse and agriculture monitoring                            | ✓      |

Both components are implemented with host tests and integrated into the `SmartGreenhouseDevice` example.

**Tier 2: Common I²C/Analog Sensors** (P1, **Complete** ✓)

Widely available sensors with simple I²C or analog interfaces:

| Component              | Type   | Interface                     | Enables                               | Status |
|------------------------|--------|-------------------------------|---------------------------------------|--------|
| **PthSensor**          | Sensor | I²C (temp/humidity/pressure)  | Weather station and altitude tracking | ✓      |
| **WaterLeakSensor**    | Sensor | `digitalRead()`               | Home security flood detection         | ✓      |
| **DoorWindowSensor**   | Sensor | `digitalRead()` (reed switch) | Home security access monitoring       | ✓      |
| **WaterFlowSensor**    | Sensor | Interrupt (YF-S201, ~450 ppl) | Water consumption tracking, leak detection, smart irrigation (requires W13) | ✓      |

**Tier 3: Niche Components** (P2–P4, deferred)

Accelerometer/IMU (MPU6050/BNO055), rain sensor, flame sensor, stepper motor, OLED display — serve specific domains and may require new library dependencies.

**Example-Driven Adoption**:

New examples bundle multiple components to demonstrate complete use cases:

| New Example               | Components                                              | IoT Category |
|---------------------------|---------------------------------------------------------|--------------|
| **SmartGreenhouseDevice** | Soil Moisture + DHT + Relay (pump) + ServoMotor (vent)  | Agriculture  |
| **WaterFlowMonitor**      | WaterFlowSensor with hybrid scheduling                  | Utilities    |
| **WeatherStationDevice**  | BME280 + Rain Sensor + DigitalLightIntensitySensor      | Weather      |
| **HomeSecurityDevice**    | PIR + Water Leak + Door/Window + Buzzer + Relay (alarm) | Security     |

**Library Release Impact**:

- **v1.0.0**: Current 15 sensors + 6 actuators + Tier 1 and Tier 2 components (Relay, Soil Moisture, WaterFlow, PthSensor, Water Leak, Door/Window).
- **v1.1.0**: Tier 2 components (BME280, Water Leak, Door/Window) with new examples.
- **v1.2.0**: Tier 3 components as needed, based on community feedback.

### On-Device ML Inference (Tier 5)

**Goal**: Enable latency-critical on-device ML inference for safety decisions and pattern recognition, while keeping the core framework lightweight.

**Two-Tier Intelligence Model**:

| Tier | Location | Capability | Latency | RAM |
|---|---|---|---|---|
| **Nano** | ESP32 device | Statistical rules, threshold anomaly detection, lightweight ML | < 10 ms | ~10–60 KB (opt-in) |
| **Edge** | Raspberry Pi | Full ML inference (TFLite), time-series analysis, multi-sensor correlation | 100–1000 ms | Unlimited |

**`AiMicroModelController` Abstract Domain Service**:

An optional, opt-in component that wraps ML runtimes (TensorFlow Lite Micro, emlearn) behind a uniform interface. The controller is a DDD domain service — not a `Sensor` or `Actuator` — that devices use for inference on extracted features.

| Design Decision | Rationale |
|---|---|
| **Abstract base with runtime implementations** | Framework not locked into one ML runtime; developers choose TFLM (complex models) or emlearn (minimal footprint). |
| **Opt-in via build guards** | Wrapped in `#if defined(USE_TFLITE_MICRO)` or `#if defined(USE_EMLEARN)`. Not included in `ModestIoT.h` by default. |
| **Domain service pattern** | Standalone service that sensors/devices use; does not extend `Sensor` or `Actuator`. Reusable across multiple sensors. |
| **Edge manages models** | Model training, quantization, and OTA distribution happen on the Edge Framework. Nano receives model updates via MQTT. |

**Use Cases**:

| Use Case | Runtime | Latency | Nano or Edge? |
|---|---|---|---|
| **Vibration anomaly detection** | emlearn (~10 KB RAM) | < 10 ms | **Nano** — immediate motor shutdown |
| **Keyword spotting** | TFLM (~60 KB RAM) | < 100 ms | **Nano** — voice command without network |
| **Gesture recognition** | TFLM or emlearn | < 50 ms | **Nano** — IMU pattern recognition |
| **Multi-sensor correlation** | TFLite (Python) | 100–1000 ms | **Edge** — cross-device analysis |
| **Predictive maintenance** | Regression models | 100–1000 ms | **Edge** — long-term trend analysis |

**Bounded Context (ADR-023)**:

On-device ML inference belongs to the **Nano Device Context** for latency-critical decisions only. The Edge Framework handles model training, updates, and complex multi-sensor correlation. The published language extends to include model distribution:

```
Edge Framework → (OTA model update via MQTT) → AiMicroModelController → (inference) → Device::on()
```

**Library Release Impact**:

- **v1.0.0–v1.2.0**: Core framework without ML controller. Edge Framework handles all ML workloads.
- **v1.3.0+**: `AiMicroModelController` as an optional Tier 5 component, released after core stability and community feedback on ML use cases.

## 2. Modest IoT Edge Framework (Raspberry Pi)

### Vision

The Edge Framework runs on Raspberry Pi (or similar always-on edge servers) and provides:

- **Device gateway**: MQTT broker and BLE/Wi-Fi access point for Nano devices.
- **Data aggregation**: Collect telemetry from multiple Nano devices, store locally, relay to cloud.
- **Local intelligence**: Run ML models, rule engines, and anomaly detection on edge.
- **Device management**: OTA updates, configuration management, health monitoring.
- **Dashboard**: Local web interface for monitoring and control.

### Architecture

The Edge Framework follows **Domain-Driven Design (DDD)** principles with explicit bounded contexts, aggregates, repositories, and domain services (see ADR-023).

**Bounded Contexts**:
- **Nano Device Context**: ESP32 microcontroller. Aggregate: `Device`. Communicates via MQTT/BLE.
- **Edge Gateway Context**: Raspberry Pi. Aggregates: `DeviceProfile`, `Rule`. Manages devices and automation.
- **External Clients Context**: Dashboard, cloud relay, third-party integrations. Accesses Edge via REST API (Anti-Corruption Layer).

**Context Relationships**:
- **Nano ↔ Edge**: Published Language — MQTT topics and BLE NUS define the stable contract.
- **Edge → External**: Anti-Corruption Layer — REST API isolates internal models from external clients.

```
┌──────────────────────────────────────────────────────────────┐
│                    Modest IoT Edge Framework                 │
│                     (Raspberry Pi / Edge Server)             │
│                                                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐    │
│  │ MQTT Broker  │  │ BLE Gateway  │  │  Web Dashboard   │    │
│  │ (Mosquitto)  │  │ (BlueZ)      │  │  (Flask + HTMX)  │    │
│  └──────┬───────┘  └───────┬──────┘  └─────────┬────────┘    │
│         │                  │                   │             │
│  ┌──────▼──────────────────▼───────────────────▼──────────┐  │
│  │              Edge Service Core (DDD)                   │  │
│  │                                                        │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │  │
│  │  │ Device       │  │ Rule Engine  │  │ Telemetry    │  │  │
│  │  │ Registry     │  │ (Domain Svc) │  │ Repository   │  │  │
│  │  │ (Repository) │  │              │  │ (SQLite)     │  │  │
│  │  └──────────────┘  └──────────────┘  └──────────────┘  │  │
│  │                                                        │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │  │
│  │  │ ML Inference │  │ Cloud Relay  │  │ OTA Manager  │  │  │
│  │  │ (Domain Svc) │  │ (Domain Svc) │  │ (Domain Svc) │  │  │
│  │  └──────────────┘  └──────────────┘  └──────────────┘  │  │
│  └────────────────────────────────────────────────────────┘  │
│                                                              │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │              Connectivity Layer (ACL)                   │ │
│  │  MQTT (Published Language) │ BLE │ REST API (ACL)       │ │
│  └─────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────┘
         │                    │                    │
         ▼                    ▼                    ▼
   ┌──────────┐         ┌──────────┐         ┌──────────┐
   │ Nano Dev │         │ Nano Dev │         │ External │
   │ (ESP32)  │         │ (ESP32)  │         │ Clients  │
   └──────────┘         └──────────┘         └──────────┘```
```

### Core Components

#### 2.1 Device Registry (Repository)

**Purpose**: Repository for `DeviceProfile` aggregates — track connected Nano devices, their capabilities, and health status.

**DDD Pattern**: Repository — provides query interface for `DeviceProfile` aggregates without exposing internal storage details.

**Features**:
- Auto-discovery via MQTT LWT (Last Will and Testament) and BLE advertising.
- Device profiles: sensor types, sampling intervals, telemetry schema.
- Health monitoring: track last seen, fault events, diagnostics snapshots.
- Configuration management: push sampling intervals, fault thresholds, engine config.

**Aggregate: `DeviceProfile`**:
```python
# app/models/device_profile.py
from peewee import Model, CharField, DateTimeField, BooleanField, TextField
from datetime import datetime

class DeviceProfile(Model):
    """Aggregate root for a registered Nano device.
    
    Invariants:
    - last_seen is updated when telemetry is received.
    - device is marked offline after configured timeout.
    """
    id = CharField(primary_key=True)  # Device ID from Nano framework
    name = CharField()
    device_type = CharField()  # "SmartBand", "AirQualityMonitor", etc.
    sensors = TextField()  # JSON: sensor types and configurations
    actuators = TextField()  # JSON: actuator types and configurations
    online = BooleanField(default=False)
    last_seen = DateTimeField(null=True)
    health_status = CharField(default="online")  # online, warning, fault, offline
    created_at = DateTimeField(default=datetime.now)
    
    def mark_seen(self):
        """Update last_seen and mark device online."""
        self.last_seen = datetime.now()
        self.online = True
        self.health_status = "online"
        self.save()
    
    def is_offline(self, timeout_seconds=300):
        """Check if device is offline based on timeout."""
        if self.last_seen is None:
            return True
        return (datetime.now() - self.last_seen).total_seconds() > timeout_seconds
```

**Repository: `DeviceRegistry`**:
```python
# app/repositories/device_registry.py
from app.models.device_profile import DeviceProfile

class DeviceRegistry:
    """Repository for DeviceProfile aggregates."""
    
    @staticmethod
    def get(device_id: str) -> DeviceProfile | None:
        """Retrieve device profile by ID."""
        return DeviceProfile.get_or_none(DeviceProfile.id == device_id)
    
    @staticmethod
    def register(device_id: str, name: str, device_type: str) -> DeviceProfile:
        """Register a new device profile."""
        return DeviceProfile.create(
            id=device_id, name=name, device_type=device_type
        )
    
    @staticmethod
    def exists(device_id: str) -> bool:
        """Check if device is registered."""
        return DeviceProfile.select().where(
            DeviceProfile.id == device_id
        ).exists()
    
    @staticmethod
    def get_online() -> list[DeviceProfile]:
        """Retrieve all online devices."""
        return list(DeviceProfile.select().where(
            DeviceProfile.online == True
        ))
    
    @staticmethod
    def get_offline(timeout_seconds=300) -> list[DeviceProfile]:
        """Retrieve all offline devices based on timeout."""
        cutoff = datetime.now() - timedelta(seconds=timeout_seconds)
        return list(DeviceProfile.select().where(
            (DeviceProfile.last_seen == None) | 
            (DeviceProfile.last_seen < cutoff)
        ))
```

#### 2.2 Telemetry Ingestion Service (Domain Service)

**Purpose**: Domain service for processing incoming telemetry — receive, validate, and store telemetry from Nano devices.

**DDD Pattern**: Domain Service — contains business logic that doesn't naturally belong to an aggregate (telemetry validation, envelope metadata checks, gap detection).

**Features**:
- MQTT subscription to device topics (`modest/{device_id}/telemetry`).
- Telemetry validation: check envelope metadata (timestamp, sequence), detect gaps.
- Time-series storage: SQLite with time-series optimization.
- Real-time processing: sliding window aggregations, anomaly detection.

**Data Flow**:
```
Nano Device → MQTT → TelemetryIngestionService → Validate → TelemetryRepository → Store
                                                        ↓
                                                 DeviceRegistry.mark_seen()
                                                        ↓
                                                 RuleEngine.evaluate()
```

**Domain Service: `TelemetryIngestionService`**:
```python
# app/services/telemetry_ingestion.py
from app.repositories.device_registry import DeviceRegistry
from app.repositories.telemetry_repository import TelemetryRepository
from app.services.rule_engine import RuleEngine

class TelemetryIngestionService:
    """Domain service for processing incoming telemetry.
    
    Responsibilities:
    - Validate envelope metadata (timestamp, sequence).
    - Update device last_seen via DeviceRegistry.
    - Store telemetry via TelemetryRepository.
    - Trigger RuleEngine evaluation.
    """
    
    @staticmethod
    def process(payload: dict):
        """Process incoming telemetry message.
        
        Domain rules:
        - Envelope metadata must contain timestamp and sequence.
        - Device must be registered (auto-register if not).
        - Device last_seen is updated on valid telemetry.
        - Rules are evaluated after storage.
        """
        # Extract envelope metadata
        device_id = payload.get('deviceId')
        timestamp = payload.get('timestamp')
        sequence = payload.get('sequence')
        
        # Domain rule: validate envelope metadata
        if not timestamp or not sequence:
            raise ValueError("Missing envelope metadata")
        
        # Domain rule: ensure device is registered
        device = DeviceRegistry.get(device_id)
        if not device:
            device = DeviceRegistry.register(
                device_id=device_id,
                name=f"Device-{device_id}",
                device_type="unknown"
            )
        
        # Domain rule: update device last_seen
        device.mark_seen()
        
        # Domain rule: store telemetry record
        TelemetryRepository.store(payload)
        
        # Domain rule: evaluate rules
        RuleEngine.evaluate(device_id, payload)
```

**Telemetry Schema**:
```json
{
  "timestamp": 1234567,
  "sequence": 42,
  "deviceId": "smart-band-001",
  "heartBeatsPerMinute": 72,
  "skinTemperatureInCelsius": 36.5,
  "relativeHumidityPercentage": 45.2
}
```

#### 2.3 Command Dispatch Service (Domain Service)

**Purpose**: Domain service for sending commands to Nano devices via MQTT or BLE.

**DDD Pattern**: Domain Service — orchestrates command delivery across multiple aggregates and external systems.

**Features**:
- Command queue per device, ordered delivery.
- Command types: actuator control, configuration update, diagnostics request, OTA trigger.
- Acknowledgment tracking: wait for command completion confirmation.
- Retry logic with exponential backoff.

**Command Schema**:
```json
{
  "commandId": "cmd-001",
  "deviceId": "smart-band-001",
  "type": "actuator_control",
  "payload": {
    "actuator": "led",
    "action": "toggle"
  }
}
```

#### 2.4 Rule Engine (Domain Service)

**Purpose**: Domain service for executing automation rules based on telemetry events.

**DDD Pattern**: Domain Service — contains business logic for rule evaluation that spans multiple aggregates.

**Aggregate: `Rule`**:
```python
# app/models/rule.py
from peewee import Model, CharField, TextField, BooleanField

class Rule(Model):
    """Aggregate root for automation rules.
    
    Invariants:
    - Condition must reference a valid device.
    - Action must be a valid command type.
    """
    id = CharField(primary_key=True)
    name = CharField()
    condition = TextField()  # JSON: condition definition
    action = TextField()     # JSON: action definition
    enabled = BooleanField(default=True)
    
    def validate_condition(self, device_registry):
        """Validate that condition references valid devices."""
        referenced_devices = self._extract_device_ids(self.condition)
        for device_id in referenced_devices:
            if not device_registry.exists(device_id):
                raise ValueError(f"Rule references non-existent device: {device_id}")
```

**Domain Service: `RuleEngine`**:
```python
# app/services/rule_engine.py
from app.models.rule import Rule
from app.services.command_dispatch import CommandDispatchService

class RuleEngine:
    """Domain service for evaluating automation rules."""
    
    @staticmethod
    def evaluate(device_id: str, telemetry: dict):
        """Evaluate all enabled rules against incoming telemetry."""
        rules = Rule.select().where(Rule.enabled == True)
        for rule in rules:
            if rule._matches_condition(device_id, telemetry):
                CommandDispatchService.dispatch(rule.action)
```

**Features**:
- Rule definition: IF-THEN rules with conditions and actions.
- Conditions: threshold comparisons, time windows, sequence patterns.
- Actions: send notification, trigger actuator, start ML inference, log event.
- Integration with Node-RED for visual rule editing.

**Example Rule**:
```yaml
rule: "High Temperature Alert"
condition:
  device_id: "air-quality-001"
  field: "gasConcentrationInPpm"
  operator: ">"
  threshold: 500
action:
  type: "notification"
  channel: "email"
  message: "Gas concentration exceeded 500 PPM"
```

#### 2.5 ML Inference Service (Domain Service)

**Purpose**: Domain service for running machine learning models on edge for anomaly detection and prediction.

**DDD Pattern**: Domain Service — encapsulates ML logic that doesn't belong to a single aggregate.

#### 2.6 Cloud Relay Service (Domain Service)

**Purpose**: Domain service for bridging edge data to cloud services (AWS IoT, GCP IoT Core, Azure IoT Hub).

**DDD Pattern**: Domain Service — handles protocol translation and cloud communication.

#### 2.7 OTA Management Service (Domain Service)

**Purpose**: Domain service for managing firmware updates for Nano devices.

**DDD Pattern**: Domain Service — orchestrates firmware lifecycle across devices.

#### 2.8 Web Dashboard

**Purpose**: Local web interface for monitoring and control.

**Features**:
- Real-time telemetry visualization: charts, gauges, maps.
- Device status overview: online/offline, health, last seen.
- Command interface: send commands to actuators, request diagnostics.
- Rule management: create, edit, and test automation rules.
- Alert history: view and acknowledge alerts.

**Tech Stack**:
- Backend: Flask (Python) with Flask-SocketIO for real-time updates.
- ORM: Peewee for database model definitions and queries.
- Frontend: HTMX for rich interactivity without JavaScript SPA complexity.
- Charts: Chart.js loaded per-page for telemetry visualizations.
- Styling: Tailwind CSS or Bootstrap for rapid UI development.
- Authentication: Flask-Login for local user accounts with role-based access control.

### Technology Stack

| Component         | Technology                   | Rationale                                            |
|-------------------|------------------------------|------------------------------------------------------|
| **Language**      | Python 3.11+                 | Rapid development, ML ecosystem, Raspberry Pi native |
| **MQTT Broker**   | Mosquitto                    | Lightweight, well-supported, Nano-compatible         |
| **BLE Stack**     | BlueZ                        | Linux native, supports NUS GATT profile              |
| **Web Framework** | Flask                        | Simple, mature ecosystem, Peewee integration         |
| **ORM**           | Peewee                       | Pythonic API, migrations, relationship handling      |
| **Frontend**      | HTMX                         | Rich interactivity, ~14 KB JS, no build pipeline     |
| **Charts**        | Chart.js                     | Lightweight, HTMX-compatible visualizations          |
| **Database**      | SQLite                       | Local storage, zero-config, time-series capable      |
| **ML Runtime**    | TensorFlow Lite              | Edge-optimized, Nano-compatible models               |
| **Container**     | Docker / Docker Compose      | Easy deployment, service isolation                   |
| **Config**        | YAML / environment variables | Simple, version-controllable                         |

### Nano-Edge Communication Protocols

#### 2.9 MQTT Protocol

**Primary communication channel** for Wi-Fi-connected Nano devices.

**Topic Structure**:
```
modest/{device_id}/telemetry      (Nano → Edge, QoS 1)
modest/{device_id}/commands       (Edge → Nano, QoS 1)
modest/{device_id}/diagnostics    (Nano → Edge, QoS 0)
modest/{device_id}/health         (Nano → Edge, QoS 1)
modest/{device_id}/ota            (Edge → Nano, QoS 1)
```

**Telemetry Message**:
```json
{
  "timestamp": 1234567,
  "sequence": 42,
  "heartBeatsPerMinute": 72,
  "skinTemperatureInCelsius": 36.5
}
```

**Command Message**:
```json
{
  "commandId": "cmd-001",
  "type": "actuator_control",
  "payload": {
    "actuator": "led",
    "action": "toggle"
  }
}
```

#### 2.10 BLE Protocol

**Secondary communication channel** for BLE-only Nano devices (no Wi-Fi).

**GATT Service**:
- Uses Nordic UART Service (NUS) profile, matching `BleConnectivityDriver`.
- TX characteristic: Nano → Edge (telemetry, diagnostics).
- RX characteristic: Edge → Nano (commands, configuration).

**Message Format**:
- Length-prefixed JSON: `[2-byte length][JSON payload]`.
- Edge service parses and routes to MQTT/topic system.

**Discovery**:
- Nano devices advertise with custom manufacturer data: `ModestIoT-{device_id}`.
- Edge service scans and auto-connects to known devices.

#### 2.11 HTTP API

**REST API** for external integration and dashboard.

**Endpoints**:
```
GET    /api/devices              — List all registered devices
GET    /api/devices/{id}         — Get device profile and status
GET    /api/devices/{id}/telemetry — Get recent telemetry
POST   /api/devices/{id}/command — Send command to device
GET    /api/alerts               — List active alerts
POST   /api/rules                — Create automation rule
GET    /api/diagnostics          — Get edge service health
```

### Deployment Models

#### Model A: Single Raspberry Pi

**Use case**: Small deployments (1-10 Nano devices), home automation, education.

**Components**:
- Raspberry Pi 4/5 with Mosquitto, Edge services, SQLite.
- All Nano devices connect via Wi-Fi or BLE to the Pi.
- Local dashboard accessible via browser.

**Resources**:
- RAM: 2GB minimum (4GB recommended).
- Storage: 16GB SD card (32GB recommended for telemetry history).
- Power: 5V/3A USB-C.

#### Model B: Docker Compose Cluster

**Use case**: Medium deployments (10-100 Nano devices), industrial monitoring.

**Components**:
- Docker Compose with separate containers for each service.
- SQLite for time-series storage.
- Node-RED for visual rule editing.
- Nginx reverse proxy for HTTPS.

**docker-compose.yml**:
```yaml
version: '3.8'
services:
  mqtt:
    image: eclipse-mosquitto:latest
  edge-core:
    image: modest-iot/edge-core:latest
  dashboard:
    image: modest-iot/dashboard:latest
  database:
    image: modest-iot/edge-db:latest  # SQLite-based
  nodered:
    image: nodered/node-red:latest
```

#### Model C: Multi-Edge with Cloud Sync

**Use case**: Large deployments (100+ Nano devices), multi-site, cloud backup.

**Components**:
- Multiple edge servers, each managing a subset of Nano devices.
- Cloud relay service syncs aggregated data to cloud.
- Central cloud dashboard for cross-site monitoring.
- Edge-to-edge communication for load balancing.

### Development Phases

#### Phase 1: Core Services (Estimated: 4-6 weeks)

1. **MQTT broker setup** — Mosquitto configuration, topic structure, authentication.
2. **Device registry** — Auto-discovery, device profiles, health tracking.
3. **Telemetry ingestion** — MQTT subscription, validation, SQLite storage.
4. **Command dispatch** — Command queue, acknowledgment, retry logic.
5. **Basic dashboard** — Device list, real-time telemetry, command interface.

**Deliverables**:
- Working MQTT bridge between Nano devices and edge server.
- Device auto-discovery and health monitoring.
- Basic web dashboard for monitoring.

#### Phase 2: Intelligence & Automation (Estimated: 4-6 weeks)

1. **Rule engine** — IF-THEN rules, conditions, actions.
2. **ML inference service** — Model loading, real-time inference, anomaly detection.
3. **Alert system** — Email, SMS, push notifications.
4. **Advanced dashboard** — Charts, gauges, alert history, rule management.

**Deliverables**:
- Automation rules for common use cases.
- Anomaly detection for sensor data.
- Alert notifications when thresholds are exceeded.

#### Phase 3: Cloud & OTA (Estimated: 4-6 weeks)

1. **Cloud relay service** — AWS/GCP/Azure integration.
2. **OTA management** — Firmware repository, update scheduling, rollback.
3. **Multi-edge support** — Edge-to-edge communication, load balancing.
4. **Security hardening** — TLS, authentication, authorization.

**Deliverables**:
- Cloud integration for remote monitoring.
- OTA firmware updates for Nano devices.
- Secure communication with TLS and authentication.

#### Phase 4: BLE & Advanced Features (Estimated: 4-6 weeks)

1. **BLE gateway** — BlueZ integration, NUS profile, auto-connect.
2. **Advanced ML** — Model training on edge, federated learning.
3. **Edge-to-edge mesh** — Decentralized coordination, fault tolerance.
4. **MicroPython edition** — Nano framework port for education.

**Deliverables**:
- BLE support for Nano devices without Wi-Fi.
- Advanced ML capabilities on edge.
- MicroPython edition for rapid prototyping.

---

## 3. Nano-Edge Integration Points

### 3.1 Shared Architectural Patterns (DDD)

Both frameworks share the same architectural DNA, guided by Domain-Driven Design principles (ADR-023):

| Pattern                   | Nano Framework (Embedded DDD Subset)                      | Edge Framework (Full DDD)                                            |
|---------------------------|-----------------------------------------------------------|----------------------------------------------------------------------|
| **Bounded Context**       | Nano Device Context (ESP32)                               | Edge Gateway Context (Raspberry Pi)                                  |
| **Aggregate Root**        | `Device` — orchestrates sensors/actuators                 | `DeviceProfile`, `Rule` — own business invariants                    |
| **Repository**            | `SamplingScheduler` — sensor slot management              | `DeviceRegistry`, `TelemetryRepository` — query interfaces           |
| **Domain Service**        | Implicit in `Device` mediator logic                       | `TelemetryIngestionService`, `RuleEngine`, `CommandDispatchService`  |
| **Value Objects**         | `Event`, `Command`, `EngineConfig`, `DiagnosticsSnapshot` | Telemetry payload, command payload, rule condition                   |
| **Domain Events**         | `Event`/`EventHandler` system (IDs 0–999)                 | `TelemetryReceivedEvent`, `DeviceOfflineEvent`, `RuleTriggeredEvent` |
| **CQRS**                  | `Command`/`Event` separation                              | Command dispatch / telemetry ingestion                               |
| **Anti-Corruption Layer** | `ConnectivityDriver` isolates Arduino/ESP-IDF APIs        | REST API isolates Peewee ORM from external clients                   |
| **Ubiquitous Language**   | `Device`, `Sensor`, `Actuator`, `TelemetryPackage`        | Shared dictionary (see ADR-023)                                      |
| **Logging**               | `MODEST_LOG_*` macros                                     | Python logging with structured format                                |
| **Identifier Registry**   | Framework 0–999, user 1000+                               | Device IDs, command IDs, rule IDs                                    |

### 3.2 Shared Naming Conventions

| Domain                | Convention             | Nano Example              | Edge Example           |
|-----------------------|------------------------|---------------------------|------------------------|
| **Classes/Types**     | `PascalCase`           | `TelemetryPackage`        | `DeviceRegistry`       |
| **Functions/Methods** | `camelCase`            | `enqueueTelemetryPayload` | `ingestTelemetry`      |
| **Constants/Macros**  | `SCREAMING_SNAKE_CASE` | `MODEST_LOG_E`            | `LOG_LEVEL`            |
| **Files/Scripts**     | `snake_case`           | `run_all.sh`              | `start_services.sh`    |
| **Documentation**     | `kebab-case`           | `improvements.md`         | `edge-improvements.md` |

### 3.3 Shared Telemetry Schema

Both frameworks use the same telemetry envelope:

```json
{
  "timestamp": 1234567,
  "sequence": 42,
  "deviceId": "smart-band-001",
  "schemaVersion": 1,
  // ... sensor-specific fields ...
}
```

This ensures:
- Consistent time-series alignment across Nano and Edge.
- Drop detection via sequence gaps.
- Schema evolution with version tracking.

### 3.4 Shared Identifier Spaces

| Space           | Range                       | Nano Usage                  | Edge Usage                |
|-----------------|-----------------------------|-----------------------------|---------------------------|
| **Device IDs**  | UUID or custom string       | `device_id` in telemetry    | Device registry key       |
| **Command IDs** | UUID or sequential          | `commandId` in commands     | Command dispatch tracking |
| **Event IDs**   | 0-999 framework, 1000+ user | Sensor/actuator events      | Edge service events       |
| **Alert IDs**   | Sequential                  | Sensor fault events (22/23) | Edge alert system         |

### 3.5 Ubiquitous Language Dictionary

A shared glossary ensures consistent terminology across both frameworks (see ADR-023):

| Term          | Definition                              | Nano Equivalent                   | Edge Equivalent                      |
|---------------|-----------------------------------------|-----------------------------------|--------------------------------------|
| **Device**    | Physical IoT endpoint                   | `Device` class (aggregate root)   | `DeviceProfile` aggregate            |
| **Sensor**    | Reads physical measurements             | `Sensor` subclass                 | Telemetry field in `TelemetryRecord` |
| **Actuator**  | Executes commands on GPIO               | `Actuator` subclass               | Command action in `Rule`             |
| **Telemetry** | Measurement data sent to Edge           | `TelemetryPackage` (value object) | `TelemetryRecord` entity             |
| **Event**     | Domain event signaling state            | `Event` class                     | `TelemetryReceivedEvent`             |
| **Command**   | Intent to perform an action             | `Command` class                   | MQTT command payload                 |
| **Rule**      | Automation condition and action         | N/A (local mediator)              | `Rule` aggregate                     |
| **Alert**     | Triggered condition requiring attention | Sensor fault event (ID 22)        | `Alert` entity                       |
| **Online**    | Device sent telemetry within timeout    | `Device::diagnostics()`           | `DeviceProfile.online`               |
| **Offline**   | Device exceeded telemetry timeout       | N/A                               | `DeviceProfile.is_offline()`         |

---

## 4. Strategic Advantages

### 4.1 For Developers

| Advantage                   | Nano Framework          | Edge Framework                   | Combined                   |
|-----------------------------|-------------------------|----------------------------------|----------------------------|
| **Consistent patterns**     | Mediator, CQRS, events  | Same patterns at edge scale      | One mental model for both  |
| **AI-assisted development** | Clean, documented API   | Python/ML ecosystem              | AI generates code for both |
| **Test infrastructure**     | 22 host test suites, CI | Python pytest, integration tests | Full-stack testing         |
| **Documentation**           | Doxygen, ADRs, README   | Sphinx, OpenAPI specs            | Unified documentation      |

### 4.2 For IoT Products

| Advantage                    | Description                                                       |
|------------------------------|-------------------------------------------------------------------|
| **Local-first architecture** | Edge processes data locally; cloud is optional backup.            |
| **Offline resilience**       | Edge continues to operate without cloud connection.               |
| **Low latency**              | Local processing avoids cloud round-trip latency.                 |
| **Privacy**                  | Sensitive data stays on edge; only aggregated data goes to cloud. |
| **Scalability**              | Add edge servers for more devices; no central bottleneck.         |
| **Cost efficiency**          | Local processing reduces cloud data transfer costs.               |

### 4.3 Competitive Positioning

| Competitor             | Nano Equivalent   | Edge Equivalent     | Modest IoT Advantage                          |
|------------------------|-------------------|---------------------|-----------------------------------------------|
| **ESP-IDF**            | Native ESP32 SDK  | None                | Arduino compatibility, architectural patterns |
| **Zephyr**             | Cross-vendor RTOS | Zephyr applications | Simpler, ESP32-focused, Arduino ecosystem     |
| **Home Assistant**     | ESPHome (YAML)    | HA core             | Custom C++ logic, not YAML-limited            |
| **AWS IoT Greengrass** | Custom firmware   | Greengrass core     | Lightweight, no AWS lock-in                   |
| **Balena**             | Custom firmware   | BalenaOS            | Open-source, no cloud dependency              |

---

## 5. Development Timeline

### Nano Framework Completion

| Quarter     | Milestone              | Deliverables                                                                   |
|-------------|------------------------|--------------------------------------------------------------------------------|
| **Q4 2026** | Tier 4 complete ✓      | W12–W15: command marshaling, interrupts, non-blocking sensors, MQTT subscribe |
| **Q4 2026** | Component Expansion ✓  | RelayActuator, SoilMoistureSensor, WaterFlowSensor with examples              |
| **Q4 2026** | Tier 5 in progress     | W16 ✓: tickless scheduling and light sleep; W17 ✓: deep sleep with WeatherStation; W19 ✓: native drivers; W18 pending |
| **Q1 2027** | Library release        | v1.0.0 tag, Arduino Library Manager submission                                 |

### Edge Framework Development

| Quarter     | Milestone        | Deliverables                                                               |
|-------------|------------------|----------------------------------------------------------------------------|
| **Q1 2027** | Phase 1 complete | Core services: MQTT, device registry, telemetry ingestion, basic dashboard |
| **Q2 2027** | Phase 2 complete | Intelligence: rule engine, ML inference, alerts, advanced dashboard        |
| **Q3 2027** | Phase 3 complete | Cloud & OTA: cloud relay, OTA management, security hardening               |
| **Q4 2027** | Phase 4 complete | BLE & advanced: BLE gateway, advanced ML, MicroPython edition              |

### Integrated System

| Quarter     | Milestone          | Deliverables                                                  |
|-------------|--------------------|---------------------------------------------------------------|
| **Q2 2027** | Alpha integration  | Nano v1.0.0 + Edge Phase 2, end-to-end telemetry and commands |
| **Q3 2027** | Beta release       | Full integration, cloud relay, OTA, multi-device support      |
| **Q4 2027** | Production release | Stable release, documentation, examples, community support    |

---

## 6. Open Questions

### 6.1 Edge Framework Language

**Decision**: Python for all Edge Framework services.

**Rationale**:
- **ML ecosystem**: TensorFlow, scikit-learn, and emlearn integration for edge inference.
- **Raspberry Pi native**: Python is pre-installed and well-supported on Raspberry Pi OS.
- **Flask for web services**: Simple, mature ecosystem, massive tutorial base.
- **Peewee ORM**: Pythonic model definitions, migrations, relationship handling.
- **HTMX for frontend**: ~14 KB JavaScript, no build pipeline, server-driven interactivity.
- **Unified stack**: Single language simplifies development, testing, and deployment.
- **Data processing**: Pandas and NumPy for time-series analysis and aggregations.

### 6.2 Database Choice

**Decision**: SQLite for all deployment models.

**Rationale**:
- **Zero configuration**: No separate database server to manage.
- **Sufficient for edge workloads**: Handles thousands of telemetry messages per second.
- **Portability**: Single-file database simplifies backups and migrations.
- **Time-series capable**: With proper indexing, handles time-series queries efficiently.

**Scaling Strategy**:
- Use SQLite WAL mode for concurrent read/write access.
- Implement data retention policies to archive old data.
- Consider TimescaleDB only if write throughput exceeds SQLite's capabilities.

### 6.3 Cloud Integration

**Question**: Which cloud providers to support first?

**Analysis**:
- **AWS IoT Core**: Largest market share, mature ecosystem.
- **GCP IoT Core**: Strong ML integration, Edge TPU support.
- **Azure IoT Hub**: Enterprise focus, digital twins.

**Recommendation**: AWS IoT Core first (largest market), then GCP (ML integration), then Azure (enterprise).

### 6.4 BLE Gateway Implementation

**Question**: BlueZ native vs hci-tools vs custom BLE stack?

**Analysis**:
- **BlueZ native**: Linux standard, well-documented, NUS support.
- **hci-tools**: Lower-level, more control, more complex.
- **Custom stack**: Full control, high maintenance burden.

**Recommendation**: BlueZ native with Python `bleak` library for NUS communication.

---

## 7. Success Metrics

### Nano Framework

| Metric                  | Target                                 |
|-------------------------|----------------------------------------|
| **Compilation success** | 100% across ESP32 variants             |
| **Test coverage**       | 100% pass rate, 25 test suites         |
| **Flash footprint**     | <30 KB overhead                        |
| **RAM footprint**       | <2 KB overhead                         |
| **Example count**       | 15 diverse examples                    |
| **CI workflows**        | 3 (host tests, compile examples, lint) |

### Edge Framework

| Metric                       | Target                            |
|------------------------------|-----------------------------------|
| **Device capacity**          | 100+ Nano devices per edge server |
| **Telemetry throughput**     | 1000+ messages/second             |
| **Latency**                  | <100ms local processing           |
| **Uptime**                   | 99.9% with auto-restart           |
| **Dashboard responsiveness** | <1 second for real-time updates   |
| **ML inference**             | <50ms per prediction              |

### Integrated System

| Metric                 | Target                                     |
|------------------------|--------------------------------------------|
| **End-to-end latency** | <500ms (Nano → Edge → Dashboard)           |
| **Offline resilience** | Edge operates without cloud for 30+ days   |
| **OTA success rate**   | 99%+ successful updates                    |
| **Alert accuracy**     | <5% false positive rate                    |
| **Scalability**        | 1000+ devices across multiple edge servers |

---

## 8. Conclusion

The Modest IoT dual-framework vision provides a complete, local-first IoT stack:

- **Nano Framework** delivers deterministic, architecturally disciplined microcontroller code for ESP32.
- **Edge Framework** provides data aggregation, local intelligence, and device management on Raspberry Pi.
- **Together** they form a scalable, offline-resilient, AI-ready IoT platform.
- **DDD-guided design** ensures consistent terminology, clear context boundaries, and appropriate patterns for each framework (see ADR-023).

The Nano Framework is **97% complete** (Tiers 0–4 done, Component Expansion Tier 1 and Tier 2 done, Tier 5: W16 ✓, W17 ✓, W19 ✓). The Edge Framework is **planned** with a clear DDD-based architecture and development roadmap.

**Next steps**:
1. Nano Framework Tier 4 is complete (W12–W15 ✓). Component Expansion Tier 1 is complete (RelayActuator, SoilMoistureSensor ✓). Component Expansion Tier 2 is complete (PthSensor, WaterLeakSensor, DoorWindowSensor ✓). Tier 5: W16 ✓ (tickless scheduling), W17 ✓ (deep sleep), W19 ✓ (native drivers).
2. Complete Tier 5 with W18 (telemetry pool), or start Edge Framework Phase 1.
3. Integrate Nano and Edge with MQTT protocol and shared telemetry schema.
4. Release Nano v1.0.0 and Edge alpha together.

This vision positions Modest IoT as a **leading open-source IoT platform** for developers who want architectural discipline, local-first processing, and AI-ready infrastructure — without vendor lock-in or cloud dependency.
