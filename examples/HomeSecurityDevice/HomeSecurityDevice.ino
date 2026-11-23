/**
 * @file HomeSecurityDevice.ino
 * @brief Home security monitoring system with high-priority alert telemetry.
 *
 * Combines a water leak sensor, door/window sensor, and PIR motion sensor to monitor
 * a home's perimeter and interior. All sensors use interrupt-driven operation for
 * zero-latency detection, a buzzer provides audible alerts, and critical events are
 * published via MQTT using the high-priority telemetry lane.
 *
 * This example demonstrates the high-priority telemetry lane (W32):
 * - Routine status updates (door closed, motion stopped, leak cleared) use
 *   `enqueueTelemetryPayload()` → standard queue → normal priority
 * - Critical alerts (water leak, door opened) use `enqueueHighPriorityPayload()`
 *   → high-priority queue → `"priority": "high"` envelope field
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `Device` base class with async engine for loop-less execution
 * - `WaterLeakSensor`, `DoorWindowSensor`, `PassiveInfraredSensor` — all interrupt-driven
 * - `Buzzer` actuator with `PLAY_TONE_COMMAND` for audible alerts
 * - `MqttGatewayClient` for cloud telemetry publishing
 * - `TelemetryPackage` subclasses for routine and high-priority telemetry
 * - `enqueueTelemetryPayload()` for routine status updates (standard queue)
 * - `enqueueHighPriorityPayload()` for critical alerts (high-priority queue)
 * - `markAsHighPriority()` adds `"priority": "high"` to the JSON envelope
 * - `appendSensorToTriggeringScheduler()` with `CHANGE` edge for all sensors
 * - Event routing via `on(Event)` with `switch` on multiple event identifiers
 * @date 2026-11-18
 * @version 1.1
 */

#include <Arduino.h>
#include <memory>
#include "ModestIoT.h"

/** @name Top-Level Declarative Canvas */
//@{

/** @name Engine Configuration */
//@{
/** @brief Sampling interval for security sensors (ms). */
static const unsigned long SAMPLING_INTERVAL_IN_MILLISECONDS = 200;
/** @brief Sampler event queue length. */
static const int ASYNC_ENGINE_QUEUE_DEPTH = 5;
//@} // Engine Configuration

/** @name GPIO Pin Assignments */
//@{
/** @brief GPIO pin for the water leak sensor. */
static const int WATER_LEAK_SENSOR_PIN = 4;
/** @brief GPIO pin for the door/window sensor. */
static const int DOOR_WINDOW_SENSOR_PIN = 5;
/** @brief GPIO pin for the PIR motion sensor. */
static const int PIR_SENSOR_PIN = 18;
/** @brief GPIO pin for the buzzer actuator. */
static const int BUZZER_PIN = 25;
//@} // GPIO Pin Assignments

/** @name Network Configuration */
//@{
/** @brief Wi-Fi SSID for the home network. */
static const char* WIRELESS_SSID = "HomeSecurity_Network";
/** @brief Wi-Fi password for the home network. */
static const char* WIRELESS_PASSWORD = "SecureHomePass2026";
/** @brief MQTT broker IP address. */
static const char* MQTT_BROKER_IP = "192.168.1.200";
/** @brief MQTT broker port. */
static const uint16_t MQTT_BROKER_PORT = 1883;
/** @brief MQTT topic for publishing security alerts. */
static const char* MQTT_PUBLISH_TOPIC = "security/home/alerts";
//@} // Network Configuration

/** @name Device Configuration */
//@{
/** @brief Firmware version string for this device. */
static const char* HOME_SECURITY_FIRMWARE_VERSION = "HSD-700 FW 1.1.0";
/** @brief MQTT client identifier for this device. */
static const char* MQTT_CLIENT_ID = "HomeSecurity_Node_01";
//@} // Device Configuration

/** @name Serial Output Configuration */
//@{
/** @brief Serial communication baud rate. */
static const unsigned long SERIAL_BAUD_RATE = 115200;
/** @brief Log prefix for all HomeSecurityDevice serial output. */
static const char* HOME_SECURITY_LOG_PREFIX = "[HomeSecurity]";
/** @brief Delay in the main loop when no work is pending. */
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;
//@} // Serial Output Configuration

/** @name Alert Type Identifiers */
//@{
/** @brief Alert type string for water leak events. */
static const char* ALERT_TYPE_WATER_LEAK = "water_leak";
/** @brief Alert type string for door/window opened events. */
static const char* ALERT_TYPE_DOOR_OPENED = "door_opened";
//@} // Alert Type Identifiers
//@} // Top-Level Declarative Canvas

/** @name Telemetry JSON Keys */
//@{
/** @brief JSON key for the alert type (e.g., "water_leak", "door_opened"). */
#define JSON_KEY_ALERT_TYPE "alertType"
/** @brief JSON key for the door/window state (true = open). */
#define JSON_KEY_DOOR_OPEN "doorOpen"
/** @brief JSON key for the motion state (true = motion detected). */
#define JSON_KEY_MOTION_DETECTED "motionDetected"
/** @brief JSON key for the water leak state (true = leak detected). */
#define JSON_KEY_WATER_LEAK "waterLeak"
//@} // Telemetry JSON Keys

/** @name Concrete Application Data Schema Specification */
//@{

/**
 * @brief Routine security status telemetry (standard priority).
 *
 * Used for non-critical status updates: door closed, motion stopped, leak cleared.
 * These are enqueued via `enqueueTelemetryPayload()` and processed after any
 * pending high-priority alerts.
 */
class SecurityStatusTelemetryPackage : public TelemetryPackage {
public:
    /** @brief Schema version for this telemetry format. */
    static constexpr const char* SCHEMA_VERSION = "1.0";

private:
    bool doorOpen;
    bool motionDetected;
    bool waterLeak;

public:
    SecurityStatusTelemetryPackage(bool door, bool motion, bool leak)
        : doorOpen(door),
        motionDetected(motion),
        waterLeak(leak)
    {
        schemaVersionValue = SCHEMA_VERSION;
    }

    ~SecurityStatusTelemetryPackage() override = default;

    void write(TelemetryWriter& writer) const override {
        writer.field(JSON_KEY_DOOR_OPEN, this->doorOpen);
        writer.field(JSON_KEY_MOTION_DETECTED, this->motionDetected);
        writer.field(JSON_KEY_WATER_LEAK, this->waterLeak);
    }
};

/**
 * @brief Critical security alert telemetry (high priority).
 *
 * Used for time-sensitive alerts: water leak detected, door/window opened.
 * These are enqueued via `enqueueHighPriorityPayload()` and processed before
 * any pending routine telemetry. The envelope includes `"priority": "high"`.
 */
class SecurityAlertTelemetryPackage : public TelemetryPackage {
public:
    /** @brief Schema version for this telemetry format. */
    static constexpr const char* SCHEMA_VERSION = "1.0";

private:
    const char* alertType;
    bool alertState;

public:
    SecurityAlertTelemetryPackage(const char* type, bool state)
        : alertType(type),
        alertState(state)
    {
        schemaVersionValue = SCHEMA_VERSION;
    }

    ~SecurityAlertTelemetryPackage() override = default;

    void write(TelemetryWriter& writer) const override {
        writer.field(JSON_KEY_ALERT_TYPE, this->alertType);
        writer.field(JSON_KEY_WATER_LEAK, this->alertState);
    }
};
//@} // Concrete Application Data Schema Specification

/// @brief Wi-Fi transport driver instance (used by HomeSecurityDevice::start()).
static WiFiConnectivityDriver* transportDriver = nullptr;

/** @name Application Mediator Implementation */
//@{

class HomeSecurityDevice : public Device {
private:
    WaterLeakSensor waterLeakSensor;
    DoorWindowSensor doorWindowSensor;
    PassiveInfraredSensor pirSensor;
    Buzzer buzzer;
    MqttGatewayClient* gatewayClient = nullptr;

    /// @brief Current door/window state (cached for telemetry).
    bool doorOpenState = false;
    /// @brief Current motion state (cached for telemetry).
    bool motionDetectedState = false;
    /// @brief Current water leak state (cached for telemetry).
    bool waterLeakState = false;

    /**
     * @brief Enqueues a routine status update via the standard telemetry queue.
     */
    void enqueueStatusUpdate() {
        auto packet = std::make_unique<SecurityStatusTelemetryPackage>(
            doorOpenState, motionDetectedState, waterLeakState);
        enqueueTelemetryPayload(std::move(packet));
    }

    /**
     * @brief Enqueues a critical alert via the high-priority telemetry queue.
     * @param alertType The type of alert (e.g., "water_leak", "door_opened").
     * @param alertState The state of the alert (true = active).
     */
    void enqueueAlert(const char* alertType, bool alertState) {
        auto packet = std::make_unique<SecurityAlertTelemetryPackage>(alertType, alertState);
        enqueueHighPriorityPayload(std::move(packet));
    }

protected:
    void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const override {
        if (rawQueueItemPayload != nullptr && gatewayClient != nullptr) {
            gatewayClient->sendTelemetryRecord(*rawQueueItemPayload);
        }
    }

    void maintainNetworkServices() const override {
        if (gatewayClient != nullptr) {
            gatewayClient->service();
        }
    }

public:
    HomeSecurityDevice(int waterLeakPin, int doorPin, int pirPin, int buzzerPin)
        : Device(SAMPLING_INTERVAL_IN_MILLISECONDS, 0)
        , waterLeakSensor(waterLeakPin, this)
        , doorWindowSensor(doorPin, this)
        , pirSensor(pirPin, this)
        , buzzer(buzzerPin, this)
    {
        // Register all sensors with interrupt-driven operation (CHANGE edge)
        // IMPORTANT: Registration must occur before initializeAsynchronousEngine(), because
        // the engine start guard rejects any registration after the engine is running.
        appendSensorToTriggeringScheduler(
            &waterLeakSensor, WaterLeakSensor::WATER_LEAK_DETECTED_EVENT_IDENTIFIER, CHANGE);
        appendSensorToTriggeringScheduler(
            &doorWindowSensor, DoorWindowSensor::DOOR_OPENED_EVENT_IDENTIFIER, CHANGE);
        appendSensorToTriggeringScheduler(
            &pirSensor, PassiveInfraredSensor::MOTION_DETECTED_EVENT_IDENTIFIER, CHANGE);

        // Launch internal background task workers
        initializeAsynchronousEngine(ASYNC_ENGINE_QUEUE_DEPTH);
    }

    /**
     * @brief Post-construction startup hook.
     *
     * Handles all initialization that cannot run in the constructor:
     * firmware version, serial logging, Wi-Fi transport, and MQTT broker
     * setup. Called from `setup()` after construction.
     */
    void start() {
        Device::setFirmwareVersion(HOME_SECURITY_FIRMWARE_VERSION);
        Serial.println((String(HOME_SECURITY_LOG_PREFIX) + " Initializing home security system...").c_str());

        transportDriver = new WiFiConnectivityDriver(WIRELESS_SSID, WIRELESS_PASSWORD);

        this->gatewayClient = new MqttGatewayClient(
            *transportDriver,
            transportDriver->getNetworkSocket(),
            MQTT_BROKER_IP,
            MQTT_BROKER_PORT,
            MQTT_PUBLISH_TOPIC,
            MQTT_CLIENT_ID
        );
    }

    ~HomeSecurityDevice() override {
        stopAsynchronousEngine();
    }

    /**
     * @brief Handles security sensor events and routes to appropriate telemetry queue.
     *
     * Critical alerts (water leak, door opened) use the high-priority lane:
     *   `enqueueHighPriorityPayload()` → high-priority queue → `"priority": "high"`
     *
     * Routine status updates (door closed, motion stopped, leak cleared) use
     * the standard lane:
     *   `enqueueTelemetryPayload()` → standard queue → normal priority
     *
     * @param event The triggered event context payload.
     * @runs_on{sampler}
     */
    void on(Event event) override {
        switch (event.identifier) {
            case WaterLeakSensor::WATER_LEAK_DETECTED_EVENT_IDENTIFIER:
                waterLeakState = true;
                Serial.println((String(HOME_SECURITY_LOG_PREFIX) + " ⚠️  WATER LEAK DETECTED!").c_str());
                buzzer.handle(Buzzer::PLAY_TONE_COMMAND);
                // CRITICAL: Water leak bypasses standard queue via high-priority lane
                enqueueAlert(ALERT_TYPE_WATER_LEAK, true);
                break;
            case WaterLeakSensor::WATER_LEAK_CLEARED_EVENT_IDENTIFIER:
                waterLeakState = false;
                Serial.println((String(HOME_SECURITY_LOG_PREFIX) + " ✓ Water leak cleared.").c_str());
                // ROUTINE: Leak cleared is a status update, not an alert
                enqueueStatusUpdate();
                break;
            case DoorWindowSensor::DOOR_OPENED_EVENT_IDENTIFIER:
                doorOpenState = true;
                Serial.println((String(HOME_SECURITY_LOG_PREFIX) + " 🚪 Door/window opened!").c_str());
                buzzer.handle(Buzzer::PLAY_TONE_COMMAND);
                // CRITICAL: Door opened bypasses standard queue via high-priority lane
                enqueueAlert(ALERT_TYPE_DOOR_OPENED, true);
                break;
            case DoorWindowSensor::DOOR_CLOSED_EVENT_IDENTIFIER:
                doorOpenState = false;
                Serial.println((String(HOME_SECURITY_LOG_PREFIX) + " 🚪 Door/window closed.").c_str());
                // ROUTINE: Door closed is a status update, not an alert
                enqueueStatusUpdate();
                break;
            case PassiveInfraredSensor::MOTION_DETECTED_EVENT_IDENTIFIER:
                motionDetectedState = true;
                Serial.println((String(HOME_SECURITY_LOG_PREFIX) + " 🏃 Motion detected!").c_str());
                buzzer.handle(Buzzer::PLAY_TONE_COMMAND);
                // ROUTINE: Motion detected is a status update (not critical)
                enqueueStatusUpdate();
                break;
            case PassiveInfraredSensor::MOTION_STOPPED_EVENT_IDENTIFIER:
                motionDetectedState = false;
                Serial.println((String(HOME_SECURITY_LOG_PREFIX) + " Motion stopped.").c_str());
                // ROUTINE: Motion stopped is a status update
                enqueueStatusUpdate();
                break;
            default:
                break;
        }
    }
};
//@} // Application Mediator Implementation

static HomeSecurityDevice* homeSecurityApplication = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    homeSecurityApplication = new HomeSecurityDevice(
        WATER_LEAK_SENSOR_PIN,
        DOOR_WINDOW_SENSOR_PIN,
        PIR_SENSOR_PIN,
        BUZZER_PIN
    );
    homeSecurityApplication->start();

    Serial.println((String(HOME_SECURITY_LOG_PREFIX) + " System armed. Monitoring...").c_str());
}

void loop() {
    // All event handling is interrupt-driven; main loop remains idle.
    // vTaskDelay clarifies that this loop yields to FreeRTOS — the device is loopless.
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
