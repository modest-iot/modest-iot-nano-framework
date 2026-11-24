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

#include <Arduino.h>
#include <ModestIoT.h>

// --- Serial and timing constants ---
static const uint32_t SERIAL_BAUD_RATE = 115200;
static const uint32_t MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;

// --- Sampling intervals ---
static const uint32_t SAMPLING_INTERVAL_IN_MILLISECONDS = 1000;

// --- Pin assignments ---
static const int WATER_LEAK_PIN = 4;
static const int DOOR_WINDOW_PIN = 5;
static const int BUZZER_PIN = 18;
static const int EMERGENCY_VENTILATION_PIN = 19;

// --- MQTT configuration ---
static const char* MQTT_BROKER = "broker.example.com";
static const int MQTT_PORT = 1883;
static const char* MQTT_TOPIC = "factory/alarm";

// --- Rule configuration ---
static const int WATER_AND_DOOR_COMPOSITE_EVENT_ID = 100;
static const uint32_t PATTERN_TIME_WINDOW_SECONDS = 10;

// --- Log prefix ---
static const char* LOG_PREFIX = "SmartFactoryAlarm";

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
    EventAggregator eventAggregator;
    RulesEngine rulesEngine;

public:
    explicit SmartFactoryAlarm(int waterLeakPin, int doorWindowPin, int buzzerPin,
                               int emergencyVentilationPin)
        : Device(SAMPLING_INTERVAL_IN_MILLISECONDS)
        , waterLeakSensor(waterLeakPin, &eventAggregator)
        , doorWindowSensor(doorWindowPin, &eventAggregator)
        , buzzer(buzzerPin)
        , emergencyVentilationController(emergencyVentilationPin)
        , eventAggregator()
        , rulesEngine()
    {
        // Register sensors with the triggering scheduler before starting the engine
        appendSensorToTriggeringScheduler(
            &waterLeakSensor, WaterLeakSensor::WATER_LEAK_DETECTED_EVENT_IDENTIFIER, CHANGE);
        appendSensorToTriggeringScheduler(
            &doorWindowSensor, DoorWindowSensor::DOOR_OPENED_EVENT_IDENTIFIER, CHANGE);

        // Wire EventAggregator to Device (this) as output handler
        eventAggregator.setOutputHandler(this);

        // Launch internal background task workers
        initializeAsynchronousEngine();
    }

    void start() {
        Serial.begin(SERIAL_BAUD_RATE);
        Serial.println((String(LOG_PREFIX) + " starting...").c_str());

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

        Serial.println((String(LOG_PREFIX) + " ready. Monitoring for water leak + door intrusion.").c_str());
    }

    ~SmartFactoryAlarm() override {
        rulesEngine.cleanupRules();
        eventAggregator.cleanupPatterns();
        stopAsynchronousEngine();
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
                Serial.println((String(LOG_PREFIX) + " WARNING: Water leak detected!").c_str());
                break;

            case WaterLeakSensor::WATER_LEAK_CLEARED_EVENT_IDENTIFIER:
                Serial.println((String(LOG_PREFIX) + " Water leak cleared.").c_str());
                break;

            case DoorWindowSensor::DOOR_OPENED_EVENT_IDENTIFIER:
                Serial.println((String(LOG_PREFIX) + " NOTICE: Door opened.").c_str());
                break;

            case DoorWindowSensor::DOOR_CLOSED_EVENT_IDENTIFIER:
                Serial.println((String(LOG_PREFIX) + " Door closed.").c_str());
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

static SmartFactoryAlarm* smartFactoryAlarmApplication = nullptr;

void setup() {
    smartFactoryAlarmApplication = new SmartFactoryAlarm(
        WATER_LEAK_PIN, DOOR_WINDOW_PIN, BUZZER_PIN, EMERGENCY_VENTILATION_PIN);
    smartFactoryAlarmApplication->start();
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
