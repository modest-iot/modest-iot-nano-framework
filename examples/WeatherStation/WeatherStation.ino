/**
 * @file WeatherStation.ino
 * @brief Battery-powered weather station demonstrating ESP32 deep sleep cycles.
 * @date 2026-11-09
 * @version 1.0
 *
 * This example shows how to use PowerMode::DeepSleep for battery-powered
 * outdoor weather monitoring. The device wakes from deep sleep, reads air
 * temperature, humidity, and light intensity, reports the readings via Serial,
 * and returns to deep sleep.
 *
 * **Energy Saving**: Deep sleep reduces power consumption from ~100mA (active)
 * to ~10µA (sleeping), a 10,000x reduction. With 5-minute sleep intervals,
 * the device spends >99.9% of its time asleep, enabling months of operation
 * on a single coin cell battery. Unlike light sleep (~100mA with Wi-Fi),
 * deep sleep shuts down the CPU, Wi-Fi, and most peripherals.
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
static const int DHT_SENSOR_PIN = 4;

// Timing constants
static const unsigned long SERIAL_BAUD_RATE = 115200;
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;

// RTC memory variables that survive deep sleep
RTC_DATA_ATTR uint32_t rtcBootCount = 0; ///< Boot cycle counter
RTC_DATA_ATTR uint32_t rtcSequenceCounter = 0; ///< Telemetry sequence counter

/**
 * @brief Battery-Powered Weather Station Device.
 * Demonstrates ESP32 deep sleep with periodic weather sampling.
 * The device wakes, reads temperature/humidity/light, prints the results, and sleeps again.
 */
class WeatherStation : public Device {
private:
    DhtSensor dhtSensor;
    DigitalLightIntensitySensor lightSensor;

public:
    /**
     * @brief Constructs the weather station device.
     * @param dhtSensorPin GPIO pin connected to the DHT sensor data output.
     * @param deepSleepIntervalInMilliseconds Interval between deep sleep cycles in milliseconds.
     * @param timerChannel Hardware timer group index. Defaults to 0.
     */
    WeatherStation(
        int dhtSensorPin,
        unsigned long deepSleepIntervalInMilliseconds = DEFAULT_DEEP_SLEEP_INTERVAL_IN_MILLISECONDS,
        uint8_t timerChannel = DEFAULT_TIMER_CHANNEL)
        : Device(deepSleepIntervalInMilliseconds, timerChannel),
          dhtSensor(dhtSensorPin, DHT22, this),
          lightSensor(0, DigitalLightIntensitySensor::DEFAULT_I2C_ADDRESS,
                      DigitalLightIntensitySensor::DEFAULT_LIGHT_DETECTED_THRESHOLD_IN_LUX, this)
    {
        // Configure deep sleep mode for maximum energy saving (~10µA vs ~100mA active)
        setPowerMode(PowerMode::DeepSleep);

        // Increment boot count (survives deep sleep via RTC_DATA_ATTR)
        ++rtcBootCount;

        // Report how the device woke up
        reportWakeReason();

        // Announce boot with energy saving status
        char buffer[128];
        snprintf(buffer, sizeof(buffer),
                 "[WeatherStation] Boot #%lu | Deep Sleep: ~10µA (10,000x energy saving)",
                 rtcBootCount);
        Serial.println(buffer);

        // Launch internal background task workers with a small queue for battery operation
        initializeAsynchronousEngine(SAMPLER_QUEUE_LENGTH);

        // Register the DHT sensor for temperature and humidity
        appendSensorToScheduler(&dhtSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);

        // Register the light sensor for ambient light detection
        appendSensorToScheduler(&lightSensor, DigitalLightIntensitySensor::LIGHT_DETECTED_EVENT_IDENTIFIER);
    }

    /**
     * @brief Inversion of Control interface contract handler.
     * Intercepts sensor data-ready events to log weather readings.
     * After this handler returns, the device enters deep sleep (~10µA) until
     * the next sampling interval, achieving >99.9% duty cycle energy savings.
     * @param event The triggered event context payload.
     */
    void on(Event event) override {
        if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
            char buffer[128];
            snprintf(buffer, sizeof(buffer),
                     "[WeatherStation] Temp: %.1f°C, Humidity: %.1f%%, Light: %.1f lux",
                     dhtSensor.getTemperatureInCelsius(),
                     dhtSensor.getRelativeHumidityInPercentage(),
                     lightSensor.getIlluminanceInLux());
            Serial.println(buffer);
            Serial.println("[WeatherStation] Entering deep sleep (~10µA) until next sample...");
        }
    }
};

static WeatherStation* weatherStationApp = nullptr;

/**
 * @brief Application entry point.
 * Initializes Serial, creates the device, and starts the deep sleep cycle.
 */
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    weatherStationApp = new WeatherStation(DHT_SENSOR_PIN);
}

/**
 * @brief Empty loop — the framework handles all timing and deep sleep cycles.
 */
void loop() {
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
