/**
 * @file PharmaceuticalCleanRoom.ino
 * @brief Nested EventAggregator chains for clean room contamination detection (W31).
 *
 * This example demonstrates **nested EventAggregator** instances where composite events
 * from one aggregator feed into another, forming a multi-level pattern detection chain.
 * The scenario is a pharmaceutical clean room monitoring system that detects environmental
 * deviations, correlates them into contamination risks, and escalates to batch quarantine.
 *
 * Unlike IntrusionDetector (standard patterns) and ColdChainMonitor (conditional patterns),
 * this example focuses on **chaining aggregators** to express hierarchical detection logic
 * that mirrors how clean room protocols actually work: faults → situations → escalation.
 *
 * @par Aggregator Chain Architecture:
 *
 * ```
 * Sensors → Aggregator Level 1 (Environmental Deviations)
 *            ↓ composite events (sourceId = DEVICE_SOURCE_ID)
 *          Aggregator Level 2 (Contamination Risks)
 *            ↓ composite events (sourceId = DEVICE_SOURCE_ID)
 *          Aggregator Level 3 (Batch Protection)
 *            ↓ composite events
 *          Device::on(Event) — triggers relay lockdown, buzzer, LED
 * ```
 *
 * @par Level 1 — Environmental Deviations:
 * - **Pressure Breach**: Pressure change (DATA_READ from PthSensor) AND door opened (72)
 *   within 15 seconds → composite event 500.
 *   Uses source-aware pattern to distinguish PthSensor from other DATA_READ sensors.
 * - **Environmental Anomaly**: Gas detected (40) OR temperature excursion (DATA_READ from
 *   DhtSensor) within 30 seconds → composite event 501.
 *   Demonstrates `addAnyMatchPattern()` (OR logic) — any one event triggers the pattern.
 *
 * @par Level 2 — Contamination Risks:
 * - **Contamination Risk**: Pressure breach (500) AND environmental anomaly (501) within
 *   60 seconds → composite event 600.
 *   Uses `DEVICE_SOURCE_ID` source filtering to match only composite events from Level 1,
 *   not raw sensor events that might share the same identifier.
 *
 * @par Level 3 — Batch Protection:
 * - **Quarantine Batch**: Contamination risk (600) within 10 seconds → composite event 700.
 *   Single-event pattern that triggers the full emergency response.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - Nested `EventAggregator` chains via `setOutputHandler()`
 * - `EventAggregator::DEVICE_SOURCE_ID` for source filtering between aggregator levels
 * - `addAllMatchPattern()` for AND logic (Levels 1, 2, 3)
 * - `addAnyMatchPattern()` for OR logic (Level 1 environmental anomaly)
 * - Source-aware patterns distinguish sensors sharing the same event identifier
 * - `removePattern()` for dynamic pattern removal during cleanup
 * - `cleanupPatterns()` for removing all patterns in one call
 * - `PthSensor` for pressure monitoring (DATA_READ, I2C)
 * - `DhtSensor` for temperature monitoring (DATA_READ)
 * - `GasSensor` for VOC/particle detection (interrupt-driven)
 * - `DoorWindowSensor` for clean room door state (interrupt-driven)
 * - `RelayActuator` for HVAC lockdown control
 * - `Buzzer` for audible alerts
 * - `Led` for visual status indication
 * - Zero heap allocation via fixed-size circular buffers
 *
 * @par Serial Output Example:
 * @code
 * [PharmaceuticalCleanRoom] Level 1: pressure breach pattern registered (500)
 * [PharmaceuticalCleanRoom] Level 1: environmental anomaly (OR) pattern registered (501)
 * [PharmaceuticalCleanRoom] Level 2: contamination risk pattern registered (600)
 * [PharmaceuticalCleanRoom] Level 3: quarantine batch pattern registered (700)
 * [PharmaceuticalCleanRoom] *** PRESSURE BREACH: Door opened during pressure change. ***
 * [PharmaceuticalCleanRoom] *** ENVIRONMENTAL ANOMALY: Gas or temperature deviation. ***
 * [PharmaceuticalCleanRoom] *** CONTAMINATION RISK: Multiple deviations detected. ***
 * [PharmaceuticalCleanRoom] *** QUARANTINE: Batch isolated, HVAC locked down. ***
 * [PharmaceuticalCleanRoom] Relay OFF — HVAC lockdown activated.
 * [PharmaceuticalCleanRoom] Buzzer alarm triggered.
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
/** @brief GPIO pin for the PthSensor (BME280) pressure/temperature sensor (I2C). */
static const int PTH_SENSOR_PIN = 8;
/** @brief Shared I2C bus configuration for the BME280 pressure sensor. */
static const I2cBusConfiguration PHARMA_CLEAN_ROOM_I2C_CONFIGURATION{-1, -1, 100000};
/** @brief GPIO pin for the DHT temperature/humidity sensor. */
static const int DHT_SENSOR_PIN = 17;
/** @brief GPIO pin for the gas sensor (VOC/particle detection). */
static const int GAS_SENSOR_PIN = 16;
/** @brief GPIO pin for the clean room door sensor. */
static const int DOOR_SENSOR_PIN = 5;
/** @brief GPIO pin for the HVAC control relay actuator. */
static const int HVAC_RELAY_PIN = 25;
/** @brief GPIO pin for the buzzer. */
static const int BUZZER_PIN = 26;
/** @brief GPIO pin for the status LED. */
static const int STATUS_LED_PIN = 2;
//@} // GPIO Pin Assignments

/** @name Device Configuration */
//@{
/** @brief Firmware version string for this device. */
static const char* PHARMA_CLEAN_ROOM_FIRMWARE_VERSION = "PCR-300 FW 1.0.0";
//@} // Device Configuration

/** @name Serial Output Configuration */
//@{
/** @brief Serial communication baud rate. */
static const unsigned long SERIAL_BAUD_RATE = 115200;
/** @brief Log prefix for all PharmaceuticalCleanRoom serial output. */
static const char* PHARMA_CLEAN_ROOM_LOG_PREFIX = "[PharmaceuticalCleanRoom]";
/** @brief Delay in the main loop when no work is pending. */
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;
//@} // Serial Output Configuration

/** @name Level 1 Composite Event Identifiers (Environmental Deviations) */
//@{
/** @brief Composite event: pressure breach detected (pressure change + door opened). */
static const int COMPOSITE_PRESSURE_BREACH = 500;
/** @brief Composite event: environmental anomaly (gas OR temperature deviation). */
static const int COMPOSITE_ENVIRONMENTAL_ANOMALY = 501;
//@} // Level 1 Composite Event Identifiers

/** @name Level 2 Composite Event Identifiers (Contamination Risks) */
//@{
/** @brief Composite event: contamination risk (pressure breach AND environmental anomaly). */
static const int COMPOSITE_CONTAMINATION_RISK = 600;
//@} // Level 2 Composite Event Identifiers

/** @name Level 3 Composite Event Identifiers (Batch Protection) */
//@{
/** @brief Composite event: batch quarantine triggered. */
static const int COMPOSITE_QUARANTINE_BATCH = 700;
//@} // Level 3 Composite Event Identifiers

/** @name Level 1 Pattern Time Windows */
//@{
/** @brief Maximum seconds between pressure change and door open for pressure breach. */
static const uint32_t PRESSURE_BREACH_TIME_WINDOW_SECONDS = 15;
/** @brief Maximum seconds for gas OR temperature anomaly. */
static const uint32_t ENVIRONMENTAL_ANOMALY_TIME_WINDOW_SECONDS = 30;
//@} // Level 1 Pattern Time Windows

/** @name Level 2 Pattern Time Windows */
//@{
/** @brief Maximum seconds between pressure breach and environmental anomaly. */
static const uint32_t CONTAMINATION_RISK_TIME_WINDOW_SECONDS = 60;
//@} // Level 2 Pattern Time Windows

/** @name Level 3 Pattern Time Windows */
//@{
/** @brief Maximum seconds for contamination risk to trigger quarantine. */
static const uint32_t QUARANTINE_TIME_WINDOW_SECONDS = 10;
//@} // Level 3 Pattern Time Windows

//@} // Top-Level Declarative Canvas

/** @name Application Mediator Implementation */
//@{

/**
 * @brief Pharmaceutical clean room monitor using nested EventAggregator chains.
 *
 * Extends Device to handle composite events from a 3-level EventAggregator chain.
 * Level 1 detects environmental deviations, Level 2 correlates contamination risks,
 * and Level 3 triggers batch quarantine. The chain demonstrates:
 * - `addAllMatchPattern()` for AND logic across all levels
 * - `addAnyMatchPattern()` for OR logic at Level 1
 * - `EventAggregator::DEVICE_SOURCE_ID` for source filtering between levels
 * - `removePattern()` and `cleanupPatterns()` for proper housekeeping
 *
 * The aggregator chain:
 * ```
 * Sensors → Level 1 (deviations) → Level 2 (risks) → Level 3 (quarantine) → Device
 * ```
 */
class PharmaceuticalCleanRoom : public Device {
private:
    ArduinoI2cBus i2cBus;
    PthSensor pressureSensor;
    DhtSensor tempSensor;
    GasSensor gasSensor;
    DoorWindowSensor doorSensor;
    RelayActuator hvacRelay;
    Buzzer buzzer;
    Led statusLed;

    /// @brief Level 1 aggregator: detects environmental deviations.
    EventAggregator aggregatorLevel1;

    /// @brief Level 2 aggregator: correlates deviations into contamination risks.
    EventAggregator aggregatorLevel2;

    /// @brief Level 3 aggregator: triggers batch quarantine.
    EventAggregator aggregatorLevel3;

    /**
     * @brief Registers Level 1 pattern: Pressure Breach (AND pattern).
     *
     * Pressure change (DATA_READ from PthSensor) AND door opened within 15 seconds.
     * Uses source-aware pattern to match only PthSensor's DATA_READ events.
     */
    void setupLevel1PressureBreachPattern() {
        int identifiers[] = {
            Sensor::DATA_READ_EVENT_IDENTIFIER,
            DoorWindowSensor::DOOR_OPENED_EVENT_IDENTIFIER
        };
        int sources[] = {
            PTH_SENSOR_PIN,  // DATA_READ: only from PthSensor
            EventAggregator::ANY_SOURCE  // door: any source
        };
        bool ok = aggregatorLevel1.addAllMatchPattern(
            identifiers, 2, sources, PRESSURE_BREACH_TIME_WINDOW_SECONDS,
            COMPOSITE_PRESSURE_BREACH);
        if (ok) {
            Serial.println((String(PHARMA_CLEAN_ROOM_LOG_PREFIX) +
                " Level 1: pressure breach pattern registered (" +
                COMPOSITE_PRESSURE_BREACH + ")").c_str());
        }
    }

    /**
     * @brief Registers Level 1 pattern: Environmental Anomaly (OR pattern).
     *
     * Gas detected (40) OR temperature excursion (DATA_READ from DhtSensor)
     * within 30 seconds. Demonstrates `addAnyMatchPattern()` — any one event
     * triggers the composite event.
     */
    void setupLevel1EnvironmentalAnomalyPattern() {
        int identifiers[] = {
            GasSensor::DIGITAL_GAS_DETECTED_EVENT_IDENTIFIER,
            Sensor::DATA_READ_EVENT_IDENTIFIER
        };
        int sources[] = {
            EventAggregator::ANY_SOURCE,  // gas: any source
            DHT_SENSOR_PIN  // DATA_READ: only from DhtSensor
        };
        bool ok = aggregatorLevel1.addAnyMatchPattern(
            identifiers, 2, sources, ENVIRONMENTAL_ANOMALY_TIME_WINDOW_SECONDS,
            COMPOSITE_ENVIRONMENTAL_ANOMALY);
        if (ok) {
            Serial.println((String(PHARMA_CLEAN_ROOM_LOG_PREFIX) +
                " Level 1: environmental anomaly (OR) pattern registered (" +
                COMPOSITE_ENVIRONMENTAL_ANOMALY + ")").c_str());
        }
    }

    /**
     * @brief Registers Level 2 pattern: Contamination Risk (AND pattern).
     *
     * Pressure breach (500) AND environmental anomaly (501) within 60 seconds.
     * Uses `DEVICE_SOURCE_ID` source filtering to match only composite events
     * from Level 1, not raw sensor events that might share the same identifier.
     */
    void setupLevel2ContaminationRiskPattern() {
        int identifiers[] = {
            COMPOSITE_PRESSURE_BREACH,
            COMPOSITE_ENVIRONMENTAL_ANOMALY
        };
        int sources[] = {
            EventAggregator::DEVICE_SOURCE_ID,  // from Level 1 aggregator
            EventAggregator::DEVICE_SOURCE_ID   // from Level 1 aggregator
        };
        bool ok = aggregatorLevel2.addAllMatchPattern(
            identifiers, 2, sources, CONTAMINATION_RISK_TIME_WINDOW_SECONDS,
            COMPOSITE_CONTAMINATION_RISK);
        if (ok) {
            Serial.println((String(PHARMA_CLEAN_ROOM_LOG_PREFIX) +
                " Level 2: contamination risk pattern registered (" +
                COMPOSITE_CONTAMINATION_RISK + ")").c_str());
        }
    }

    /**
     * @brief Registers Level 3 pattern: Quarantine Batch (AND pattern).
     *
     * Contamination risk (600) within 10 seconds → triggers full quarantine.
     * Single-event pattern demonstrating the final escalation level.
     * Uses `DEVICE_SOURCE_ID` to match only composite events from Level 2.
     */
    void setupLevel3QuarantineBatchPattern() {
        int identifiers[] = {
            COMPOSITE_CONTAMINATION_RISK
        };
        int sources[] = {
            EventAggregator::DEVICE_SOURCE_ID  // from Level 2 aggregator
        };
        bool ok = aggregatorLevel3.addAllMatchPattern(
            identifiers, 1, sources, QUARANTINE_TIME_WINDOW_SECONDS,
            COMPOSITE_QUARANTINE_BATCH);
        if (ok) {
            Serial.println((String(PHARMA_CLEAN_ROOM_LOG_PREFIX) +
                " Level 3: quarantine batch pattern registered (" +
                COMPOSITE_QUARANTINE_BATCH + ")").c_str());
        }
    }

public:
    PharmaceuticalCleanRoom(
        int pthPin,
        int dhtPin,
        int gasPin,
        int doorPin,
        int relayPin,
        int buzzerPin,
        int ledPin)
        : Device(SAMPLING_INTERVAL_IN_MILLISECONDS, 0)
        , i2cBus()
        , pressureSensor(i2cBus, pthPin, PthSensor::DEFAULT_I2C_ADDRESS, &aggregatorLevel1)
        , tempSensor(dhtPin, DhtSensor::SENSOR_MODEL_DHT11, &aggregatorLevel1)
        , gasSensor(gasPin, -1, &aggregatorLevel1)
        , doorSensor(doorPin, &aggregatorLevel1)
        , hvacRelay(relayPin, this)
        , buzzer(buzzerPin, this)
        , statusLed(ledPin, false, this)
    {}

    bool start() {
        Device::setFirmwareVersion(PHARMA_CLEAN_ROOM_FIRMWARE_VERSION);
        const I2cStatus busStatus = i2cBus.start(PHARMA_CLEAN_ROOM_I2C_CONFIGURATION);
        if (busStatus != I2cStatus::Ok) {
            Serial.printf("%s I2C bus startup failed (status %u).\n",
                          PHARMA_CLEAN_ROOM_LOG_PREFIX, static_cast<unsigned int>(busStatus));
            return false;
        }
        if (!pressureSensor.start()) {
            Serial.printf("%s BME280 startup failed (status %u).\n",
                          PHARMA_CLEAN_ROOM_LOG_PREFIX,
                          static_cast<unsigned int>(pressureSensor.getLastI2cResult().status));
            return false;
        }

        // Register interrupt-driven sensors (CHANGE edge)
        // IMPORTANT: Registration must occur before initializeAsynchronousEngine(), because
        // the engine start guard rejects any registration after the engine is running.
        appendSensorToTriggeringScheduler(
            &gasSensor, GasSensor::DIGITAL_GAS_DETECTED_EVENT_IDENTIFIER, CHANGE);
        appendSensorToTriggeringScheduler(
            &doorSensor, DoorWindowSensor::DOOR_OPENED_EVENT_IDENTIFIER, CHANGE);

        // Register periodic sensors
        appendSensorToScheduler(
            &pressureSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        appendSensorToScheduler(
            &tempSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);

        // Chain the aggregators: Level 1 → Level 2 → Level 3 → Device
        aggregatorLevel1.setOutputHandler(&aggregatorLevel2);
        aggregatorLevel2.setOutputHandler(&aggregatorLevel3);
        aggregatorLevel3.setOutputHandler(this);

        // Register patterns at each level
        setupLevel1PressureBreachPattern();
        setupLevel1EnvironmentalAnomalyPattern();
        setupLevel2ContaminationRiskPattern();
        setupLevel3QuarantineBatchPattern();

        // Launch internal background task workers
        return initializeAsynchronousEngine(ASYNC_ENGINE_QUEUE_DEPTH);
    }

    ~PharmaceuticalCleanRoom() override {
        stopAsynchronousEngine();
        // Demonstrate removePattern() by explicitly removing one pattern before cleanup.
        // In a real system, this could be used to dynamically disable patterns
        // when certain conditions change (e.g., maintenance mode).
        aggregatorLevel1.removePattern(COMPOSITE_PRESSURE_BREACH);

        // Remove all remaining patterns from all aggregator levels.
        aggregatorLevel1.cleanupPatterns();
        aggregatorLevel2.cleanupPatterns();
        aggregatorLevel3.cleanupPatterns();

    }

    /**
     * @brief Handles composite events from the Level 3 EventAggregator.
     *
     * Only Level 3 composite events reach this handler. Level 1 and Level 2
     * composite events are consumed by the next aggregator in the chain.
     * Individual sensor events are NOT received here because the sensors'
     * parent handler is set to aggregatorLevel1, not the Device.
     *
     * @param event The composite event context payload.
     * @runs_on{sampler}
     */
    void on(Event event) override {
        switch (event.identifier) {
            case COMPOSITE_QUARANTINE_BATCH:
                // Level 3 matched: contamination risk confirmed.
                // Trigger full emergency response: HVAC lockdown, alarm, status LED.
                Serial.println((String(PHARMA_CLEAN_ROOM_LOG_PREFIX) +
                    " *** QUARANTINE: Batch isolated, HVAC locked down. ***").c_str());
                hvacRelay.handle(RelayActuator::RELAY_OFF_COMMAND);
                buzzer.handle(Buzzer::PLAY_TONE_COMMAND);
                statusLed.handle(Led::TOGGLE_LED_COMMAND);
                break;

            default:
                // Intermediate composite events from Level 1 or Level 2 may arrive
                // if the chain does not consume them; log them for diagnostics.
                Serial.println((String(PHARMA_CLEAN_ROOM_LOG_PREFIX) +
                    " Composite event received: " + event.identifier).c_str());
                break;
        }
    }
};
//@} // Application Mediator Implementation

static PharmaceuticalCleanRoom* pharmaCleanRoomDevice = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    pharmaCleanRoomDevice = new PharmaceuticalCleanRoom(
        PTH_SENSOR_PIN,
        DHT_SENSOR_PIN,
        GAS_SENSOR_PIN,
        DOOR_SENSOR_PIN,
        HVAC_RELAY_PIN,
        BUZZER_PIN,
        STATUS_LED_PIN
    );
    if (pharmaCleanRoomDevice->start()) {
        Serial.println((String(PHARMA_CLEAN_ROOM_LOG_PREFIX) +
            " Clean room monitoring active — nested aggregator chain initialized.").c_str());
    } else {
        Serial.println("[PharmaceuticalCleanRoom] Startup failed; monitoring engine was not started.");
    }
}

void loop() {
    // All event handling is interrupt-driven; main loop remains idle.
    // vTaskDelay clarifies that this loop yields to FreeRTOS — the device is loopless.
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
