# Migration Guide: From `loop()` to Modest IoT

This guide shows how to convert a traditional Arduino sketch that uses `loop()`, `millis()`, and blocking calls into an event-driven Modest IoT Nano Framework application.

Each section covers one migration pattern with a **before** (traditional `loop()`) and **after** (framework) code example.

---

## Migrating from v1.2.1 to v1.3.0

Existing `Device`, sensor, and gateway usage can continue as before. The release adds the following opt-in capabilities:

- **BLE compatibility**: `BleHal` supports ESP32 Arduino core 3.x and 4.x; core 4.0.0 is no longer required solely for BLE.
- **Change-only telemetry**: Configure a per-sensor deadband through the `appendSensorToScheduler()` overloads to suppress insignificant measurement changes.
- **Composite-event automation**: Connect `EventAggregator` to `RulesEngine` to dispatch one or more actuator commands for a matching composite event.
- **On-device inference**: Use `AiMicroModelController` and `EmlearnController` for local prediction or regression. Generated C models do not require the emlearn package; other emlearn model APIs are optional.
- **MPU6050 sensing**: Use `ImuSensor` for calibrated inertial readings and windowed vibration statistics; it requires an MPU6050 on the device's I²C bus.

MQTT remains the only required external Arduino library dependency.

---

## Why Migrate?

Traditional Arduino sketches have several limitations:

| Problem | Traditional `loop()` | Modest IoT Framework |
|---|---|---|
| **Blocking calls** | `delay()` freezes the entire sketch | Hardware timers + FreeRTOS tasks |
| **Spaghetti logic** | All code in one `loop()` function | Separated into Sensors, Device, Actuators |
| **Timing bugs** | Hand-written `millis()` flags are error-prone | Framework scheduler handles all timing |
| **Scalability** | Adding sensors means more `loop()` code | Register sensor → done |
| **Testing** | Hard to test without hardware | Host tests via pure C++ interfaces |
| **Maintainability** | Global state scattered everywhere | Encapsulated in `Device` mediator |

---

## Pattern 1: From `delay()` to Event-Driven Timing

### Before: Blocking LED Blink

The classic Arduino blink example blocks the entire sketch for 1 second per toggle:

```cpp
const int ledPin = 2;

void setup() {
    pinMode(ledPin, OUTPUT);
}

void loop() {
    digitalWrite(ledPin, HIGH);
    delay(500);  // Blocks everything for 500ms
    digitalWrite(ledPin, LOW);
    delay(500);  // Blocks everything for another 500ms
}
```

**Problems**: Nothing else can happen during `delay()`. No sensor reads, no network, no button handling.

### After: Event-Driven LED Toggle

```cpp
#include <Arduino.h>
#include <ModestIoT.h>

class Blinker : public Device {
    Led led;
public:
    static constexpr int TIMER_EVENT = 300;

    Blinker(int pin, unsigned long intervalMs, uint8_t timerChannel = 0)
    : Device(intervalMs, timerChannel), led(pin, false, this)
    {
        initializeAsynchronousEngine(5);
        appendSensorToScheduler(this, TIMER_EVENT, intervalMs, 0);
    }

    void on(Event event) override {
        if (event.identifier == TIMER_EVENT) {
            led.handle(Led::TOGGLE_LED_COMMAND);
        }
    }
};

static Blinker* app;

void setup() {
    app = new Blinker(2, 500);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(60000));  // Yield to FreeRTOS
}
```

**Benefits**:
- `loop()` is empty — all work happens in background FreeRTOS tasks.
- The LED toggles every 500ms via hardware timer, not `delay()`.
- You can add sensors, network, or buttons without affecting the LED timing.

---

## Pattern 2: From `millis()` Timers to Scheduler

### Before: Non-Blocking millis() Timer

A common pattern to avoid `delay()` is hand-written `millis()` timing:

```cpp
const int ledPin = 2;
const int sensorPin = 4;
unsigned long lastBlink = 0;
unsigned long lastRead = 0;
bool ledState = false;

void setup() {
    pinMode(ledPin, OUTPUT);
    pinMode(sensorPin, INPUT);
    Serial.begin(115200);
}

void loop() {
    // LED blink every 500ms
    if (millis() - lastBlink >= 500) {
        lastBlink = millis();
        ledState = !ledState;
        digitalWrite(ledPin, ledState ? HIGH : LOW);
    }

    // Sensor read every 1000ms
    if (millis() - lastRead >= 1000) {
        lastRead = millis();
        int value = analogRead(sensorPin);
        Serial.println(value);
    }

    // More timing flags for other tasks...
}
```

**Problems**: Each new timer adds more `millis()` checks, more global state, and more chances for timing bugs (overflow, drift, missed readings).

### After: Scheduler-Managed Timing

```cpp
#include <Arduino.h>
#include <ModestIoT.h>

class SensorDevice : public Device {
    Led led;
    SoilMoistureSensor sensor;
public:
    static constexpr int BLINK_EVENT = 300;
    static constexpr int SENSOR_EVENT = 301;

    SensorDevice(int ledPin, int sensorPin)
    : Device(1000, 0), led(ledPin, false, this), sensor(sensorPin, nullptr)
    {
        sensor.setHandler(this);
        initializeAsynchronousEngine(5);

        // LED blinks every 500ms
        appendSensorToScheduler(this, BLINK_EVENT, 500, 0);
        // Sensor reads every 1000ms
        appendSensorToScheduler(&sensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    }

    void on(Event event) override {
        if (event.identifier == BLINK_EVENT) {
            led.handle(Led::TOGGLE_LED_COMMAND);
        }
        else if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
            Serial.println(sensor.getMoistureLevel());
        }
    }
};

static SensorDevice* app;

void setup() {
    Serial.begin(115200);
    app = new SensorDevice(2, 4);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(60000));
}
```

**Benefits**:
- No `millis()` variables, no timing flags, no overflow handling.
- Each sensor and timer is registered independently.
- The framework guarantees precise timing via hardware timers.

---

## Pattern 3: From Blocking Sensor Reads to Non-Blocking Handlers

### Before: Blocking Sensor Read

```cpp
#include <DHT.h>

DHT dht(4, DHT22);

void setup() {
    dht.begin();
    Serial.begin(115200);
}

void loop() {
    // This blocks for ~250ms while the sensor reads
    float temp = dht.readTemperature();
    float humidity = dht.readHumidity();
    Serial.printf("Temp: %.1f, Humidity: %.1f\n", temp, humidity);
    delay(2000);  // Wait before next read
}
```

**Problems**: `dht.readTemperature()` blocks the entire sketch for ~250ms. Nothing else can happen during that time.

### After: Non-Blocking Sensor Handler

```cpp
#include <Arduino.h>
#include <ModestIoT.h>

class ClimateDevice : public Device {
    DhtSensor climateSensor;
public:
    ClimateDevice(int pin)
    : Device(2000, 0), climateSensor(pin, DhtSensor::SENSOR_MODEL_DHT22, nullptr)
    {
        climateSensor.setHandler(this);
        initializeAsynchronousEngine(5);
        appendSensorToScheduler(&climateSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    }

    void on(Event event) override {
        if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
            Serial.printf("Temp: %.1f, Humidity: %.1f\n",
                climateSensor.getTemperatureInCelsius(),
                climateSensor.getRelativeHumidityInPercentage());
        }
    }
};

static ClimateDevice* app;

void setup() {
    Serial.begin(115200);
    app = new ClimateDevice(4);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(60000));
}
```

**Benefits**:
- The DHT sensor read runs in a background FreeRTOS task, not the main loop.
- Other tasks (LED, network, buttons) continue running during the sensor read.
- The framework's `DhtSensor` implements the single-wire protocol natively — no external library needed.

---

## Pattern 4: From Globals to Device-Mediated Architecture

### Before: Scattered Global State

```cpp
const int ledPin = 2;
const int sensorPin = 4;
const int buttonPin = 14;

bool ledState = false;
int sensorValue = 0;
bool buttonPressed = false;
unsigned long lastAction = 0;

void setup() {
    pinMode(ledPin, OUTPUT);
    pinMode(sensorPin, INPUT);
    pinMode(buttonPin, INPUT_PULLUP);
    Serial.begin(115200);
}

void loop() {
    // LED logic
    if (millis() - lastAction >= 500) {
        lastAction = millis();
        ledState = !ledState;
        digitalWrite(ledPin, ledState ? HIGH : LOW);
    }

    // Sensor logic
    sensorValue = analogRead(sensorPin);

    // Button logic
    if (digitalRead(buttonPin) == LOW) {
        buttonPressed = true;
        Serial.println("Button pressed!");
    }

    // More scattered logic...
}
```

**Problems**: All state is global, all logic is in `loop()`, no encapsulation, hard to test or reuse.

### After: Device as Application Mediator

```cpp
#include <Arduino.h>
#include <ModestIoT.h>

class MyDevice : public Device {
    Led led;
    SoilMoistureSensor sensor;
    Button button;
public:
    static constexpr int BLINK_EVENT = 300;

    MyDevice(int ledPin, int sensorPin, int buttonPin)
    : Device(500, 0),
    led(ledPin, false, this),
    sensor(sensorPin, nullptr),
    button(buttonPin, 50, nullptr)  // 50ms debounce
    {
        sensor.setHandler(this);
        button.setHandler(this);
        initializeAsynchronousEngine(5);
        appendSensorToScheduler(this, BLINK_EVENT, 500, 0);
        appendSensorToScheduler(&sensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        appendSensorToTriggeringScheduler(&button,
            Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, FALLING);
    }

    void on(Event event) override {
        if (event.identifier == BLINK_EVENT) {
            led.handle(Led::TOGGLE_LED_COMMAND);
        }
        else if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
            // Sensor reading available via sensor.getLatestValue()
        }
        else if (event.identifier == Button::BUTTON_PRESSED_EVENT_IDENTIFIER) {
            Serial.println("Button pressed!");
        }
    }
};

static MyDevice* app;

void setup() {
    Serial.begin(115200);
    app = new MyDevice(2, 4, 14);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(60000));
}
```

**Benefits**:
- All state is encapsulated in `MyDevice` — no globals.
- Each component (LED, sensor, button) is a framework class with clear responsibilities.
- The `on(Event)` handler cleanly separates different event types.
- Easy to test: mock the sensors and actuators, call `on()` directly.

---

## Pattern 5: From Monolithic `loop()` to Separated Concerns

### Before: Everything in `loop()`

```cpp
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "MyNetwork";
const char* password = "MyPassword";
const char* mqtt_server = "192.168.1.100";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

const int sensorPin = 4;
unsigned long lastRead = 0;

void setup() {
    Serial.begin(115200);
    pinMode(sensorPin, INPUT);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {  // Blocks until connected!
        delay(500);
    }

    mqttClient.setServer(mqtt_server, 1883);
    while (!mqttClient.connected()) {        // Blocks until connected!
        mqttClient.connect("device-1");
        delay(500);
    }
}

void loop() {
    // Keep MQTT alive
    if (!mqttClient.connected()) {
        mqttClient.connect("device-1");
    }
    mqttClient.loop();

    // Read sensor and publish
    if (millis() - lastRead >= 2000) {
        lastRead = millis();
        int value = analogRead(sensorPin);
        char buf[50];
        snprintf(buf, sizeof(buf), "{\"value\":%d}", value);
        mqttClient.publish("sensor/data", buf);
    }
}
```

**Problems**:
- `setup()` blocks waiting for Wi-Fi and MQTT — if the network is down, the device never starts.
- `loop()` mixes MQTT keep-alive, sensor reading, and JSON serialization.
- Uses external libraries (`PubSubClient`) that the framework replaces natively.

### After: Framework-Managed Concerns

```cpp
#include <Arduino.h>
#include <ModestIoT.h>

class SensorTelemetryPackage : public TelemetryPackage {
    int32_t value;
public:
    SensorTelemetryPackage(int32_t v) : value(v) {}
    void write(TelemetryWriter& writer) const override {
        writer.field("value", value);
    }
};

class SensorDevice : public Device {
    SoilMoistureSensor sensor;
    MqttGatewayClient& gatewayClient;

    void packageAndQueueTelemetrySnapshot() {
        auto packet = std::make_unique<SensorTelemetryPackage>(
            sensor.getMoistureLevel());
        enqueueTelemetryPayload(std::move(packet));
    }

protected:
    void processQueuedTelemetryData(const TelemetryPackage* payload) const override {
        if (payload != nullptr) {
            gatewayClient.sendTelemetryRecord(*payload);
        }
    }

    void maintainNetworkServices() const override {
        gatewayClient.service();
    }

public:
    SensorDevice(int sensorPin, MqttGatewayClient& client, unsigned long intervalMs)
    : Device(intervalMs, 0), sensor(sensorPin, nullptr), gatewayClient(client)
    {
        sensor.setHandler(this);
        initializeAsynchronousEngine(10);
        appendSensorToScheduler(&sensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    }

    void on(Event event) override {
        if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
            packageAndQueueTelemetrySnapshot();
        }
    }
};

static WiFiConnectivityDriver* wifiDriver;
static MqttGatewayClient* mqttClient;
static SensorDevice* app;

void setup() {
    Serial.begin(115200);

    // Wi-Fi connects asynchronously — setup() doesn't block
    wifiDriver = new WiFiConnectivityDriver("MyNetwork", "MyPassword");

    mqttClient = new MqttGatewayClient(
        *wifiDriver,
        wifiDriver->getNetworkSocket(),
        "192.168.1.100", 1883,
        "sensor/data", "device-1"
    );

    app = new SensorDevice(4, *mqttClient, 2000);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(60000));
}
```

**Benefits**:
- `setup()` returns immediately — Wi-Fi connects in the background.
- MQTT keep-alive is handled by `maintainNetworkServices()` in the network task.
- Telemetry flows through a queue: sensor → package → queue → network task → MQTT.
- No external libraries: `MqttGatewayClient` and `WiFiConnectivityDriver` are framework components.

---

## Migration Checklist

Use this checklist when migrating a sketch:

- [ ] **Remove all `delay()` calls** — replace with hardware timers via `appendSensorToScheduler()`.
- [ ] **Remove all `millis()` timing flags** — the framework scheduler handles all timing.
- [ ] **Encapsulate state in a `Device` subclass** — no more globals.
- [ ] **Replace sensor libraries** with framework sensors (`DhtSensor`, `SoilMoistureSensor`, `PulseSensor`, etc.).
- [ ] **Replace actuator code** with framework actuators (`Led`, `ServoMotor`, `CharacterLcdDisplay`, etc.).
- [ ] **Replace blocking network code** with `WiFiConnectivityDriver` and `MqttGatewayClient`.
- [ ] **Move all logic out of `loop()`** — `loop()` should only contain `vTaskDelay()`.
- [ ] **Handle events in `on(Event)`** — one handler per event type.
- [ ] **Use `TelemetryPackage`** for sensor data sent to the cloud.
- [ ] **Test with Serial Monitor** — verify event flow matches expectations.

---

## Key Concepts Mapping

| Traditional Arduino | Modest IoT Framework |
|---|---|
| `delay(ms)` | Hardware timer + `appendSensorToScheduler()` |
| `millis()` timing flags | `Device` scheduler with event identifiers |
| Global variables | Fields in `Device` subclass |
| `loop()` body | `on(Event)` handler |
| `digitalWrite()` | `Led` actuator with commands |
| `analogRead()` | `SoilMoistureSensor` with scheduled reads |
| DHT library | `DhtSensor` (native implementation) |
| Servo library | `ServoMotor` (native LEDC PWM) |
| LCD I2C library | `CharacterLcdDisplay` (native I²C protocol) |
| `PubSubClient` | `MqttGatewayClient` |
| Blocking Wi-Fi connect | `WiFiConnectivityDriver` (async) |
| Manual JSON serialization | `TelemetryPackage` + `write(TelemetryWriter&)` |

---

## Next Steps

After migrating your sketch:

1. **Review the [Quick Start Guide](./quick-start.md)** for the complete onboarding flow.
2. **Check the [examples](../examples/) directory** for reference implementations of common patterns.
3. **Read the [Troubleshooting Guide](./troubleshooting.md)** if you encounter issues during migration.
4. **Explore the [full README](../README.md)** for architecture overview and component documentation.
