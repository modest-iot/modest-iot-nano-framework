/**
 * @file IntrusionDetector.ino
 * @brief Standard composite event pattern matching using EventAggregator (W31).
 *
 * This example demonstrates the EventAggregator component with **standard patterns**
 * that always emit a composite event when their registered event identifiers occur
 * together within the specified time window. No condition function is used — the
 * pattern match alone is sufficient to trigger the reaction.
 *
 * For conditional patterns that check sensor values before emitting, see the
 * ColdChainMonitor example.
 *
 * It combines a PIR motion sensor, a door/window sensor, and a water leak sensor
 * to define automation rules that fire only when multiple events occur together
 * within a specified time window.
 *
 * @par Composite Patterns Demonstrated:
 * - **Intrusion Alert**: Motion (30) AND door opened (72) within 10 seconds
 *   → composite event 400 triggers the buzzer and LED alert.
 * - **Leak Near Entry**: Water leak (70) AND door opened (72) within 30 seconds
 *   → composite event 401 logs a warning that a door was opened near a leak.
 *
 * The EventAggregator implements `EventHandler`, so sensors dispatch events
 * to it instead of directly to the Device. When a pattern matches, the
 * aggregator emits a composite event to the output handler (the Device),
 * which then reacts to the composite event.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `EventAggregator` for composite event pattern matching (W31)
 * - `addAllMatchPattern()` to register AND-based patterns with time windows
 * - Standard patterns always emit when identifiers match (no condition function)
 * - `setOutputHandler()` to route composite events to the Device
 * - `PassiveInfraredSensor`, `DoorWindowSensor`, `WaterLeakSensor` — all interrupt-driven
 * - `Buzzer` actuator for audible alerts on composite events
 * - `Led` actuator for visual alerts on composite events
 * - Event chaining: sensors → EventAggregator → Device::on(Event)
 * - Zero heap allocation via fixed-size circular buffer (32 events)
 *
 * @par Serial Output Example:
 * @code
 * [IntrusionDetector] Pattern registered: motion + door within 10s → intrusion alert (400)
 * [IntrusionDetector] Pattern registered: leak + door within 30s → leak near entry (401)
 * [IntrusionDetector] Motion detected.
 * [IntrusionDetector] Door opened.
 * [IntrusionDetector] *** INTRUSION ALERT: Motion + door opened within 10s! ***
 * [IntrusionDetector] Buzzer alert triggered.
 * [IntrusionDetector] Alert LED toggled.
 * @endcode
 *
 * @date 2026-11-20
 * @version 1.0
 */

#include <Arduino.h>
#include <ModestIoT.h>

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
/** @brief GPIO pin for the PIR motion sensor. */
static const int PIR_SENSOR_PIN = 18;
/** @brief GPIO pin for the door/window sensor. */
static const int DOOR_WINDOW_SENSOR_PIN = 5;
/** @brief GPIO pin for the water leak sensor. */
static const int WATER_LEAK_SENSOR_PIN = 4;
/** @brief GPIO pin for the buzzer actuator. */
static const int BUZZER_PIN = 25;
/** @brief GPIO pin for the alert LED. */
static const int ALERT_LED_PIN = 2;
//@} // GPIO Pin Assignments

/** @name Device Configuration */
//@{
/** @brief Firmware version string for this device. */
static const char* INTRUSION_DETECTOR_FIRMWARE_VERSION = "ID-100 FW 1.0.0";
//@} // Device Configuration

/** @name Serial Output Configuration */
//@{
/** @brief Serial communication baud rate. */
static const unsigned long SERIAL_BAUD_RATE = 115200;
/** @brief Log prefix for all IntrusionDetector serial output. */
static const char* INTRUSION_DETECTOR_LOG_PREFIX = "[IntrusionDetector]";
/** @brief Delay in the main loop when no work is pending. */
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;
//@} // Serial Output Configuration

/** @name Composite Event Identifiers */
//@{
/** @brief Composite event: intrusion detected (motion + door within time window). */
static const int COMPOSITE_INTRUSION_ALERT = 400;
/** @brief Composite event: leak near entry (leak + door within time window). */
static const int COMPOSITE_LEAK_NEAR_ENTRY = 401;
//@} // Composite Event Identifiers

/** @name Pattern Time Windows */
//@{
/** @brief Maximum seconds between motion and door events for intrusion alert. */
static const uint32_t INTRUSION_TIME_WINDOW_SECONDS = 10;
/** @brief Maximum seconds between leak and door events for leak near entry. */
static const uint32_t LEAK_NEAR_ENTRY_TIME_WINDOW_SECONDS = 30;
//@} // Pattern Time Windows

//@} // Top-Level Declarative Canvas

/** @name Application Mediator Implementation */
//@{

/**
 * @brief Intrusion detector using EventAggregator for standard composite event patterns.
 *
 * Extends Device to handle composite events emitted by the EventAggregator.
 * Sensors dispatch events to the EventAggregator, which matches patterns
 * and emits composite events back to this Device for reaction.
 *
 * This example uses standard patterns that always emit when their event identifiers
 * match within the time window. For conditional patterns with triggering condition
 * functions, see the ColdChainMonitor example.
 */
class IntrusionDetector : public Device {
private:
    PassiveInfraredSensor pirSensor;
    DoorWindowSensor doorWindowSensor;
    WaterLeakSensor waterLeakSensor;
    Buzzer buzzer;
    Led alertLed;

    /// @brief The event aggregator that matches composite patterns.
    EventAggregator aggregator;

    /**
     * @brief Registers the Intrusion Alert standard pattern.
     *
     * Motion detected AND door opened within 10 seconds.
     * Triggers the buzzer and LED alert.
     */
    void setupPatternForIntrusionAlert() {
        int identifiers[] = {
            PassiveInfraredSensor::MOTION_DETECTED_EVENT_IDENTIFIER,
            DoorWindowSensor::DOOR_OPENED_EVENT_IDENTIFIER
        };
        bool ok = aggregator.addAllMatchPattern(
            identifiers, 2, INTRUSION_TIME_WINDOW_SECONDS, COMPOSITE_INTRUSION_ALERT);
        if (ok) {
            Serial.println((String(INTRUSION_DETECTOR_LOG_PREFIX) +
                " Pattern registered: motion + door within " +
                INTRUSION_TIME_WINDOW_SECONDS + "s → intrusion alert (400)").c_str());
        }
    }

    /**
     * @brief Registers the Leak Near Entry standard pattern.
     *
     * Water leak detected AND door opened within 30 seconds.
     * Logs a warning that a door was opened near a leak.
     */
    void setupPatternForLeakNearEntry() {
        int identifiers[] = {
            WaterLeakSensor::WATER_LEAK_DETECTED_EVENT_IDENTIFIER,
            DoorWindowSensor::DOOR_OPENED_EVENT_IDENTIFIER
        };
        bool ok = aggregator.addAllMatchPattern(
            identifiers, 2, LEAK_NEAR_ENTRY_TIME_WINDOW_SECONDS, COMPOSITE_LEAK_NEAR_ENTRY);
        if (ok) {
            Serial.println((String(INTRUSION_DETECTOR_LOG_PREFIX) +
                " Pattern registered: leak + door within " +
                LEAK_NEAR_ENTRY_TIME_WINDOW_SECONDS + "s → leak near entry (401)").c_str());
        }
    }

public:
    IntrusionDetector(int pirPin, int doorPin, int leakPin, int buzzerPin, int ledPin)
        : Device(SAMPLING_INTERVAL_IN_MILLISECONDS, 0)
        , pirSensor(pirPin, &aggregator)   // Events go to aggregator, not directly to Device
        , doorWindowSensor(doorPin, &aggregator)
        , waterLeakSensor(leakPin, &aggregator)
        , buzzer(buzzerPin, this)
        , alertLed(ledPin, false, this)
    {
        // Register all sensors with interrupt-driven operation (CHANGE edge)
        // IMPORTANT: Registration must occur before initializeAsynchronousEngine(), because
        // the engine start guard rejects any registration after the engine is running.
        appendSensorToTriggeringScheduler(
            &pirSensor, PassiveInfraredSensor::MOTION_DETECTED_EVENT_IDENTIFIER, CHANGE);
        appendSensorToTriggeringScheduler(
            &doorWindowSensor, DoorWindowSensor::DOOR_OPENED_EVENT_IDENTIFIER, CHANGE);
        appendSensorToTriggeringScheduler(
            &waterLeakSensor, WaterLeakSensor::WATER_LEAK_DETECTED_EVENT_IDENTIFIER, CHANGE);

        // Configure the EventAggregator to send composite events to this Device
        aggregator.setOutputHandler(this);

        // Register composite event patterns
        setupPatternForIntrusionAlert();
        setupPatternForLeakNearEntry();

        // Launch internal background task workers
        if (!initializeAsynchronousEngine(ASYNC_ENGINE_QUEUE_DEPTH)) {
            return;
        }
    }

    /**
     * @brief Post-construction startup hook.
     */
    bool start() {
        if (!isAsynchronousEngineStarted()) {
            return false;
        }
        Device::setFirmwareVersion(INTRUSION_DETECTOR_FIRMWARE_VERSION);
        return true;
    }

    ~IntrusionDetector() override {
        stopAsynchronousEngine();
        // Remove all registered patterns after stopping the engine.
        aggregator.cleanupPatterns();
    }

    /**
     * @brief Handles composite events from the EventAggregator.
     *
     * The EventAggregator intercepts all sensor events and checks them against
     * registered patterns. When a pattern matches, it emits a composite event
     * to this handler. Individual sensor events are NOT received here because
     * the sensors' parent handler is set to the aggregator, not the Device.
     *
     * @param event The composite event context payload.
     * @runs_on{sampler}
     */
    void on(Event event) override {
        switch (event.identifier) {
            case COMPOSITE_INTRUSION_ALERT:
                Serial.println((String(INTRUSION_DETECTOR_LOG_PREFIX) +
                    " *** INTRUSION ALERT: Motion + door opened within " +
                    INTRUSION_TIME_WINDOW_SECONDS + "s! ***").c_str());
                buzzer.handle(Buzzer::PLAY_TONE_COMMAND);
                alertLed.handle(Led::TOGGLE_LED_COMMAND);
                break;

            case COMPOSITE_LEAK_NEAR_ENTRY:
                Serial.println((String(INTRUSION_DETECTOR_LOG_PREFIX) +
                    " *** LEAK NEAR ENTRY: Water leak + door opened within " +
                    LEAK_NEAR_ENTRY_TIME_WINDOW_SECONDS + "s! ***").c_str());
                buzzer.handle(Buzzer::PLAY_TONE_COMMAND);
                break;

            default:
                // Individual sensor events may also arrive if the aggregator
                // does not consume them; ignore them in this demo.
                break;
        }
    }
};
//@} // Application Mediator Implementation

static IntrusionDetector* intrusionDetectorDevice = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    intrusionDetectorDevice = new IntrusionDetector(
        PIR_SENSOR_PIN,
        DOOR_WINDOW_SENSOR_PIN,
        WATER_LEAK_SENSOR_PIN,
        BUZZER_PIN,
        ALERT_LED_PIN
    );
    if (!intrusionDetectorDevice->start()) {
        Serial.println("[IntrusionDetector] Engine startup failed; monitoring was not started.");
        return;
    }

    Serial.println((String(INTRUSION_DETECTOR_LOG_PREFIX) + " Composite event monitoring active.").c_str());
}

void loop() {
    // All event handling is interrupt-driven; main loop remains idle.
    // vTaskDelay clarifies that this loop yields to FreeRTOS — the device is loopless.
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
