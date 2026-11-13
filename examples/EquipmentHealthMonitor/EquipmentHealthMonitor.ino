/**
 * @file EquipmentHealthMonitor.ino
 * @brief Industrial equipment health dashboard with structured logging and diagnostics.
 *
 * This example demonstrates a grounded real-world device: a monitoring unit attached to
 * industrial equipment (motor, pump, compressor) that tracks vibration (via LoadCellAmplifier
 * as a force proxy) and ambient temperature, reporting diagnostics to help predict
 * maintenance needs.
 *
 * The primary purpose of this example is to demonstrate the ModestLog zero-overhead
 * compile-time logging system and the Device::diagnostics() runtime health monitoring
 * API, which are essential for production deployments.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `ModestLog` zero-overhead compile-time logging with `MIOT_LOG_*` macros
 * - Log level filtering: `MIOT_LOG_I` (Info), `MIOT_LOG_W` (Warn), `MIOT_LOG_E` (Error)
 * - `Device::diagnostics()` for runtime health snapshot (`DiagnosticsSnapshot`)
 * - `EngineConfiguration` for custom task stack sizes, priorities, and core pinning
 * - `LoadCellAmplifier` for force/vibration measurement (proxy for accelerometer)
 * - `DhtSensor` for ambient temperature monitoring
 * - Periodic diagnostics reporting to Serial
 * - Loop-less, event-driven asynchronous execution via `initializeAsynchronousEngine()`
 *
 * @par Diagnostics metrics reported:
 * - Missed ticks: sampler handler overrun count
 * - Packets enqueued/dropped: telemetry queue health
 * - Queue high water mark: maximum queue depth observed
 * - Free heap / min free heap: memory pressure
 * - Stack high water marks: task stack usage
 * - Uptime seconds: device runtime
 *
 * @par Configuring log level:
 * - Define `MODEST_LOG_LEVEL` before including ModestIoT.h:
 *   `#define MODEST_LOG_LEVEL MODEST_LOG_LEVEL_INFO` (for Info and above)
 *   `#define MODEST_LOG_LEVEL MODEST_LOG_LEVEL_DEBUG` (for Debug and above)
 * - Default level is `MODEST_LOG_LEVEL_WARN` (Warn and above)
 *
 * @date 2026-11-12
 * @version 1.0
 */

/** @brief Log tag for this device component. */
#define EQUIP_HEALTH_LOG_TAG "EquipHealth"

/** @brief Number of samples between diagnostics reports. */
#define DIAGNOSTICS_REPORT_INTERVAL_SAMPLES 10

/** @brief Sampler event queue length for equipment health monitoring. */
#define SAMPLER_QUEUE_LENGTH 16

#include <Arduino.h>
#include "ModestIoT.h"

/** @brief Serial communication baud rate. */
static const unsigned long SERIAL_BAUD_RATE = 115200;

/** @brief Delay in the main loop when no work is pending. */
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;

/** @name Application Mediator Implementation */
//@{

/** @brief Minimum vibration weight for load cell calibration (grams). */
static const float VIBRATION_MIN_WEIGHT_IN_GRAMS = 0.0f;

/** @brief Maximum vibration weight for load cell calibration (grams). */
static const float VIBRATION_MAX_WEIGHT_IN_GRAMS = 1000.0f;

/** @brief Minimum raw ADC value for load cell calibration. */
static const long VIBRATION_MIN_RAW_VALUE = 0;

/** @brief Maximum raw ADC value for load cell calibration. */
static const long VIBRATION_MAX_RAW_VALUE = 1000000;

/** @brief Moving average filter depth for the load cell. */
static const int VIBRATION_FILTER_DEPTH = 5;

/** @brief Vibration warning threshold in grams. */
static const float VIBRATION_WARNING_THRESHOLD_IN_GRAMS = 5.0f;

/**
 * @brief Industrial equipment health monitoring device.
 *
 * This class extends the `Device` base class to manage vibration and temperature sensors,
 * using ModestLog for structured logging and Device::diagnostics() for runtime health
 * monitoring. Custom EngineConfiguration is used to tune task parameters for industrial
 * monitoring workloads.
 */
class EquipmentHealthMonitor : public Device {
private:
    LoadCellAmplifier vibrationSensor;
    DhtSensor tempSensor;

    uint32_t sampleCount; ///< Running count of samples for periodic diagnostics.

    /**
     * @brief Reports the current diagnostics snapshot to Serial.
     */
    void reportDiagnostics() {
        DiagnosticsSnapshot diag = diagnostics();

        MIOT_LOG_I(EQUIP_HEALTH_LOG_TAG, "=== Diagnostics Report (sample #%lu) ===", sampleCount);
        MIOT_LOG_I(EQUIP_HEALTH_LOG_TAG, "Uptime: %lu seconds", diag.uptimeSeconds);
        MIOT_LOG_I(EQUIP_HEALTH_LOG_TAG, "Missed ticks: %lu", diag.missedTicks);
        MIOT_LOG_I(EQUIP_HEALTH_LOG_TAG, "Packets enqueued: %lu, dropped: %lu",
                   diag.packetsEnqueued, diag.packetsDropped);
        MIOT_LOG_I(EQUIP_HEALTH_LOG_TAG, "Queue high water mark: %u", diag.queueHighWaterMark);
        MIOT_LOG_I(EQUIP_HEALTH_LOG_TAG, "Free heap: %lu bytes (min: %lu)",
                   diag.freeHeapBytes, diag.minFreeHeapBytes);
        MIOT_LOG_I(EQUIP_HEALTH_LOG_TAG, "Sampler stack free: %u words",
                   diag.samplerStackFreeWords);
        MIOT_LOG_I(EQUIP_HEALTH_LOG_TAG, "Network stack free: %u words",
                   diag.networkStackFreeWords);
        MIOT_LOG_I(EQUIP_HEALTH_LOG_TAG, "=== End Diagnostics ===");
    }

protected:
    void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const override {
        // This device does not send telemetry; it only monitors locally.
        (void)rawQueueItemPayload;
    }

    void maintainNetworkServices() const override {
        // No network services for this standalone monitoring device.
    }

public:
    /**
     * @brief Constructs a new EquipmentHealthMonitor application instance.
     *
     * Uses custom EngineConfiguration with increased stack sizes for industrial monitoring.
     *
     * @param vibrationDataPin GPIO pin connected to the HX711 Data (DOUT) line.
     * @param vibrationClockPin GPIO pin connected to the HX711 Clock (PD_SCK) line.
     * @param tempPin GPIO pin connected to the DHT22 data line.
     * @param samplingIntervalInMilliseconds How often to sample sensors (ms).
     * @param timerGroupIndex Hardware timer group index (0-3).
     */
    EquipmentHealthMonitor(int vibrationDataPin, int vibrationClockPin, int tempPin,
                           unsigned long samplingIntervalInMilliseconds,
                           uint8_t timerGroupIndex = 0)
        : Device(samplingIntervalInMilliseconds, timerGroupIndex),
          vibrationSensor(vibrationDataPin, vibrationClockPin, VIBRATION_MIN_WEIGHT_IN_GRAMS,
                          VIBRATION_MAX_WEIGHT_IN_GRAMS, VIBRATION_MIN_RAW_VALUE,
                          VIBRATION_MAX_RAW_VALUE, VIBRATION_FILTER_DEPTH, this),
          tempSensor(tempPin, DhtSensor::SENSOR_MODEL_DHT22, this),
          sampleCount(0)
    {
        // Custom engine configuration for industrial monitoring workload
        EngineConfiguration config;
        config.samplerStackBytes = 8192;    ///< Larger stack for sensor processing
        config.networkStackBytes = 4096;    ///< Smaller stack (no network)
        config.samplerPriority = 3;
        config.queueLength = SAMPLER_QUEUE_LENGTH;
        initializeAsynchronousEngine(config);

        // Hybrid scheduling: interrupt-driven data-ready detection eliminates the 100 ms busy-wait.
        appendToBothSchedulers(&vibrationSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, FALLING);
        appendSensorToScheduler(&tempSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);

        MIOT_LOG_I(EQUIP_HEALTH_LOG_TAG, "Engine configured: sampler stack %lu bytes, queue length %u",
                   config.samplerStackBytes, config.queueLength);
    }

    ~EquipmentHealthMonitor() override = default;

    /**
     * @brief Handles sensor data events and reports readings with structured logging.
     *
     * @param event The triggered event context payload.
     * @runs_on{sampler}
     */
    void on(Event event) override {
        if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
            sampleCount++;

            float vibration = vibrationSensor.getWeightInGrams();
            float temperature = tempSensor.getTemperatureInCelsius();

            MIOT_LOG_I(EQUIP_HEALTH_LOG_TAG, "Sample #%lu -> Vibration: %.2f g, Temp: %.1f°C",
                       sampleCount, vibration, temperature);

            // Check for abnormal vibration levels
            if (vibration > VIBRATION_WARNING_THRESHOLD_IN_GRAMS) {
                MIOT_LOG_W(EQUIP_HEALTH_LOG_TAG, "High vibration detected: %.2f g (sample #%lu)",
                           vibration, sampleCount);
            }

            // Periodically report full diagnostics
            if (sampleCount % DIAGNOSTICS_REPORT_INTERVAL_SAMPLES == 0) {
                reportDiagnostics();
            }
        }
    }

    void handle(Command command) override {
        // This device does not process external commands.
        (void)command;
    }

    /**
     * @brief Initializes the device and logs startup information.
     * @return True if sensors are responding.
     */
    bool start() {
        MIOT_LOG_I(EQUIP_HEALTH_LOG_TAG, "Equipment health monitor starting...");

        vibrationSensor.tare();
        if (vibrationSensor.getWeightInGrams() < 0.0f) {
            MIOT_LOG_W(EQUIP_HEALTH_LOG_TAG, "Load cell amplifier may need recalibration on pin %d",
                       vibrationSensor.getSignalGpioPin());
        }

        Serial.println("[EquipmentHealthMonitor] Equipment health monitor online.");
        MIOT_LOG_I(EQUIP_HEALTH_LOG_TAG, "Monitoring active. Diagnostics every %u samples.",
                   DIAGNOSTICS_REPORT_INTERVAL_SAMPLES);
        return true;
    }
};
//@} // Application Mediator Implementation

/** @name Top-Level Declarative Canvas */
//@{

static const int VIBRATION_SENSOR_DATA_PIN   = 34; ///< GPIO pin for HX711 Data (DOUT).
static const int VIBRATION_SENSOR_CLOCK_PIN  = 32; ///< GPIO pin for HX711 Clock (PD_SCK).
static const int TEMPERATURE_SENSOR_PIN      = 4;  ///< GPIO pin for DHT22 data line.

static const unsigned long SAMPLING_INTERVAL_IN_MILLISECONDS = 2000;
static const uint8_t HARDWARE_TIMER_CHANNEL_INDEX = 0;

static EquipmentHealthMonitor* healthMonitor = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    healthMonitor = new EquipmentHealthMonitor(
        VIBRATION_SENSOR_DATA_PIN,
        VIBRATION_SENSOR_CLOCK_PIN,
        TEMPERATURE_SENSOR_PIN,
        SAMPLING_INTERVAL_IN_MILLISECONDS,
        HARDWARE_TIMER_CHANNEL_INDEX
    );

    if (!healthMonitor->start()) {
        Serial.println("[EquipmentHealthMonitor] Device startup failed.");
        return;
    }
}

void loop() {
    // The master loop is completely clean and empty.
    // The framework handles all timing, sensor sampling, and diagnostics reporting.
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
//@} // Top-Level Declarative Canvas
