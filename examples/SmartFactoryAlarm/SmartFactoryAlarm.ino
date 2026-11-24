/**
 * @file SmartFactoryAlarm.ino
 * @brief Factory alarm system with automated rules for water leak and door intrusion detection.
 * @version 1.0
 * @date 2026-11-23
 * @copyright (c) 2025-2026 Angel Velasquez
 * @license Apache-2.0
 *
 * Demonstrates the RulesEngine component (W29) for on-device automation.
 * Sensors dispatch events to EventAggregator, which matches composite patterns
 * and forwards events to this Device via setOutputHandler(this). Non-matching
 * individual sensor events are also forwarded by EventAggregator. When a water
 * leak AND door intrusion are detected within 10 seconds, EventAggregator emits
 * a composite event to this Device, which forwards it to RulesEngine to
 * automatically sound the buzzer and activate the emergency ventilation fan.
 *
 * Event flow: sensors → EventAggregator → Device::on(Event) → RulesEngine → actuators
 *
 * Modest IoT Nano Framework aspects illustrated:
 * - EventAggregator: composite event pattern matching (AND logic)
 * - RulesEngine: automated rule-based action dispatch to multiple actuators
 * - EventAggregator forwards non-matching events to output handler (Device)
 * - Device::on(Event) as central event router for composite events
 * - Multiple sensors and actuators with per-slot scheduling
 *
 * @runs_on{sampler} All sensor on() methods
 * @runs_on{network} All gateway client methods
 */

#include <ModestIoT.h>

// --- Serial and timing constants ---
static const uint32_t SERIAL_BAUD_RATE = 115200;
static const uint32_t MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;

// --- Pin assignments ---
static const int WATER_LEAK_PIN = 4;
static const int DOOR_WINDOW_PIN = 5;
static const int BUZZER_PIN = 18;
static const int EMERGENCY_VENTILATION_PIN = 19;

// --- Sampling intervals ---
static const uint32_t WATER_LEAK_INTERVAL_IN_MILLISECONDS = 1000;
static const uint32_t DOOR_WINDOW_INTERVAL_IN_MILLISECONDS = 1000;

// --- MQTT configuration ---
static const char* MQTT_BROKER = "broker.example.com";
static const int MQTT_PORT = 1883;
static const char* MQTT_TOPIC = "factory/alarm";

// --- Rule configuration ---
static const int WATER_AND_DOOR_COMPOSITE_EVENT_ID = 100;
static const uint32_t PATTERN_TIME_WINDOW_SECONDS = 10;

// --- Log prefix ---
static const char* LOG_PREFIX = "SmartFactoryAlarm";

// --- Application instance ---
SmartFactoryAlarm* smartFactoryAlarmApplication = nullptr;

/**
 * @name SmartFactoryAlarm Application
 * @{
 */
class SmartFactoryAlarm : public Device {
private:
    WaterLeakSensor waterLeakSensor;
    DoorWindowSensor doorWindowSensor;
    Buzzer buzzer;
    RelayActuator emergencyVentilationController;
    MqttGatewayClient mqttClient;
    EventAggregator eventAggregator;
    RulesEngine rulesEngine;

public:
    explicit SmartFactoryAlarm(int waterLeakPin, int doorWindowPin, int buzzerPin, int emergencyVentilationPin)
        : waterLeakSensor(waterLeakPin, &eventAggregator)
        , doorWindowSensor(doorWindowPin, &eventAggregator)
        , buzzer(buzzerPin)
        , emergencyVentilationController(emergencyVentilationPin)
        , mqttClient(MQTT_BROKER, MQTT_PORT)
        , eventAggregator()
        , rulesEngine()
    {
        setFirmwareVersion("SFA-100 FW 1.0.0");
    }

    void start() override {
        Serial.begin(SERIAL_BAUD_RATE);
        Serial.println(LOG_PREFIX " starting...");

        // Configure engine
        EngineConfiguration engineConfiguration;
        engineConfiguration.samplerStackFreeWords = 4096;
        engineConfiguration.networkStackFreeWords = 8192;
        startEngine(engineConfiguration);

        // Register sensors with per-slot scheduling
        appendSensorToScheduler(&waterLeakSensor, WATER_LEAK_INTERVAL_IN_MILLISECONDS);
        appendSensorToScheduler(&doorWindowSensor, DOOR_WINDOW_INTERVAL_IN_MILLISECONDS);

        // Set up MQTT gateway
        setTelemetryGateway(&mqttClient);
        mqttClient.setServiceEndpointUrl(MQTT_TOPIC);

        // --- EventAggregator: composite event pattern ---
        // When water leak AND door opened within 10 seconds, emit composite event 100
        int eventIds[] = {
            WaterLeakSensor::WATER_LEAK_DETECTED_EVENT_IDENTIFIER,
            DoorWindowSensor::DOOR_OPENED_EVENT_IDENTIFIER
        };
        eventAggregator.addAllMatchPattern(
            eventIds, 2, PATTERN_TIME_WINDOW_SECONDS, WATER_AND_DOOR_COMPOSITE_EVENT_ID);

        // --- RulesEngine: automated actions ---
        // When composite event 100 occurs, sound buzzer AND activate emergency
        // ventilation fan. Using RulesEngine instead of a manual on(Event) switch
        // statement: one rule can dispatch multiple Commands to different actuators,
        // keeping the event handler clean and declarative.
        rulesEngine.addRule(WATER_AND_DOOR_COMPOSITE_EVENT_ID, &buzzer,
                           Buzzer::PLAY_TONE_COMMAND_IDENTIFIER, 2000);
        rulesEngine.addActionToRule(WATER_AND_DOOR_COMPOSITE_EVENT_ID,
                                    &emergencyVentilationController,
                                    RelayActuator::RELAY_ON_COMMAND_IDENTIFIER);

        // Wire EventAggregator to Device (this) as output handler.
        // Sensors dispatch to EventAggregator (see constructor). EventAggregator
        // matches patterns and emits composite events to this Device.
        // Non-matching individual sensor events are also forwarded to this Device.
        eventAggregator.setOutputHandler(this);

        Serial.println(LOG_PREFIX " ready. Monitoring for water leak + door intrusion.");
    }

    /**
     * @brief Handles events from EventAggregator — composite and individual.
     *
     * Sensors dispatch to EventAggregator (not this Device). EventAggregator
     * matches patterns and forwards all events to this Device via
     * setOutputHandler(this): both matched composite events and unmatched
     * individual sensor events. Composite events go to RulesEngine; individual
     * sensor events are logged for monitoring.
     *
     * @param event The triggered event context payload.
     * @runs_on{sampler}
     */
    void on(Event event) override {
        switch (event.identifier) {
            case WATER_AND_DOOR_COMPOSITE_EVENT_ID:
                // Composite event from EventAggregator: forward to RulesEngine
                rulesEngine.on(event);
                break;

            case WaterLeakSensor::WATER_LEAK_DETECTED_EVENT_IDENTIFIER:
                Serial.println(LOG_PREFIX " WARNING: Water leak detected!");
                break;

            case WaterLeakSensor::WATER_LEAK_CLEARED_EVENT_IDENTIFIER:
                Serial.println(LOG_PREFIX " Water leak cleared.");
                break;

            case DoorWindowSensor::DOOR_OPENED_EVENT_IDENTIFIER:
                Serial.println(LOG_PREFIX " NOTICE: Door opened.");
                break;

            case DoorWindowSensor::DOOR_CLOSED_EVENT_IDENTIFIER:
                Serial.println(LOG_PREFIX " Door closed.");
                break;

            default:
                break;
        }
    }

    void handle(const Command command) override {
        // No custom commands needed — RulesEngine handles all automation.
        (void)command;
    }

};

/** @} */

void setup() {
    smartFactoryAlarmApplication = new SmartFactoryAlarm(WATER_LEAK_PIN, DOOR_WINDOW_PIN, BUZZER_PIN, EMERGENCY_VENTILATION_PIN);
    smartFactoryAlarmApplication->start();
}

void loop() {
    delay(MAIN_LOOP_DELAY_IN_MILLISECONDS);
}
