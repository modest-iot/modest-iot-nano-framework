/**
 * @file VibrationMonitor.ino
 * @brief Industrial vibration monitor with on-device ML anomaly detection.
 *
 * Monitors motor vibration using an MPU6050 accelerometer and runs an emlearn-based
 * anomaly detection model directly on the ESP32. When abnormal vibration is
 * detected, the device triggers an audible alert via a Buzzer actuator.
 * This example demonstrates the complete sensor → ML → actuator pipeline:
 * ImuSensor collects vibration data, EmlearnController runs inference,
 * and Buzzer sounds an alert when an anomaly is detected.
 *
 * emlearn generates inline C code with a `predict()` function. Instead of
 * calling it directly, this example passes the generated function to
 * EmlearnController::loadGeneratedModel(), so the Device interacts with the
 * model through a uniform API that supports swapping ML runtimes.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `Device` base class with event-driven sampling
 * - `ImuSensor` with windowed statistics (RMS, peak-to-peak)
 * - `AiMicroModelController` abstract interface for ML inference
 * - `EmlearnController` wrapping emlearn-generated model via function pointer
 * - `Buzzer` actuator for audible anomaly alerts
 * - `SamplingScheduler` for periodic vibration sampling
 * @date 2026-11-22
 * @version 1.0
 */

#include <Arduino.h>
#include <ModestIoT.h>

// Include the emlearn-generated model.
// In a real project, generate this file using:
//   emlearn convert vibration_model.pkl --trees --optimize --output vibration_model.h
//
// The generated header provides:
//   - vibration_predict(int16_t* features, int32_t length) → class index
//   - vibration_predict_proba(int16_t* features, int32_t length, float* out, int outLen)
#include "vibration_model.h"

/** @name Hardware Configuration */
//@{
/** @brief I²C address of the MPU6050 accelerometer (AD0 pin low). */
static const int ACCELEROMETER_I2C_ADDRESS = 0x68;
/** @brief GPIO pin for the alert buzzer. */
static const int BUZZER_GPIO_PIN = 13;
//@} // Hardware Configuration

/** @name Timing Configuration */
//@{
/** @brief Vibration sampling interval in milliseconds (1 second). */
static const unsigned long VIBRATION_SAMPLE_INTERVAL_IN_MILLISECONDS = 1000;
/** @brief Delay in the main loop when no work is pending. */
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;
//@} // Timing Configuration

/** @name Framework Configuration */
//@{
/** @brief Default hardware timer channel index. */
static const uint8_t DEFAULT_TIMER_CHANNEL = 0;
/** @brief Sampler event queue length. */
static const int SAMPLER_QUEUE_LENGTH = 5;
/** @brief Serial communication baud rate. */
static const unsigned long SERIAL_BAUD_RATE = 115200;
//@} // Framework Configuration

/** @name ML Model Configuration */
//@{
/** @brief Number of features in the vibration model (6-axis accelerometer, quantized). */
static const int32_t VIBRATION_MODEL_FEATURES = 6;
/** @brief ML prediction class index for normal operation. */
static const int32_t NORMAL_CLASS = 0;
/** @brief ML prediction class index for anomaly detected. */
static const int32_t ANOMALY_CLASS = 1;
//@} // ML Model Configuration

/** @name Buzzer Alert Configuration */
//@{
/** @brief Alert tone frequency in Hertz for anomaly detection. */
static const int ANOMALY_ALERT_TONE_FREQUENCY_IN_HERTZ = 2000;
/** @brief Alert tone duration in milliseconds. */
static const unsigned long ANOMALY_ALERT_TONE_DURATION_IN_MILLISECONDS = 500;
//@} // Buzzer Alert Configuration

/** @name Serial Output Configuration */
//@{
/** @brief Buffer size for formatted serial output messages. */
static const size_t SERIAL_OUTPUT_BUFFER_SIZE = 128;
/** @brief Log prefix for all VibrationMonitor serial output. */
static const char* VIBRATION_MONITOR_LOG_PREFIX = "[VibrationMonitor]";
//@} // Serial Output Configuration

/**
 * @brief Industrial Vibration Monitor with On-Device ML Anomaly Detection.
 * Demonstrates EmlearnController wrapping an emlearn-generated model. The
 * device samples vibration data from an MPU6050 accelerometer, extracts
 * time-domain features (RMS and peak-to-peak acceleration), and runs inference
 * through EmlearnController which handles quantization internally.
 */
class VibrationMonitor : public Device {
private:
    ImuSensor accelerometer;
    Buzzer alertBuzzer;
    EmlearnController mlController;

    /**
     * @brief Handles ML inference result and triggers appropriate action.
     * @param prediction ML prediction class index.
     */
    void handlePrediction(int32_t prediction) {
        char buffer[SERIAL_OUTPUT_BUFFER_SIZE];

        if (prediction == ANOMALY_CLASS) {
            snprintf(buffer, SERIAL_OUTPUT_BUFFER_SIZE,
                     "%s ANOMALY DETECTED — vibration pattern abnormal",
                     VIBRATION_MONITOR_LOG_PREFIX);
            Serial.println(buffer);

            // Trigger audible alert via Buzzer actuator.
            // Uses PLAY_TONE_COMMAND with buffered frequency and duration.
            alertBuzzer.setTone(ANOMALY_ALERT_TONE_FREQUENCY_IN_HERTZ,
                                ANOMALY_ALERT_TONE_DURATION_IN_MILLISECONDS);
            alertBuzzer.handleCommand(Buzzer::PLAY_TONE_COMMAND);
        } else {
            snprintf(buffer, SERIAL_OUTPUT_BUFFER_SIZE,
                     "%s Normal vibration — class %d",
                     VIBRATION_MONITOR_LOG_PREFIX, prediction);
            Serial.println(buffer);
        }
    }

public:
    VibrationMonitor()
        : Device(DEFAULT_TIMER_CHANNEL, SAMPLER_QUEUE_LENGTH)
        , accelerometer(ACCELEROMETER_I2C_ADDRESS, ImuSensor::DEFAULT_I2C_ADDRESS,
                        ImuSensor::IMU_ACCEL_FS_8G,
                        ImuSensor::IMU_GYRO_FS_2000, this)
        , alertBuzzer(BUZZER_GPIO_PIN, this)
    {
    }

    void start() {
        setFirmwareVersion("1.0.0");

        // Initialize the MPU6050 accelerometer
        if (!accelerometer.initialize()) {
            Serial.println(F(VIBRATION_MONITOR_LOG_PREFIX " ERROR: MPU6050 not found!"));
        } else {
            Serial.println(F(VIBRATION_MONITOR_LOG_PREFIX " MPU6050 initialized successfully"));
        }

        // Load the emlearn-generated model into EmlearnController.
        // The generated vibration_predict() function is wrapped behind the
        // AiMicroModelController interface, so the device interacts with
        // the model through a uniform API.
        mlController.loadGeneratedModel(&vibration_predict);

        Serial.println(F(VIBRATION_MONITOR_LOG_PREFIX " Starting vibration monitor"));
        Serial.println(F(VIBRATION_MONITOR_LOG_PREFIX " ML model: 3 trees, "
                        "6 features, 2 classes (normal/anomaly)"));
        Serial.println(F(VIBRATION_MONITOR_LOG_PREFIX " Accelerometer: MPU6050 (±8g, ±2000°/s)"));
        snprintf(buffer, SERIAL_OUTPUT_BUFFER_SIZE,
                 "%s Alert buzzer: GPIO %d",
                 VIBRATION_MONITOR_LOG_PREFIX, BUZZER_GPIO_PIN);
        Serial.println(buffer);
        Serial.println(F(VIBRATION_MONITOR_LOG_PREFIX " ML inference ready"));

        Device::start();

        // Register MPU6050 accelerometer with the scheduler.
        // The accelerometer samples at 1 Hz (1000 ms interval) for vibration analysis.
        appendSensorToScheduler(
            &accelerometer,
            MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER,
            VIBRATION_SAMPLE_INTERVAL_IN_MILLISECONDS
        );
    }

    void on(Event event) override {
        if (event.identifier == DATA_READ_EVENT_IDENTIFIER) {
            // Extract time-domain features directly from the accelerometer sensor.
            // The sensor maintains a circular sample buffer and computes
            // windowed statistics (RMS and peak-to-peak) automatically.
            float features[VIBRATION_MODEL_FEATURES];
            features[0] = accelerometer.getAccelerationXRmsInGForce();
            features[1] = accelerometer.getAccelerationYRmsInGForce();
            features[2] = accelerometer.getAccelerationZRmsInGForce();
            features[3] = accelerometer.getAccelerationXPeakToPeakInGForce();
            features[4] = accelerometer.getAccelerationYPeakToPeakInGForce();
            features[5] = accelerometer.getAccelerationZPeakToPeakInGForce();

            // Run ML inference through EmlearnController.
            // The controller wraps the generated vibration_predict() function
            // and provides a uniform float-based API.
            int32_t prediction = mlController.predict(features, VIBRATION_MODEL_FEATURES);

            if (mlController.lastResult() == InferenceResult::Success) {
                handlePrediction(prediction);
            } else {
                Serial.println(F(VIBRATION_MONITOR_LOG_PREFIX " ML inference failed"));
            }
        }
    }
};

/** @brief Global vibration monitor instance. */
VibrationMonitor vibrationMonitor;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    vibrationMonitor.start();
}

void loop() {
    // The master loop is completely clean and empty.
    // The framework handles all timing, sensor sampling, and ML inference asynchronously.
    // vTaskDelay clarifies that this loop yields to FreeRTOS — the device is loopless.
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
