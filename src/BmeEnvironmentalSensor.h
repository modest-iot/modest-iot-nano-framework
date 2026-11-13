#ifndef BME_ENVIRONMENTAL_SENSOR_H
#define BME_ENVIRONMENTAL_SENSOR_H

/**
 * @file BmeEnvironmentalSensor.h
 * @brief Declares the abstract base class for Bosch BME280/BME680 environmental sensors.
 * @ingroup sensors_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @license Apache-2.0
 */

#include "Sensor.h"
#include <Arduino.h>
#include <Wire.h>
#include <cstdint>

/**
 * @brief Abstract base class for Bosch BME280/BME680 environmental sensors.
 *
 * Encapsulates the shared Bosch compensation formulas for temperature, humidity,
 * and pressure, as well as the I²C calibration data reading and register access
 * patterns common to both BME280 and BME680 chips.
 *
 * Subclasses provide chip-specific initialization (chip ID, mode configuration),
 * raw data reading (register addresses, byte counts), and optional extra values
 * (e.g., gas resistance in BME680).
 *
 * @note The constructor calls Wire.begin(), which initializes the shared Arduino
 *       I2C interface. Subclasses should call initialize() to verify connectivity
 *       and read calibration coefficients.
 * @see PthSensor
 * @see EnvironmentalGasSensor
 */
class BmeEnvironmentalSensor : public Sensor {
protected:
    /**
     * @brief Constructs a Bosch environmental sensor adapter.
     * @param signalGpioPin Framework source identifier attached to emitted events.
     * @param i2cAddress Sensor's 7-bit I2C address.
     * @param parentHandler Optional handler that receives emitted events.
     * @note Calls Wire.begin() and initializes all calibration coefficients to zero.
     */
    explicit BmeEnvironmentalSensor(int signalGpioPin, uint8_t i2cAddress,
                                    EventHandler* parentHandler = nullptr);

    uint8_t i2cAddress; ///< Configured BME 7-bit I2C address.

    /** @name BME Factory Calibration Coefficients */
    //@{
    uint16_t digT1; ///< Temperature calibration coefficient T1.
    int16_t digT2;  ///< Temperature calibration coefficient T2.
    int16_t digT3;  ///< Temperature calibration coefficient T3.
    uint16_t digP1; ///< Pressure calibration coefficient P1.
    int16_t digP2;  ///< Pressure calibration coefficient P2.
    int16_t digP3;  ///< Pressure calibration coefficient P3.
    int16_t digP4;  ///< Pressure calibration coefficient P4.
    int16_t digP5;  ///< Pressure calibration coefficient P5.
    int16_t digP6;  ///< Pressure calibration coefficient P6.
    int16_t digP7;  ///< Pressure calibration coefficient P7.
    int16_t digP8;  ///< Pressure calibration coefficient P8.
    int16_t digP9;  ///< Pressure calibration coefficient P9.
    uint8_t digH1;  ///< Humidity calibration coefficient H1.
    int16_t digH2;  ///< Humidity calibration coefficient H2.
    uint8_t digH3;  ///< Humidity calibration coefficient H3.
    int16_t digH4;  ///< Humidity calibration coefficient H4.
    int16_t digH5;  ///< Humidity calibration coefficient H5.
    uint8_t digH6;  ///< Humidity calibration coefficient H6.
    //@} // BME Factory Calibration Coefficients

    int32_t tFine; ///< Internal temperature for compensation (5 Q8 format).

    /** @name Cached Readings */
    //@{
    float cachedTemperatureInCelsius;      ///< Cached temperature in degrees Celsius.
    float cachedHumidityInPercentage;      ///< Cached relative humidity in percentage.
    float cachedPressureInHectopascals;    ///< Cached atmospheric pressure in hectopascals.
    //@} // Cached Readings

    /** @name Last-Notified Readings */
    //@{
    float lastNotifiedTemperatureInCelsius;      ///< Last-notified temperature for change detection.
    float lastNotifiedHumidityInPercentage;      ///< Last-notified humidity for change detection.
    float lastNotifiedPressureInHectopascals;    ///< Last-notified pressure for change detection.
    //@} // Last-Notified Readings

    bool initialized; ///< Whether calibration data was read successfully.

    /** @name Change Detection Epsilon Thresholds */
    //@{
    static constexpr float TEMPERATURE_CHANGE_EPSILON = 0.1f; ///< Minimum temperature change to emit event (°C).
    static constexpr float HUMIDITY_CHANGE_EPSILON = 0.1f;    ///< Minimum humidity change to emit event (%).
    static constexpr float PRESSURE_CHANGE_EPSILON = 0.1f;    ///< Minimum pressure change to emit event (hPa).
    //@} // Change Detection Epsilon Thresholds

    /** @name Initial Cached Values */
    //@{
    static constexpr float INITIAL_CACHED_TEMPERATURE_IN_CELSIUS = 0.0f; ///< Initial cached temperature.
    static constexpr float INITIAL_CACHED_HUMIDITY_IN_PERCENTAGE = 0.0f; ///< Initial cached humidity.
    static constexpr float INITIAL_CACHED_PRESSURE_IN_HECTOPASCALS = 0.0f; ///< Initial cached pressure.
    //@} // Initial Cached Values

    /** @name Initial Last-Notified Values */
    //@{
    static constexpr float INITIAL_LAST_NOTIFIED_TEMPERATURE_IN_CELSIUS = -1.0f; ///< Initial last-notified temperature sentinel.
    static constexpr float INITIAL_LAST_NOTIFIED_HUMIDITY_IN_PERCENTAGE = -1.0f; ///< Initial last-notified humidity sentinel.
    static constexpr float INITIAL_LAST_NOTIFIED_PRESSURE_IN_HECTOPASCALS = -1.0f; ///< Initial last-notified pressure sentinel.
    //@} // Initial Last-Notified Values

    static constexpr int PROPAGATION_SUPPRESSION_IDENTIFIER = -1; ///< Event identifier used internally to prevent propagation.

    /// Shared BME I2C register addresses (identical for BME280 and BME680).
    static constexpr uint8_t BME_REGISTER_TEMP_PRESS_X4 = 0xF4;
    static constexpr uint8_t BME_REGISTER_HUMIDITY = 0xF5;
    static constexpr uint8_t BME_REGISTER_DATA = 0xF7;
    static constexpr uint8_t BME_REGISTER_CALIBRATION_START = 0x88;
    static constexpr uint8_t BME_REGISTER_CALIBRATION_HUMIDITY = 0xE1;
    static constexpr uint8_t BME_REGISTER_CHIP_ID = 0xD0;

    /// Shared BME configuration constants.
    static constexpr uint8_t BME_OSRS_TEMP_16 = 0x05;
    static constexpr uint8_t BME_OSRS_PRESS_16 = 0x05;
    static constexpr uint8_t BME_OSRS_HUMID_16 = 0x05;
    static constexpr uint8_t BME_STANDBY_0_5_MS = 0x00;
    static constexpr uint8_t BME_FILTER_COEFF_16 = 0x05;

    /// Shared BME data register layout.
    static constexpr uint8_t BME_CALIBRATION_TEMP_PRESSURE_BYTE_COUNT = 26;
    static constexpr uint8_t BME_CALIBRATION_HUMIDITY_BYTE_COUNT = 7;
    static constexpr uint8_t BME_RAW_DATA_BYTE_COUNT = 8;
    static constexpr uint8_t BME_RAW_DATA_SKIP_BYTE_COUNT = 3;
    static constexpr uint8_t BME_RAW_DATA_SHIFT_MSB = 12;
    static constexpr uint8_t BME_RAW_DATA_SHIFT_LSB = 4;

    /// Barometric altitude formula constants.
    static constexpr float ALTITUDE_SCALE_HEIGHT_IN_METERS = 44330.0f;
    static constexpr float ALTITUDE_BAROMETRIC_EXPONENT = 0.1903f;

    /// BME temperature compensation constants (identical for BME280 and BME680).
    static constexpr float BME_TEMP_COMPENSATION_SCALE_14 = 16384.0f;
    static constexpr float BME_TEMP_COMPENSATION_SCALE_10 = 1024.0f;
    static constexpr float BME_TEMP_COMPENSATION_OFFSET = 24576.0f;
    static constexpr float BME_TEMP_COMPENSATION_SCALE_16 = 65536.0f;
    static constexpr float BME_TEMP_TFINE_SCALE_8 = 256.0f;

    /// BME pressure compensation constants (identical for BME280 and BME680).
    static constexpr float BME_PRESS_COMPENSATION_DIVISOR_2 = 2.0f;
    static constexpr float BME_PRESS_COMPENSATION_OFFSET = 64000.0f;
    static constexpr float BME_PRESS_COMPENSATION_SCALE_15 = 32768.0f;
    static constexpr float BME_PRESS_COMPENSATION_DIVISOR_4 = 4.0f;
    static constexpr float BME_PRESS_COMPENSATION_SCALE_17 = 131072.0f;
    static constexpr float BME_PRESS_COMPENSATION_SCALE_19 = 524288.0f;
    static constexpr float BME_PRESS_RAW_OFFSET = 1048576.0f;
    static constexpr float BME_PRESS_COMPENSATION_SCALE_12 = 4096.0f;
    static constexpr float BME_PRESS_SCALING_FACTOR = 6250.0f;
    static constexpr float BME_PRESS_OVERFLOW_PROTECTION = 2147483648.0f;
    static constexpr float BME_PRESS_DIVISOR_16 = 16.0f;
    static constexpr float BME_PRESS_TO_HECTOPASCALS = 100.0f;

    /// BME humidity compensation constants (identical for BME280 and BME680).
    static constexpr float BME_HUMID_COMPENSATION_OFFSET = 76800.0f;
    static constexpr float BME_HUMID_COMPENSATION_SCALE_6 = 64.0f;
    static constexpr float BME_HUMID_COMPENSATION_SCALE_14 = 16384.0f;
    static constexpr float BME_HUMID_COMPENSATION_SCALE_16 = 65536.0f;
    static constexpr float BME_HUMID_COMPENSATION_SCALE_26 = 67108864.0f;
    static constexpr float BME_HUMID_COMPENSATION_SCALE_19 = 524288.0f;
    static constexpr float BME_HUMIDITY_MAX_PERCENTAGE = 100.0f;
    static constexpr float BME_HUMIDITY_MIN_PERCENTAGE = 0.0f;

    /**
     * @brief Reads a 16-bit little-endian value from the sensor.
     * @param registerAddress BME register address.
     * @return 16-bit value read from the sensor.
     */
    [[nodiscard]] int16_t readRegister16(uint8_t registerAddress) const;

    /**
     * @brief Reads the shared factory calibration coefficients (digT1-3, digP1-9, digH1-6).
     * @return @c true if all calibration registers were read successfully.
     * @details Reads 26 bytes for temperature/pressure and 7 bytes for humidity.
     *          Subclasses that need additional coefficients (e.g., gas) should
     *          extend this method or read them separately.
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
     * @details Updates tFine for use by pressure compensation.
     */
    float compensateTemperature(int32_t rawTemperature);

    /**
     * @brief Compensates raw pressure reading.
     * @param rawPressure Raw 20-bit pressure value.
     * @return Compensated pressure in hectopascals.
     * @details Requires tFine to be set by compensateTemperature first.
     */
    float compensatePressure(int32_t rawPressure) const;

    /**
     * @brief Compensates raw humidity reading.
     * @param rawHumidity Raw 16-bit humidity value.
     * @return Compensated humidity in percentage.
     * @details Requires tFine to be set by compensateTemperature first.
     */
    float compensateHumidity(uint16_t rawHumidity) const;

public:
    /// Default BME address when SDO is low.
    static constexpr uint8_t DEFAULT_I2C_ADDRESS = 0x76;
    /// Alternate BME address when SDO is high.
    static constexpr uint8_t ALTERNATE_I2C_ADDRESS = 0x77;

    /// Releases the adapter; the shared Wire interface is not shut down.
    ~BmeEnvironmentalSensor() override = 0;

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

#endif // BME_ENVIRONMENTAL_SENSOR_H
