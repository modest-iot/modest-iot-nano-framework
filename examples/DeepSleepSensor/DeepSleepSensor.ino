/**
 * @file DeepSleepSensor.ino
 * @brief Deep sleep sensor example demonstrating ESP32 deep sleep cycles.
 * @date 2026-11-09
 * @version 1.0
 *
 * This example shows how to use PowerMode::DeepSleep for battery-powered
 * operation. The device wakes from deep sleep, reads soil moisture, reports
 * the reading via Serial, and returns to deep sleep. Unlike light sleep,
 * deep sleep shuts down the CPU, Wi-Fi, and most peripherals (~10µA).
 * The device reboots on each wake — only RTC memory survives.
 */

#include <Arduino.h>
#include <ModestIoT.h>

// --- DEFAULT CONFIGURATION MACROS ---
// Preprocessor macros for constructor defaults, allowing use before class declaration.

/** @brief Default deep sleep interval in milliseconds (5 minutes). */
#define DEFAULT_DEEP_SLEEP_INTERVAL_IN_MILLISECONDS 300000UL

/** @brief Default hardware timer channel index. */
#define DEFAULT_TIMER_CHANNEL 0

/** @brief Sampler event queue length for battery operation. */
#define SAMPLER_QUEUE_LENGTH 5

// Pin definitions
static const int SOIL_MOISTURE_PIN = 34;

// Timing constants
static const unsigned long SERIAL_BAUD_RATE = 115200;
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;

// RTC memory variables that survive deep sleep
RTC_DATA_ATTR uint32_t rtcBootCount = 0; ///< Boot cycle counter
RTC_DATA_ATTR uint32_t rtcSequenceCounter = 0; ///< Telemetry sequence counter

/**
 * @brief Deep Sleep Sensor Device.
 * Demonstrates ESP32 deep sleep with periodic soil moisture sampling.
 * The device wakes, reads the sensor, prints the result, and sleeps again.
 */
class DeepSleepSensor : public Device {
private:
    SoilMoistureSensor soilSensor;

public:
    /**
     * @brief Constructs the deep sleep sensor device.
     * @param soilSensorPin GPIO pin connected to the soil moisture sensor analog output.
     * @param deepSleepIntervalInMilliseconds Interval between deep sleep cycles in milliseconds.
     * @param timerChannel Hardware timer group index. Defaults to 0.
     */
    DeepSleepSensor(
        int soilSensorPin,
        unsigned long deepSleepIntervalInMilliseconds = DEFAULT_DEEP_SLEEP_INTERVAL_IN_MILLISECONDS,
        uint8_t timerChannel = DEFAULT_TIMER_CHANNEL)
        : Device(deepSleepIntervalInMilliseconds, timerChannel),
          soilSensor(soilSensorPin, this)
    {
        // Configure deep sleep mode for battery operation
        setPowerMode(PowerMode::DeepSleep);

        // Increment boot count (survives deep sleep via RTC_DATA_ATTR)
        ++rtcBootCount;

        // Report how the device woke up
        reportWakeReason();

        // Announce boot
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "[DeepSleepSensor] Boot #%lu, Sequence: %lu", rtcBootCount, rtcSequenceCounter);
        Serial.println(buffer);

        // Launch internal background task workers with a small queue for battery operation
        initializeAsynchronousEngine(SAMPLER_QUEUE_LENGTH);

        // Register the soil moisture sensor
        appendSensorToScheduler(&soilSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    }

    /**
     * @brief Inversion of Control interface contract handler.
     * Intercepts sensor data-ready events to log the moisture level.
     * @param event The triggered event context payload.
     */
    void on(Event event) override {
        if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "[DeepSleepSensor] Moisture: %d", soilSensor.getMoistureLevel());
            Serial.println(buffer);
        }
    }
};

static DeepSleepSensor* deepSleepApp = nullptr;

/**
 * @brief Application entry point.
 * Initializes Serial, creates the device, and starts the deep sleep cycle.
 */
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    deepSleepApp = new DeepSleepSensor(SOIL_MOISTURE_PIN);
}

/**
 * @brief Empty loop — the framework handles all timing and deep sleep cycles.
 */
void loop() {
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
