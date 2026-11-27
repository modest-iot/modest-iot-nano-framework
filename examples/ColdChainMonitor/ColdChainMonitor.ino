/**
 * @file ColdChainMonitor.ino
 * @brief Refrigerated cargo bay monitoring with conditional composite event patterns (W31).
 *
 * This example demonstrates the EventAggregator component in an Industrial IoT (IIoT)
 * context: a refrigerated truck cargo bay monitoring system that uses conditional
 * composite event patterns to manage energy and prevent cargo loss.
 *
 * Unlike the IntrusionDetector example, which uses standard patterns that always emit
 * when their event identifiers match, this example focuses on **conditional patterns**
 * where a triggering condition function determines whether the composite event is
 * actually emitted based on sensor values at the moment of the match.
 *
 * @par Composite Patterns Demonstrated:
 * - **Energy Save (Conditional)**: Door opened (72) AND weight change detected (20)
 *   within 30 seconds, **only if** the cargo bay is empty → composite event 500
 *   triggers relay off, saving cooler energy. The condition function checks the
 *   LoadCellAmplifier weight to suppress the event when cargo is present.
 * - **Unauthorized Access**: Door opened (72) AND motion stopped (31) within 60 seconds
 *   → composite event 501 triggers buzzer alarm when the door is left open with cargo
 *   present but no one actively loading or unloading.
 * - **Loading/Unloading Active**: Weight change detected (20) AND door opened (72) AND
 *   motion detected (30) within 120 seconds → composite event 502 logs that cargo is
 *   actively being loaded or unloaded from the truck, keeping the cooler running.
 *
 * The EventAggregator implements `EventHandler`, so sensors dispatch events to it
 * instead of directly to the Device. When a pattern matches, the aggregator checks
 * the optional condition function before emitting the composite event to the output
 * handler (the Device).
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `EventAggregator` for composite event pattern matching (W31)
 * - `addAllMatchPattern()` with condition functions for value-based pattern filtering
 * - `PatternCondition` callback type for triggering conditions
 * - Source-aware patterns distinguish sensors sharing the same event identifier
 *   (e.g., DATA_READ from LoadCellAmplifier vs. DhtSensor)
 * - `setOutputHandler()` to route composite events to the Device
 * - `DoorWindowSensor` for cargo bay door state (interrupt-driven)
 * - `PassiveInfraredSensor` for motion detection inside cargo bay (interrupt-driven)
 * - `LoadCellAmplifier` for cargo weight detection (hybrid scheduling)
 * - `DhtSensor` for temperature monitoring (sampling-based)
 * - `RelayActuator` for cooler/compressor control
 * - `Buzzer` for audible alerts
 * - `Led` for visual status indication
 * - Event chaining: sensors → EventAggregator → Device::on(Event)
 * - Zero heap allocation via fixed-size circular buffer (32 events)
 *
 * @par Serial Output Example:
 * @code
 * [ColdChainMonitor] Pattern registered: door + weight (source 4) within 30s → energy save (500) [conditional]
 * [ColdChainMonitor] Pattern registered: door + no motion within 60s → unauthorized access (501)
 * [ColdChainMonitor] Pattern registered: weight (source 4) + door + motion within 120s → loading active (502)
 * [ColdChainMonitor] *** LOADING/UNLOADING ACTIVE: Cargo operation detected, cooler remains ON. ***
 * [ColdChainMonitor] *** ENERGY SAVE: Door open, bay empty — cooler relay OFF. ***
 * [ColdChainMonitor] Door opened.
 * [ColdChainMonitor] No motion detected while door is open with cargo present.
 * [ColdChainMonitor] *** UNAUTHORIZED ACCESS: Door left open, no activity! ***
 * [ColdChainMonitor] Buzzer alarm triggered.
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
/** @brief Sampling interval for periodic sensors (ms). */
static const unsigned long SAMPLING_INTERVAL_IN_MILLISECONDS = 200;
/** @brief Sampler event queue length. */
static const int ASYNC_ENGINE_QUEUE_DEPTH = 5;
//@} // Engine Configuration

/** @name GPIO Pin Assignments */
//@{
/** @brief GPIO pin for the cargo bay door sensor. */
static const int DOOR_SENSOR_PIN = 5;
/** @brief GPIO pin for the PIR motion sensor inside the cargo bay. */
static const int MOTION_SENSOR_PIN = 18;
/** @brief GPIO pin for the load cell data line (HX711 DOUT). */
static const int LOAD_CELL_DATA_PIN = 4;
/** @brief GPIO pin for the load cell clock line (HX711 PD_SCK). */
static const int LOAD_CELL_CLOCK_PIN = 15;
/** @brief GPIO pin for the DHT temperature/humidity sensor. */
static const int DHT_SENSOR_PIN = 17;
/** @brief GPIO pin for the cooler relay actuator. */
static const int COOLER_RELAY_PIN = 25;
/** @brief GPIO pin for the buzzer. */
static const int BUZZER_PIN = 26;
/** @brief GPIO pin for the status LED. */
static const int STATUS_LED_PIN = 2;
//@} // GPIO Pin Assignments

/** @name Device Configuration */
//@{
/** @brief Firmware version string for this device. */
static const char* COLD_CHAIN_MONITOR_FIRMWARE_VERSION = "CCM-200 FW 1.0.0";
//@} // Device Configuration

/** @name Serial Output Configuration */
//@{
/** @brief Serial communication baud rate. */
static const unsigned long SERIAL_BAUD_RATE = 115200;
/** @brief Log prefix for all ColdChainMonitor serial output. */
static const char* COLD_CHAIN_MONITOR_LOG_PREFIX = "[ColdChainMonitor]";
/** @brief Delay in the main loop when no work is pending. */
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;
//@} // Serial Output Configuration

/** @name Composite Event Identifiers */
//@{
/** @brief Composite event: energy save triggered (door open, bay empty). */
static const int COMPOSITE_ENERGY_SAVE = 500;
/** @brief Composite event: unauthorized access detected (door open, no activity). */
static const int COMPOSITE_UNAUTHORIZED_ACCESS = 501;
/** @brief Composite event: loading or unloading operation actively in progress. */
static const int COMPOSITE_LOADING_UNLOADING = 502;
//@} // Composite Event Identifiers

/** @name Pattern Time Windows */
//@{
/** @brief Maximum seconds between door open and weight change for energy save. */
static const uint32_t ENERGY_SAVE_TIME_WINDOW_SECONDS = 30;
/** @brief Maximum seconds between door open and motion stop for unauthorized access. */
static const uint32_t UNAUTHORIZED_ACCESS_TIME_WINDOW_SECONDS = 60;
/** @brief Maximum seconds for all three loading/unloading events to occur together. */
static const uint32_t LOADING_UNLOADING_TIME_WINDOW_SECONDS = 120;
//@} // Pattern Time Windows

/** @name Load Cell Calibration */
//@{
/** @brief Minimum weight for load cell calibration (grams). */
static const float LOAD_CELL_MIN_WEIGHT_IN_GRAMS = 0.0f;
/** @brief Maximum weight for load cell calibration (grams). */
static const float LOAD_CELL_MAX_WEIGHT_IN_GRAMS = 500000.0f;
/** @brief Minimum raw ADC value for load cell calibration. */
static const long LOAD_CELL_MIN_RAW_VALUE = 0;
/** @brief Maximum raw ADC value for load cell calibration. */
static const long LOAD_CELL_MAX_RAW_VALUE = 1000000;
/** @brief Moving average filter depth for the load cell. */
static const int LOAD_CELL_FILTER_DEPTH = 5;
/** @brief Weight threshold below which the cargo bay is considered empty (grams). */
static const float CARGO_EMPTY_THRESHOLD_IN_GRAMS = 100.0f;
//@} // Load Cell Calibration

//@} // Top-Level Declarative Canvas

/** @name Application Mediator Implementation */
//@{

/// @brief Pointer to the load cell instance for use in the static condition function.
static LoadCellAmplifier* coldChainLoadCell;

/**
 * @brief Condition function for the Energy Save pattern.
 *
 * Called by the EventAggregator after the door + weight change pattern matches.
 * Returns true only if the cargo bay is empty (weight below threshold),
 * suppressing the composite event when cargo is present.
 *
 * The compositeEventId parameter identifies which pattern matched (500 for
 * Energy Save), allowing a single condition function to handle multiple patterns.
 *
 * This demonstrates the core value of conditional patterns: the EventAggregator
 * matches the event identifiers, but the condition function checks actual sensor
 * values to decide whether the composite event should be emitted.
 *
 * @param compositeEventId The composite event identifier (500 for Energy Save).
 * @return True if the cargo bay is empty and the cooler relay should be turned off.
 * @runs_on{sampler}
 */
static bool isCargoBayEmptyCondition(int compositeEventId) {
    (void)compositeEventId;  // Energy Save is the only conditional pattern in this example.
    return coldChainLoadCell &&
           coldChainLoadCell->getWeightInGrams() < CARGO_EMPTY_THRESHOLD_IN_GRAMS;
}

/**
 * @brief Cold chain monitoring device using EventAggregator for conditional composite patterns.
 *
 * Extends Device to handle composite events emitted by the EventAggregator.
 * Sensors dispatch events to the EventAggregator, which matches patterns
 * and emits composite events back to this Device for reaction.
 *
 * This example focuses on **conditional patterns** where a triggering condition
 * function determines whether the composite event is emitted based on sensor
 * values at the moment of the match. See IntrusionDetector for standard patterns.
 *
 * Three composite patterns are demonstrated:
 * 1. Energy Save (Conditional): Door opened AND weight change detected within 30 seconds.
 *    The condition function checks if the cargo bay is empty. Only when empty,
 *    the composite event is emitted and the cooler relay is turned off.
 * 2. Unauthorized Access: Door opened AND motion stopped within 60 seconds.
 *    When the door is open with cargo present but no one is actively loading
 *    or unloading, a buzzer alarm alerts nearby personnel.
 * 3. Loading/Unloading Active: Weight change detected AND door opened AND motion
 *    detected within 120 seconds. When all three conditions occur together, it
 *    indicates cargo is actively being loaded or unloaded, so the cooler remains
 *    running and the operation is logged.
 */
class ColdChainMonitor : public Device {
private:
    DoorWindowSensor doorSensor;
    PassiveInfraredSensor motionSensor;
    LoadCellAmplifier loadCell;
    DhtSensor tempSensor;
    RelayActuator coolerRelay;
    Buzzer buzzer;
    Led statusLed;

    /// @brief The event aggregator that matches composite patterns.
    EventAggregator aggregator;

    /// @brief Tracks whether the door is currently open.
    bool doorOpen;

    /**
     * @brief Registers the Energy Save conditional pattern.
     *
     * Door opened AND weight change detected within 30 seconds,
     * only if the cargo bay is empty. The condition function checks
     * the LoadCellAmplifier weight to suppress the event when cargo is present.
     */
    void setupPatternForEnergySaveUnderCondition() {
        int identifiers[] = {
            DoorWindowSensor::DOOR_OPENED_EVENT_IDENTIFIER,
            Sensor::DATA_READ_EVENT_IDENTIFIER
        };
        int sources[] = {
            EventAggregator::ANY_SOURCE,  // door: any source
            LOAD_CELL_DATA_PIN  // DATA_READ: only from LoadCellAmplifier
        };
        bool ok = aggregator.addAllMatchPattern(
            identifiers, 2, sources, ENERGY_SAVE_TIME_WINDOW_SECONDS, COMPOSITE_ENERGY_SAVE,
            isCargoBayEmptyCondition);
        if (ok) {
            Serial.println((String(COLD_CHAIN_MONITOR_LOG_PREFIX) +
                " Pattern registered: door + weight (source " + LOAD_CELL_DATA_PIN +
                ") within " + ENERGY_SAVE_TIME_WINDOW_SECONDS +
                "s → energy save (500) [conditional]")
                .c_str());
        }
    }

    /**
     * @brief Registers the Unauthorized Access standard pattern.
     *
     * Door opened AND motion stopped within 60 seconds.
     * When the door is open with cargo present but no one is active, sound alarm.
     */
    void setupPatternForUnauthorizedAccess() {
        int identifiers[] = {
            DoorWindowSensor::DOOR_OPENED_EVENT_IDENTIFIER,
            PassiveInfraredSensor::MOTION_STOPPED_EVENT_IDENTIFIER
        };
        bool ok = aggregator.addAllMatchPattern(
            identifiers, 2, UNAUTHORIZED_ACCESS_TIME_WINDOW_SECONDS,
            COMPOSITE_UNAUTHORIZED_ACCESS);
        if (ok) {
            Serial.println((String(COLD_CHAIN_MONITOR_LOG_PREFIX) +
                " Pattern registered: door + no motion within " +
                UNAUTHORIZED_ACCESS_TIME_WINDOW_SECONDS + "s → unauthorized access (501)")
                .c_str());
        }
    }

    /**
     * @brief Registers the Loading/Unloading Active standard pattern.
     *
     * Weight change detected AND door opened AND motion detected within 120 seconds.
     * Indicates active cargo operation, cooler stays ON.
     */
    void setupPatternForActiveCargoOperation() {
        int identifiers[] = {
            Sensor::DATA_READ_EVENT_IDENTIFIER,
            DoorWindowSensor::DOOR_OPENED_EVENT_IDENTIFIER,
            PassiveInfraredSensor::MOTION_DETECTED_EVENT_IDENTIFIER
        };
        int sources[] = {
            LOAD_CELL_DATA_PIN,  // DATA_READ: only from LoadCellAmplifier
            EventAggregator::ANY_SOURCE,  // door: any source
            EventAggregator::ANY_SOURCE   // motion: any source
        };
        bool ok = aggregator.addAllMatchPattern(
            identifiers, 3, sources, LOADING_UNLOADING_TIME_WINDOW_SECONDS,
            COMPOSITE_LOADING_UNLOADING);
        if (ok) {
            Serial.println((String(COLD_CHAIN_MONITOR_LOG_PREFIX) +
                " Pattern registered: weight (source " + LOAD_CELL_DATA_PIN +
                ") + door + motion within " + LOADING_UNLOADING_TIME_WINDOW_SECONDS +
                "s → loading active (502)").c_str());
        }
    }

public:
    ColdChainMonitor(
        int doorPin,
        int motionPin,
        int loadCellDataPin,
        int loadCellClockPin,
        int dhtPin,
        int relayPin,
        int buzzerPin,
        int ledPin)
        : Device(SAMPLING_INTERVAL_IN_MILLISECONDS, 0)
        , doorSensor(doorPin, &aggregator)   // Events go to aggregator, not directly to Device
        , motionSensor(motionPin, &aggregator)
        , loadCell(loadCellDataPin, loadCellClockPin,
                   LOAD_CELL_MIN_WEIGHT_IN_GRAMS, LOAD_CELL_MAX_WEIGHT_IN_GRAMS,
                   LOAD_CELL_MIN_RAW_VALUE, LOAD_CELL_MAX_RAW_VALUE,
                   LOAD_CELL_FILTER_DEPTH, &aggregator)
        , tempSensor(dhtPin, DhtSensor::SENSOR_MODEL_DHT11, this)
        , coolerRelay(relayPin, this)
        , buzzer(buzzerPin, this)
        , statusLed(ledPin, false, this)
        , doorOpen(false)
    {
        // Register interrupt-driven sensors (CHANGE edge)
        // IMPORTANT: Registration must occur before initializeAsynchronousEngine(), because
        // the engine start guard rejects any registration after the engine is running.
        appendSensorToTriggeringScheduler(
            &doorSensor, DoorWindowSensor::DOOR_OPENED_EVENT_IDENTIFIER, CHANGE);
        appendSensorToTriggeringScheduler(
            &motionSensor, PassiveInfraredSensor::MOTION_DETECTED_EVENT_IDENTIFIER, CHANGE);

        // Register LoadCellAmplifier with hybrid scheduling (interrupt + periodic)
        appendToBothSchedulers(
            &loadCell, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, FALLING);

        // Register DhtSensor with periodic sampling (not event-aggregated)
        appendSensorToScheduler(
            &tempSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);

        // Configure the EventAggregator to send composite events to this Device
        aggregator.setOutputHandler(this);

        // Set the static pointer so the condition function can access the load cell.
        coldChainLoadCell = &loadCell;

        // Register composite event patterns
        setupPatternForEnergySaveUnderCondition();
        setupPatternForUnauthorizedAccess();
        setupPatternForActiveCargoOperation();

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
        Device::setFirmwareVersion(COLD_CHAIN_MONITOR_FIRMWARE_VERSION);
        return true;
    }

    ~ColdChainMonitor() override {
        stopAsynchronousEngine();
        // Clear the static pointer so the condition function cannot access a destroyed instance.
        coldChainLoadCell = nullptr;
        // Remove all registered patterns after stopping the engine.
        aggregator.cleanupPatterns();
    }

    /**
     * @brief Handles composite events from the EventAggregator and individual sensor events.
     *
     * The EventAggregator intercepts door, motion, and weight events, checking them
     * against registered patterns. For conditional patterns, the condition function
     * is evaluated before emitting — so when COMPOSITE_ENERGY_SAVE arrives, the cargo
     * bay is guaranteed to be empty (the condition already checked).
     * The DhtSensor reports directly to this Device (not aggregated).
     *
     * @param event The event context payload.
     * @runs_on{sampler}
     */
    void on(Event event) override {
        switch (event.identifier) {
            case COMPOSITE_ENERGY_SAVE:
                // Pattern matched AND condition passed: door opened, weight changed,
                // and cargo bay is empty. Turn off the cooler relay to save energy.
                Serial.println((String(COLD_CHAIN_MONITOR_LOG_PREFIX) +
                    " *** ENERGY SAVE: Door open, bay empty — cooler relay OFF. ***")
                    .c_str());
                coolerRelay.handle(RelayActuator::RELAY_OFF_COMMAND);
                break;

            case COMPOSITE_UNAUTHORIZED_ACCESS:
                // Pattern matched: door opened AND motion stopped.
                // Door is open with cargo present, but no one is actively loading/unloading.
                Serial.println((String(COLD_CHAIN_MONITOR_LOG_PREFIX) +
                    " *** UNAUTHORIZED ACCESS: Door left open, no activity! ***").c_str());
                buzzer.handle(Buzzer::PLAY_TONE_COMMAND);
                statusLed.handle(Led::TOGGLE_LED_COMMAND);
                break;

            case COMPOSITE_LOADING_UNLOADING:
                // Pattern matched: weight change AND door opened AND motion detected.
                // All three conditions indicate active loading or unloading operation.
                // Keep the cooler running and log the activity.
                Serial.println((String(COLD_CHAIN_MONITOR_LOG_PREFIX) +
                    " *** LOADING/UNLOADING ACTIVE: Cargo operation detected, cooler remains ON. ***")
                    .c_str());
                break;

            case DoorWindowSensor::DOOR_OPENED_EVENT_IDENTIFIER:
                // Individual door event may arrive if the aggregator does not consume it.
                doorOpen = true;
                Serial.println((String(COLD_CHAIN_MONITOR_LOG_PREFIX) + " Door opened.").c_str());
                break;

            case DoorWindowSensor::DOOR_CLOSED_EVENT_IDENTIFIER:
                doorOpen = false;
                Serial.println((String(COLD_CHAIN_MONITOR_LOG_PREFIX) + " Door closed.").c_str());
                break;

            case Sensor::DATA_READ_EVENT_IDENTIFIER:
                // DhtSensor temperature reading (not aggregated).
                if (event.sourceId == DHT_SENSOR_PIN) {
                    float temp = tempSensor.getTemperatureInCelsius();
                    Serial.println((String(COLD_CHAIN_MONITOR_LOG_PREFIX) +
                        " Temperature: " + String(temp, 1) + "°C").c_str());
                }
                break;

            default:
                break;
        }
    }
};
//@} // Application Mediator Implementation

static ColdChainMonitor* coldChainMonitorDevice = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    coldChainMonitorDevice = new ColdChainMonitor(
        DOOR_SENSOR_PIN,
        MOTION_SENSOR_PIN,
        LOAD_CELL_DATA_PIN,
        LOAD_CELL_CLOCK_PIN,
        DHT_SENSOR_PIN,
        COOLER_RELAY_PIN,
        BUZZER_PIN,
        STATUS_LED_PIN
    );
    if (!coldChainMonitorDevice->start()) {
        Serial.println("[ColdChainMonitor] Engine startup failed; monitoring was not started.");
        return;
    }

    Serial.println((String(COLD_CHAIN_MONITOR_LOG_PREFIX) +
        " Cold chain monitoring active.").c_str());
}

void loop() {
    // All event handling is interrupt-driven; main loop remains idle.
    // vTaskDelay clarifies that this loop yields to FreeRTOS — the device is loopless.
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
