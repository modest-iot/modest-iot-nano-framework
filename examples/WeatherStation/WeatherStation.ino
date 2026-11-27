/**
 * @file WeatherStation.ino
 * @brief Battery-powered outdoor weather station with adaptive power management.
 *
 * Monitors air temperature, humidity, atmospheric pressure, and light intensity.
 * Starts in Periodic mode for normal operation, and automatically switches to
 * DeepSleep mode when low battery is detected. The low-battery flag persists
 * across reboots via RTC memory, so the device remains in DeepSleep mode until
 * the battery is replaced. NTP time synchronization provides absolute Unix
 * timestamps for server-side data correlation. PowerManager monitors the battery
 * voltage via ADC and triggers the power mode transition.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `Device` base class with adaptive `PowerMode` switching (Periodic → DeepSleep)
 * - `DhtSensor` (digital) for temperature/humidity
 * - `PthSensor` (I2C BME280) for atmospheric pressure
 * - `DigitalLightIntensitySensor` (I2C) for ambient light
 * - `TimeSynchronizationDriver` with NTP and RTC save/restore for deep sleep persistence
 * - `PowerManager` for battery voltage sensing, level calculation, and low-battery alerts
 * - `Device::setFirmwareVersion()` for fleet management in telemetry
 * - `RTC_DATA_ATTR` for variables that survive deep sleep reboot
 * - `reportWakeReason()` for boot cycle tracking
 * - Cross-task event routing via `on(Event)` handler
 * @date 2026-11-15
 * @version 1.2
 */

#include <Arduino.h>
#include <ModestIoT.h>

/** @name Hardware Configuration */
//@{
/** @brief GPIO pin for the DHT22 sensor data line. */
static const int DHT_SENSOR_PIN = 4;
/** @brief ADC pin for the battery voltage divider circuit. */
static const int BATTERY_ADC_PIN = 34;
//@} // Hardware Configuration

/** @name Timing Configuration */
//@{
/** @brief Default deep sleep interval in milliseconds (5 minutes). */
static const unsigned long DEFAULT_DEEP_SLEEP_INTERVAL_IN_MILLISECONDS = 300000UL;
/** @brief Delay in the main loop when no work is pending. */
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;
//@} // Timing Configuration

/** @name Framework Configuration */
//@{
/** @brief Default hardware timer channel index. */
static const uint8_t DEFAULT_TIMER_CHANNEL = 0;
/** @brief Sampler event queue length for battery operation. */
static const int SAMPLER_QUEUE_LENGTH = 5;
/** @brief Serial communication baud rate. */
static const unsigned long SERIAL_BAUD_RATE = 115200;
//@} // Framework Configuration

/** @name NTP Configuration */
//@{
/** @brief Timezone identifier for NTP synchronization. */
static const char* WEATHER_STATION_TIMEZONE = "UTC";
/** @brief Primary NTP server for time synchronization. */
static const char* WEATHER_STATION_NTP_SERVER = "pool.ntp.org";
//@} // NTP Configuration

/** @brief Shared I2C bus configuration for the BME280 and BH1750 sensors. */
static const I2cBusConfiguration WEATHER_STATION_I2C_CONFIGURATION{-1, -1, 100000};

/** @name PowerManager Configuration */
//@{
/** @brief Voltage divider ratio for the battery monitoring circuit (2:1 divider). */
static const float BATTERY_DIVIDER_RATIO = 2.0f;
//@} // PowerManager Configuration

/** @name Change-Only Mode Configuration */
//@{
/** @brief Temperature deadband in °C; suppresses telemetry when change is below this threshold. */
static const float TEMPERATURE_DEADBAND_CELSIUS = 0.5f;
//@} // Change-Only Mode Configuration

/** @name Serial Output Configuration */
//@{
/** @brief Buffer size for formatted serial output messages. */
static const size_t SERIAL_OUTPUT_BUFFER_SIZE = 128;
/** @brief Log prefix for all WeatherStation serial output. */
static const char* WEATHER_STATION_LOG_PREFIX = "[WeatherStation]";
//@} // Serial Output Configuration

/** @name RTC Memory Variables */
//@{
/** @brief Boot cycle counter that survives deep sleep. */
RTC_DATA_ATTR uint32_t rtcBootCount = 0;
/** @brief Telemetry sequence counter that survives deep sleep. */
RTC_DATA_ATTR uint32_t rtcSequenceCounter = 0;
/** @brief Flag set when low battery is detected; persists across deep sleep reboots. */
RTC_DATA_ATTR bool rtcLowBatteryDetected = false;
//@} // RTC Memory Variables

/**
 * @brief Battery-Powered Weather Station Device.
 * Demonstrates adaptive power management: starts in Periodic mode for normal
 * operation, and automatically switches to DeepSleep mode when low battery
 * is detected. The low-battery flag persists across reboots via RTC memory,
 * so the device remains in DeepSleep mode until the battery is replaced.
 * All initialization and behavior is owned by this class; the sketch's setup()
 * is only responsible for instance creation and calling start().
 */
class WeatherStation : public Device {
public:
    ~WeatherStation() override { stopAsynchronousEngine(); }

private:
    ArduinoI2cBus i2cBus;
    DhtSensor dhtSensor;
    PthSensor pthSensor;
    DigitalLightIntensitySensor lightSensor;

    /**
     * @brief Announces boot status with energy saving metrics.
     */
    void announceBoot() {
        char buffer[SERIAL_OUTPUT_BUFFER_SIZE];
        snprintf(buffer, sizeof(buffer),
                 "%s Boot #%lu | Deep Sleep: ~10µA (10,000x energy saving)",
                 WEATHER_STATION_LOG_PREFIX, rtcBootCount);
        Serial.println(buffer);
    }

    /**
     * @brief Displays the current weather readings on Serial.
     */
    void displayWeatherReadings() {
        char buffer[SERIAL_OUTPUT_BUFFER_SIZE];
        snprintf(buffer, sizeof(buffer),
                 "%s Temp: %.1f°C, Humidity: %.1f%%, Pressure: %.1f hPa, Light: %.1f lux",
                 WEATHER_STATION_LOG_PREFIX,
                 dhtSensor.getTemperatureInCelsius(),
                 dhtSensor.getRelativeHumidityInPercentage(),
                 pthSensor.getPressureInHectopascals(),
                 lightSensor.getIlluminanceInLux());
        Serial.println(buffer);
    }

    /**
     * @brief Displays the current battery status on Serial.
     * Checks for low battery and switches to DeepSleep mode if needed.
     * The low-battery flag persists across reboots via RTC memory,
     * so the device remains in DeepSleep mode until the battery is replaced.
     */
    void displayBatteryStatus() {
        uint32_t batteryVoltage = PowerManager::getBatteryVoltageInMillivolts();
        float batteryLevel = PowerManager::getBatteryLevelInPercentage();
        char buffer[SERIAL_OUTPUT_BUFFER_SIZE];
        snprintf(buffer, sizeof(buffer),
                 "%s Battery: %lu mV (%.1f%%)",
                 WEATHER_STATION_LOG_PREFIX, batteryVoltage, batteryLevel);
        Serial.println(buffer);

        if (PowerManager::isLowBattery()) {
            rtcLowBatteryDetected = true;
            Serial.printf("%s WARNING: Low battery detected!\n",
                          WEATHER_STATION_LOG_PREFIX);

            if (getPowerMode() != PowerMode::DeepSleep) {
                setPowerMode(PowerMode::DeepSleep);
                Serial.printf("%s Switched to DeepSleep mode to conserve power.\n",
                              WEATHER_STATION_LOG_PREFIX);
            }
        }
    }

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
          i2cBus(),
          dhtSensor(dhtSensorPin, DhtSensor::SENSOR_MODEL_DHT22, this),
          pthSensor(i2cBus, 0, PthSensor::DEFAULT_I2C_ADDRESS, this),
          lightSensor(i2cBus, 0, DigitalLightIntensitySensor::DEFAULT_I2C_ADDRESS,
                      DigitalLightIntensitySensor::DEFAULT_LIGHT_DETECTED_THRESHOLD_IN_LUX, this)
    {
        // Start in Periodic mode; switch to DeepSleep when low battery is detected
        setPowerMode(PowerMode::Periodic);
    }

    /**
     * @brief Handles sensor data-ready events to log weather readings.
     * After this handler returns, the device enters the current power mode
     * (Periodic or DeepSleep) until the next sampling interval.
     * @param event The triggered event context payload.
     * @runs_on{sampler}
     */
    void on(Event event) override {
        if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
            displayWeatherReadings();
            displayBatteryStatus();
            if (getPowerMode() == PowerMode::DeepSleep) {
                Serial.printf("%s Entering deep sleep (~10µA) until next sample...\n",
                              WEATHER_STATION_LOG_PREFIX);
            } else {
                Serial.printf("%s Waiting for next sample (Periodic mode)...\n",
                              WEATHER_STATION_LOG_PREFIX);
            }
        }
    }

    /**
     * @brief Initializes all subsystems and starts the deep sleep cycle.
     * This method encapsulates all setup logic so the sketch's setup()
     * is only responsible for instance creation.
     */
    bool start() {
        const I2cStatus busStatus = i2cBus.start(WEATHER_STATION_I2C_CONFIGURATION);
        if (busStatus != I2cStatus::Ok) {
            Serial.printf("%s I2C bus startup failed (status %u).\n",
                          WEATHER_STATION_LOG_PREFIX, static_cast<unsigned int>(busStatus));
            return false;
        }
        if (!pthSensor.start()) {
            Serial.printf("%s BME280 startup failed (status %u).\n",
                          WEATHER_STATION_LOG_PREFIX,
                          static_cast<unsigned int>(pthSensor.getLastI2cResult().status));
            return false;
        }
        if (!lightSensor.start()) {
            Serial.printf("%s BH1750 startup failed (status %u).\n",
                          WEATHER_STATION_LOG_PREFIX,
                          static_cast<unsigned int>(lightSensor.getLastI2cResult().status));
            return false;
        }

        // Increment boot count (survives deep sleep via RTC_DATA_ATTR)
        ++rtcBootCount;

        // Report how the device woke up
        Device::reportWakeReason();

        // Restore time from RTC memory after deep sleep wake
        TimeSynchronizationDriver::restoreFromRtc();

        // Announce boot with energy saving status
        announceBoot();

        // Register sensors
        // IMPORTANT: Registration must occur before initializeAsynchronousEngine(), because
        // the engine start guard rejects any registration after the engine is running.
        // Register the DHT sensor for temperature and humidity with change-only mode.
        // Deadband of 0.5°C suppresses telemetry when temperature is stable, reducing
        // Wi-Fi transmissions by ~80% for slowly changing sensors.
        appendSensorToScheduler(&dhtSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER,
                                TEMPERATURE_DEADBAND_CELSIUS);

        // Register the PthSensor for atmospheric pressure
        appendSensorToScheduler(&pthSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);

        // Register the light sensor for ambient light detection
        appendSensorToScheduler(&lightSensor, DigitalLightIntensitySensor::LIGHT_DETECTED_EVENT_IDENTIFIER);

        // Launch internal background task workers with a small queue for battery operation
        return initializeAsynchronousEngine(SAMPLER_QUEUE_LENGTH);
    }

    /**
     * @brief Prepares the device for first boot or subsequent wake-ups.
     * Initializes framework-level services (firmware version, NTP, PowerManager)
     * and then starts the device behavior.
     */
    bool initializeAndStart() {
        // Set firmware version for fleet management in telemetry
        Device::setFirmwareVersion("WS-3200 FW 1.1.0");

        // Initialize NTP time synchronization for absolute timestamps
        TimeSynchronizationDriver::initialize(WEATHER_STATION_TIMEZONE, WEATHER_STATION_NTP_SERVER);

        // Initialize battery monitoring on ADC pin 34 with 2:1 voltage divider
        PowerManager::begin(BATTERY_ADC_PIN, BATTERY_DIVIDER_RATIO);

        // Restore DeepSleep mode if low battery was detected in a previous cycle
        if (rtcLowBatteryDetected) {
            setPowerMode(PowerMode::DeepSleep);
            Serial.printf("%s Low battery detected previously — running in DeepSleep mode.\n",
                          WEATHER_STATION_LOG_PREFIX);
        }

        // Start the device behavior
        return start();
    }
};

static WeatherStation* weatherStationApplication = nullptr;

/**
 * @brief Application entry point.
 * Initializes Serial and creates the device; all further initialization
 * is handled by the WeatherStation class itself.
 */
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    weatherStationApplication = new WeatherStation(DHT_SENSOR_PIN);
    if (!weatherStationApplication->initializeAndStart()) {
        Serial.println("[WeatherStation] Device startup failed; asynchronous sampling was not started.");
    }
}

/**
 * @brief Empty loop — the framework handles all timing and deep sleep cycles.
 */
void loop() {
    // The master loop is completely clean and empty.
    // The framework handles all timing, sensor sampling, and telemetry publishing asynchronously.
    // vTaskDelay clarifies that this loop yields to FreeRTOS — the device is loopless.
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
