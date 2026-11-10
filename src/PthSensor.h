#ifndef PTH_SENSOR_H
#define PTH_SENSOR_H

/**
 * @file PthSensor.h
 * @brief Declares the BME280-compatible pressure, temperature, and humidity sensor adapter.
 * @ingroup sensors_module
 */

#include "Sensor.h"
#include <Arduino.h>
#include <Wire.h>
#include <cstdint>

/**
 * @brief Event-driven adapter for a BME280-compatible environmental sensor.
 *
 * The sensor provides factory-calibrated readings for temperature, humidity,
 * and atmospheric pressure via I²C. On each @ref Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER
 * request, the adapter reads all three values, applies Bosch compensation formulas,
 * and emits @ref Sensor::DATA_READ_EVENT_IDENTIFIER when any value changes beyond
 * the configured epsilon thresholds.
 *
 * Initialization reads the factory calibration coefficients from non-volatile
 * registers. Check isInitialized() before relying on readings.
 *
 * @note The BME280 ADDR pin selects @ref DEFAULT_I2C_ADDRESS (low) or
 *       @ref ALTERNATE_I2C_ADDRESS (high). The constructor calls Wire.begin(),
 *       which initializes the shared Arduino I2C interface.
 * @see Sensor
 */
class PthSensor : public Sensor {
private:
    uint8_t i2cAddress; ///< Configured BME280 7-bit I2C address.

    // BME280 factory calibration coefficients
    uint16_t digT1; int16_t digT2; int16_t digT3;
    uint16_t digP1; int16_t digP2; int16_t digP3; int16_t digP4;
    int16_t digP5; int16_t digP6; int16_t digP7; int16_t digP8;
    int16_t digP9;
    uint8_t digH1; int16_t digH2; uint8_t digH3; int16_t digH4;
    int16_t digH5; uint8_t digH6;
    
    /// Internal temperature for compensation (5 Q8 format).
    int32_t tFine;

protected:
    /// Cached readings from the most recent successful sample.
    float cachedTemperatureInCelsius;
    float cachedHumidityInPercentage;
    float cachedPressureInHectopascals;
    
    /// Last-notified readings for change detection.
    float lastNotifiedTemperatureInCelsius;
    float lastNotifiedHumidityInPercentage;
    float lastNotifiedPressureInHectopascals;
    
    bool initialized; ///< Whether calibration data was read successfully.

    /// Epsilon thresholds for change detection.
    static constexpr float TEMPERATURE_CHANGE_EPSILON = 0.1f;
    static constexpr float HUMIDITY_CHANGE_EPSILON = 0.1f;
    static constexpr float PRESSURE_CHANGE_EPSILON = 0.1f;
    
    /// Initial values for cached fields.
    static constexpr float INITIAL_CACHED_TEMPERATURE_IN_CELSIUS = 0.0f;
    static constexpr float INITIAL_CACHED_HUMIDITY_IN_PERCENTAGE = 0.0f;
    static constexpr float INITIAL_CACHED_PRESSURE_IN_HECTOPASCALS = 0.0f;
    
    /// Initial values for last-notified fields (sentinel to ensure first reading emits event).
    static constexpr float INITIAL_LAST_NOTIFIED_TEMPERATURE_IN_CELSIUS = -1.0f;
    static constexpr float INITIAL_LAST_NOTIFIED_HUMIDITY_IN_PERCENTAGE = -1.0f;
    static constexpr float INITIAL_LAST_NOTIFIED_PRESSURE_IN_HECTOPASCALS = -1.0f;
    
    /// Event identifier used internally to prevent propagation.
    static constexpr int PROPAGATION_SUPPRESSION_IDENTIFIER = -1;

    /// BME280 I2C register addresses.
    static constexpr uint8_t BME280_REGISTER_TEMP_PRESS_X4 = 0xF4;
    static constexpr uint8_t BME280_REGISTER_HUMIDITY = 0xF5;
    static constexpr uint8_t BME280_REGISTER_DATA = 0xF7;
    static constexpr uint8_t BME280_REGISTER_CALIBRATION_START = 0x88;
    static constexpr uint8_t BME280_REGISTER_CALIBRATION_HUMIDITY = 0xE1;
    static constexpr uint8_t BME280_REGISTER_CHIP_ID = 0xD0;

    /// BME280 configuration constants.
    static constexpr uint8_t BME280_CHIP_ID_VALUE = 0x60;
    static constexpr uint8_t BME280_OSRS_TEMP_16 = 0x05;
    static constexpr uint8_t BME280_OSRS_PRESS_16 = 0x05;
    static constexpr uint8_t BME280_OSRS_HUMID_16 = 0x05;
    static constexpr uint8_t BME280_STANDBY_0_5_MS = 0x00;
    static constexpr uint8_t BME280_FILTER_COEFF_16 = 0x05;
    static constexpr uint8_t BME280_MODE_NORMAL = 0x03;

    /// BME280 data register layout.
    static constexpr uint8_t BME280_CALIBRATION_TEMP_PRESSURE_BYTE_COUNT = 26;
    static constexpr uint8_t BME280_CALIBRATION_HUMIDITY_BYTE_COUNT = 7;
    static constexpr uint8_t BME280_RAW_DATA_BYTE_COUNT = 8;
    static constexpr uint8_t BME280_RAW_DATA_SKIP_BYTE_COUNT = 3;
    static constexpr uint8_t BME280_RAW_DATA_SHIFT_MSB = 12;
    static constexpr uint8_t BME280_RAW_DATA_SHIFT_LSB = 4;

    /// Barometric altitude formula constants.
    static constexpr float ALTITUDE_SCALE_HEIGHT_IN_METERS = 44330.0f;
    static constexpr float ALTITUDE_BAROMETRIC_EXPONENT = 0.1903f;

    /// BME280 temperature compensation constants.
    static constexpr float BME280_TEMP_COMPENSATION_SCALE_14 = 16384.0f;
    static constexpr float BME280_TEMP_COMPENSATION_SCALE_10 = 1024.0f;
    static constexpr float BME280_TEMP_COMPENSATION_OFFSET = 24576.0f;
    static constexpr float BME280_TEMP_COMPENSATION_SCALE_16 = 65536.0f;
    static constexpr float BME280_TEMP_TFINE_SCALE_8 = 256.0f;

    /// BME280 pressure compensation constants.
    static constexpr float BME280_PRESS_COMPENSATION_DIVISOR_2 = 2.0f;
    static constexpr float BME280_PRESS_COMPENSATION_OFFSET = 64000.0f;
    static constexpr float BME280_PRESS_COMPENSATION_SCALE_15 = 32768.0f;
    static constexpr float BME280_PRESS_COMPENSATION_DIVISOR_4 = 4.0f;
    static constexpr float BME280_PRESS_COMPENSATION_SCALE_17 = 131072.0f;
    static constexpr float BME280_PRESS_COMPENSATION_SCALE_19 = 524288.0f;
    static constexpr float BME280_PRESS_RAW_OFFSET = 1048576.0f;
    static constexpr float BME280_PRESS_COMPENSATION_SCALE_12 = 4096.0f;
    static constexpr float BME280_PRESS_SCALING_FACTOR = 6250.0f;
    static constexpr float BME280_PRESS_OVERFLOW_PROTECTION = 2147483648.0f;
    static constexpr float BME280_PRESS_DIVISOR_16 = 16.0f;
    static constexpr float BME280_PRESS_TO_HECTOPASCALS = 100.0f;

    /// BME280 humidity compensation constants.
    static constexpr float BME280_HUMID_COMPENSATION_OFFSET = 76800.0f;
    static constexpr float BME280_HUMID_COMPENSATION_SCALE_6 = 64.0f;
    static constexpr float BME280_HUMID_COMPENSATION_SCALE_14 = 16384.0f;
    static constexpr float BME280_HUMID_COMPENSATION_SCALE_16 = 65536.0f;
    static constexpr float BME280_HUMID_COMPENSATION_SCALE_26 = 67108864.0f;
    static constexpr float BME280_HUMID_COMPENSATION_SCALE_19 = 524288.0f;
    static constexpr float BME280_HUMIDITY_MAX_PERCENTAGE = 100.0f;
    static constexpr float BME280_HUMIDITY_MIN_PERCENTAGE = 0.0f;

    /**
     * @brief Reads a 16-bit little-endian value from the sensor.
     * @param registerAddress BME280 register address.
     * @return 16-bit value read from the sensor.
     */
    [[nodiscard]] int16_t readRegister16(uint8_t registerAddress) const;

    /**
     * @brief Reads the factory calibration coefficients.
     * @return @c true if all calibration registers were read successfully.
     */
    [[nodiscard]] bool readCalibrationData();

    /**
     * @brief Reads raw temperature, pressure, and humidity data.
     * @param rawTemperature Output receiving raw temperature (20-bit).
     * @param rawPressure Output receiving raw pressure (20-bit).
     * @param rawHumidity Output receiving raw humidity (16-bit).
     * @return @c true if all 8 data bytes were read successfully.
     */
    bool readRawData(int32_t& rawTemperature, int32_t& rawPressure, uint16_t& rawHumidity) const;

    /**
     * @brief Compensates raw temperature reading.
     * @param rawTemperature Raw 20-bit temperature value.
     * @return Compensated temperature in degrees Celsius.
     */
    float compensateTemperature(int32_t rawTemperature);

    /**
     * @brief Compensates raw pressure reading.
     * @param rawPressure Raw 20-bit pressure value.
     * @return Compensated pressure in hectopascals.
     */
    float compensatePressure(int32_t rawPressure) const;

    /**
     * @brief Compensates raw humidity reading.
     * @param rawHumidity Raw 16-bit humidity value.
     * @return Compensated humidity in percentage.
     */
    float compensateHumidity(uint16_t rawHumidity) const;

protected:
    /**
     * @brief Handles measurement requests and emits data-read events.
     * @param event Request event to inspect and, when applicable, replace with a
     *              data-read event or the propagation-suppression identifier.
     * @details Non-measurement events are left unchanged. A measurement request is
     *          suppressed while uninitialized, on read failure, or when no value
     *          has changed beyond the epsilon thresholds.
     * @runs_on sampler
     */
    void processEvent(Event& event) override;

    [[nodiscard]] uint32_t getDefaultSamplingIntervalInMilliseconds() const override;
    [[nodiscard]] uint32_t getMinimalSamplingIntervalInMilliseconds() const override;

public:
    /// Default BME280 address when SDO is low.
    static constexpr uint8_t DEFAULT_I2C_ADDRESS = 0x76;
    /// Alternate BME280 address when SDO is high.
    static constexpr uint8_t ALTERNATE_I2C_ADDRESS = 0x77;

    /// Default sampling interval in milliseconds.
    static constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS = 2000;
    /// Minimum sampling interval in milliseconds.
    static constexpr uint32_t MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 250;

    /**
     * @brief Constructs and initializes a BME280-compatible sensor adapter.
     * @param signalGpioPin Framework source identifier attached to emitted events;
     *                       this sensor communicates over I2C and does not use it
     *                       as a GPIO pin.
     * @param i2cAddress Sensor's 7-bit I2C address; use
     *                   @ref DEFAULT_I2C_ADDRESS or @ref ALTERNATE_I2C_ADDRESS.
     * @param parentHandler Optional handler that receives emitted events.
     * @note Calls Wire.begin() and attempts initialize(); inspect isInitialized()
     *       to determine whether initialization succeeded.
     */
    explicit PthSensor(int signalGpioPin, uint8_t i2cAddress = DEFAULT_I2C_ADDRESS,
                       EventHandler* parentHandler = nullptr);

    /// Releases the adapter; the shared Wire interface is not shut down.
    ~PthSensor() override = default;

    /**
     * @brief Retries the BME280 initialization sequence.
     * @return @c true if the chip ID and calibration data were read successfully.
     * @details Reads the chip ID to verify connectivity, then reads all calibration
     *          coefficients. On success, configures the sensor for normal mode.
     */
    bool initialize();

    /**
     * @brief Reports the result of the most recent initialization attempt.
     * @return @c true if calibration data was read successfully.
     */
    [[nodiscard]] bool isInitialized() const;

    /**
     * @brief Returns the temperature from the most recent successful sample.
     * @return Temperature in degrees Celsius.
     * @warning Check isInitialized() first; before initialization this returns 0.
     */
    [[nodiscard]] float getTemperatureInCelsius() const;

    /**
     * @brief Returns the humidity from the most recent successful sample.
     * @return Relative humidity in percentage.
     * @warning Check isInitialized() first; before initialization this returns 0.
     */
    [[nodiscard]] float getHumidityInPercentage() const;

    /**
     * @brief Returns the pressure from the most recent successful sample.
     * @return Atmospheric pressure in hectopascals.
     * @warning Check isInitialized() first; before initialization this returns 0.
     */
    [[nodiscard]] float getPressureInHectopascals() const;

    /**
     * @brief Computes altitude from the current pressure reading.
     * @param seaLevelPressureInHectopascals Local sea-level pressure (QNH) for
     *                                        reference; use 1013.25 hPa as the
     *                                        standard atmosphere default.
     * @return Approximate altitude in meters above mean sea level.
     * @details Uses the barometric formula:
     *          altitude = 44330 × (1 − (pressure / seaLevelPressure)^0.1903).
     * @warning Check isInitialized() first; before initialization this returns 0.
     */
    [[nodiscard]] float getAltitudeInMeters(float seaLevelPressureInHectopascals) const;
};

#endif // PTH_SENSOR_H
