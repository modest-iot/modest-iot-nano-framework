/**
 * @file WeatherStation.ino
 * @brief Battery-powered outdoor weather station with deep sleep for months of operation.
 *
 * Monitors air temperature, humidity, atmospheric pressure, and light intensity.
 * Uses ESP32 deep sleep to reduce power consumption from ~100mA active to ~10µA sleeping,
 * enabling months of operation on a coin cell battery. NTP time synchronization provides
 * absolute Unix timestamps for server-side data correlation.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `Device` base class with `PowerMode::DeepSleep` for ESP32 deep sleep
 * - `DhtSensor` (digital) for temperature/humidity
 * - `PthSensor` (I2C BME280) for atmospheric pressure
 * - `DigitalLightIntensitySensor` (I2C) for ambient light
 * - `TimeSynchronizationDriver` with NTP and RTC save/restore for deep sleep persistence
 * - `RTC_DATA_ATTR` for variables that survive deep sleep reboot
 * - `reportWakeReason()` for boot cycle tracking
 * - Cross-task event routing via `on(Event)` handler
 * @date 2026-11-10
 * @version 1.0
 */

#include <Arduino.h>
#include <ModestIoT.h>

/** @name Default Configuration Constants */
//@{
// Static constants for constructor defaults, allowing use before class declaration.

/** @brief Default deep sleep interval in milliseconds (5 minutes). */
static const unsigned long DEFAULT_DEEP_SLEEP_INTERVAL_IN_MILLISECONDS = 300000UL;

/** @brief Default hardware timer channel index. */
static const uint8_t DEFAULT_TIMER_CHANNEL = 0;

/** @brief Sampler event queue length for battery operation. */
static const int SAMPLER_QUEUE_LENGTH = 5;

// Pin definitions
static const int DHT_SENSOR_PIN = 4;

// Timing constants
static const unsigned long SERIAL_BAUD_RATE = 115200;
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;

// RTC memory variables that survive deep sleep
RTC_DATA_ATTR uint32_t rtcBootCount = 0; ///< Boot cycle counter
RTC_DATA_ATTR uint32_t rtcSequenceCounter = 0; ///< Telemetry sequence counter

/** @name NTP Configuration */
//@{

/** @brief Timezone identifier for NTP synchronization. */
static const char* WEATHER_STATION_TIMEZONE = "UTC";

/** @brief Primary NTP server for time synchronization. */
static const char* WEATHER_STATION_NTP_SERVER = "pool.ntp.org";

//@}

/**
 * @brief Battery-Powered Weather Station Device.
 * Demonstrates ESP32 deep sleep with periodic weather sampling.
 * The device wakes, reads temperature/humidity/light, prints the results, and sleeps again.
 */
class WeatherStation : public Device {
private:
    DhtSensor dhtSensor;
    PthSensor pthSensor;
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
          dhtSensor(dhtSensorPin, DhtSensor::SENSOR_MODEL_DHT22, this),
          pthSensor(0, PthSensor::DEFAULT_I2C_ADDRESS, this),
          lightSensor(0, DigitalLightIntensitySensor::DEFAULT_I2C_ADDRESS,
                      DigitalLightIntensitySensor::DEFAULT_LIGHT_DETECTED_THRESHOLD_IN_LUX, this)
    {
        // Configure deep sleep mode for maximum energy saving (~10µA vs ~100mA active)
        setPowerMode(PowerMode::DeepSleep);

        // Increment boot count (survives deep sleep via RTC_DATA_ATTR)
        ++rtcBootCount;

        // Report how the device woke up
        Device::reportWakeReason();

        // Restore time from RTC memory after deep sleep wake
        TimeSynchronizationDriver::restoreFromRtc();

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

        // Register the PthSensor for atmospheric pressure
        appendSensorToScheduler(&pthSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);

        // Register the light sensor for ambient light detection
        appendSensorToScheduler(&lightSensor, DigitalLightIntensitySensor::LIGHT_DETECTED_EVENT_IDENTIFIER);
    }

    /**
     * @brief Handles sensor data-ready events to log weather readings.
     * After this handler returns, the device enters deep sleep (~10µA) until
     * the next sampling interval, achieving >99.9% duty cycle energy savings.
     * @param event The triggered event context payload.
     * @runs_on{sampler}
     */
    void on(Event event) override {
        if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
            char buffer[128];
            snprintf(buffer, sizeof(buffer),
                     "[WeatherStation] Temp: %.1f°C, Humidity: %.1f%%, Pressure: %.1f hPa, Light: %.1f lux",
                     dhtSensor.getTemperatureInCelsius(),
                     dhtSensor.getRelativeHumidityInPercentage(),
                     pthSensor.getPressureInHectopascals(),
                     lightSensor.getIlluminanceInLux());
            Serial.println(buffer);
            Serial.println("[WeatherStation] Entering deep sleep (~10µA) until next sample...");
        }
    }
};

static WeatherStation* weatherStationApplication = nullptr;

/**
 * @brief Application entry point.
 * Initializes Serial, creates the device, and starts the deep sleep cycle.
 */
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    // Initialize NTP time synchronization for absolute timestamps
    TimeSynchronizationDriver::initialize(WEATHER_STATION_TIMEZONE, WEATHER_STATION_NTP_SERVER);

    weatherStationApplication = new WeatherStation(DHT_SENSOR_PIN);
}

/**
 * @brief Empty loop — the framework handles all timing and deep sleep cycles.
 */
void loop() {
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
