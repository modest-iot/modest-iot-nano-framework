#ifndef ENVIRONMENTAL_GAS_SENSOR_H
#define ENVIRONMENTAL_GAS_SENSOR_H

/**
 * @file EnvironmentalGasSensor.h
 * @brief Declares the BME680-compatible environmental gas sensor adapter.
 * @ingroup sensors_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @license Apache-2.0
 */

#include "BmeEnvironmentalSensor.h"

/**
 * @brief Event-driven adapter for a BME680-compatible environmental gas sensor.
 *
 * The sensor provides factory-calibrated readings for temperature, humidity,
 * atmospheric pressure, and gas resistance (VOC detection) via I²C. On each
 * @ref Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER request, the adapter reads
 * all four values, applies Bosch compensation formulas, and emits
 * @ref Sensor::DATA_READ_EVENT_IDENTIFIER when any value changes beyond the
 * configured epsilon thresholds.
 *
 * Initialization reads the factory calibration coefficients from non-volatile
 * registers. Check isInitialized() before relying on readings.
 *
 * @note The BME680 ADDR pin selects @ref DEFAULT_I2C_ADDRESS (low) or
 *       @ref ALTERNATE_I2C_ADDRESS (high). The constructor calls Wire.begin(),
 *       which initializes the shared Arduino I2C interface.
 * @see BmeEnvironmentalSensor
 */
class EnvironmentalGasSensor : public BmeEnvironmentalSensor {
protected:
    /** @name BME680 Gas Calibration Coefficients */
    //@{
    int8_t digG1;  ///< Gas calibration coefficient G1.
    uint8_t digG2; ///< Gas calibration coefficient G2.
    int8_t digG3;  ///< Gas calibration coefficient G3.
    //@} // BME680 Gas Calibration Coefficients

    float cachedGasResistanceInKOhms;      ///< Cached gas resistance in kilo-ohms.
    float lastNotifiedGasResistanceInKOhms; ///< Last-notified gas resistance for change detection.

    /** @name Gas Resistance Change Detection */
    //@{
    static constexpr float GAS_RESISTANCE_CHANGE_EPSILON = 1000.0f; ///< Minimum gas resistance change to emit event (kΩ).
    static constexpr float INITIAL_CACHED_GAS_RESISTANCE_IN_KOHMS = 0.0f; ///< Initial cached gas resistance.
    static constexpr float INITIAL_LAST_NOTIFIED_GAS_RESISTANCE_IN_KOHMS = -1.0f; ///< Initial last-notified gas resistance sentinel.
    //@} // Gas Resistance Change Detection

    /** @name BME680 Chip-Specific Configuration */
    //@{
    static constexpr uint8_t BME680_CHIP_ID_VALUE = 0x61; ///< BME680 chip ID value.
    static constexpr uint8_t BME680_MODE_FORCED = 0x01;   ///< Forced measurement mode.
    static constexpr uint8_t BME680_REGISTER_GAS_DATA = 0x2A; ///< Gas data register address.
    static constexpr uint8_t BME680_REGISTER_CALIBRATION_GAS = 0xE3; ///< Gas calibration register address.
    static constexpr uint8_t BME680_CALIBRATION_GAS_BYTE_COUNT = 3; ///< Gas calibration byte count.
    static constexpr uint8_t BME680_RAW_GAS_DATA_BYTE_COUNT = 3; ///< Raw gas data byte count.
    //@} // BME680 Chip-Specific Configuration

    /** @name BME680 Gas Resistance Lookup Table */
    //@{
    static constexpr int32_t BME680_GAS_CONST_ARRAY_0 = 2600;  ///< Gas lookup table entry 0.
    static constexpr int32_t BME680_GAS_CONST_ARRAY_1 = 2560;  ///< Gas lookup table entry 1.
    static constexpr int32_t BME680_GAS_CONST_ARRAY_2 = 2520;  ///< Gas lookup table entry 2.
    static constexpr int32_t BME680_GAS_CONST_ARRAY_3 = 2480;  ///< Gas lookup table entry 3.
    static constexpr int32_t BME680_GAS_CONST_ARRAY_4 = 2440;  ///< Gas lookup table entry 4.
    static constexpr int32_t BME680_GAS_CONST_ARRAY_5 = 2400;  ///< Gas lookup table entry 5.
    static constexpr int32_t BME680_GAS_CONST_ARRAY_6 = 2360;  ///< Gas lookup table entry 6.
    static constexpr int32_t BME680_GAS_CONST_ARRAY_7 = 2320;  ///< Gas lookup table entry 7.
    static constexpr int32_t BME680_GAS_CONST_ARRAY_8 = 2280;  ///< Gas lookup table entry 8.
    static constexpr int32_t BME680_GAS_CONST_ARRAY_9 = 2240;  ///< Gas lookup table entry 9.
    static constexpr int32_t BME680_GAS_CONST_ARRAY_10 = 2200; ///< Gas lookup table entry 10.
    static constexpr int32_t BME680_GAS_CONST_ARRAY_11 = 2160; ///< Gas lookup table entry 11.
    static constexpr int32_t BME680_GAS_CONST_ARRAY_12 = 2120; ///< Gas lookup table entry 12.
    static constexpr int32_t BME680_GAS_CONST_ARRAY_13 = 2080; ///< Gas lookup table entry 13.
    static constexpr int32_t BME680_GAS_CONST_ARRAY_14 = 2040; ///< Gas lookup table entry 14.
    static constexpr int32_t BME680_GAS_CONST_ARRAY_15 = 2000; ///< Gas lookup table entry 15.
    //@} // BME680 Gas Resistance Lookup Table

    /** @name BME680 Gas Resistance Calculation */
    //@{
    static constexpr int32_t BME680_GAS_SHIFT_15 = 15;   ///< Gas calculation shift amount 15.
    static constexpr int32_t BME680_GAS_SHIFT_24 = 24;   ///< Gas calculation shift amount 24.
    static constexpr int32_t BME680_GAS_SHIFT_9 = 9;     ///< Gas calculation shift amount 9.
    static constexpr int32_t BME680_GAS_SHIFT_1 = 1;     ///< Gas calculation shift amount 1.
    static constexpr float BME680_OHMS_TO_KOHMS = 1000.0f; ///< Ohms to kilo-ohms conversion factor.
    //@} // BME680 Gas Resistance Calculation

    /**
     * @brief Reads gas calibration coefficients (digG1-3).
     * @return @c true if gas calibration registers were read successfully.
     */
    [[nodiscard]] bool readGasCalibrationData();

    /**
     * @brief Reads raw gas resistance data.
     * @param rawGasResistance Output receiving raw gas resistance (20-bit).
     * @param gasRange Output receiving gas ADC range (0-15).
     * @return @c true if gas data bytes were read successfully.
     */
    bool readRawGasData(uint16_t& rawGasResistance, uint8_t& gasRange) const;

    /**
     * @brief Compensates raw gas resistance reading.
     * @param rawGasResistance Raw 20-bit gas ADC value.
     * @param gasRange Gas ADC range (0-15).
     * @return Compensated gas resistance in kilo-ohms.
     */
    float compensateGasResistance(uint16_t rawGasResistance, uint8_t gasRange) const;

    /**
     * @brief Handles measurement requests and emits data-read events.
     * @param event Request event to inspect and, when applicable, replace with a
     *              data-read event or the propagation-suppression identifier.
     * @details Non-measurement events are left unchanged. A measurement request is
     *          suppressed while uninitialized, on read failure, or when no value
     *          has changed beyond the epsilon thresholds.
     * @runs_on{sampler}
     */
    void processEvent(Event& event) override;

    [[nodiscard]] uint32_t getDefaultSamplingIntervalInMilliseconds() const override;
    [[nodiscard]] uint32_t getMinimalSamplingIntervalInMilliseconds() const override;

public:
    /// Default sampling interval in milliseconds.
    static constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS = 2000;
    /// Minimum sampling interval in milliseconds.
    static constexpr uint32_t MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 500;

    /**
     * @brief Constructs and initializes a BME680-compatible sensor adapter.
     * @param signalGpioPin Framework source identifier attached to emitted events;
     *                       this sensor communicates over I2C and does not use it
     *                       as a GPIO pin.
     * @param i2cAddress Sensor's 7-bit I2C address; use
     *                   @ref DEFAULT_I2C_ADDRESS or @ref ALTERNATE_I2C_ADDRESS.
     * @param parentHandler Optional handler that receives emitted events.
     * @note Calls Wire.begin() and attempts initialize(); inspect isInitialized()
     *       to determine whether initialization succeeded.
     */
    explicit EnvironmentalGasSensor(int signalGpioPin, uint8_t i2cAddress = DEFAULT_I2C_ADDRESS,
                                    EventHandler* parentHandler = nullptr);

    /// Releases the adapter; the shared Wire interface is not shut down.
    ~EnvironmentalGasSensor() override = default;

    /**
     * @brief Retries the BME680 initialization sequence.
     * @return @c true if the chip ID and calibration data were read successfully.
     * @details Reads the chip ID to verify connectivity, then reads all calibration
     *          coefficients. On success, configures the sensor for forced mode.
     */
    bool initialize();

    /**
     * @brief Returns the gas resistance from the most recent successful sample.
     * @return Gas resistance in kilo-ohms (kΩ).
     * @warning Check isInitialized() first; before initialization this returns 0.
     */
    [[nodiscard]] float getGasResistanceInKOhms() const;
};

#endif // ENVIRONMENTAL_GAS_SENSOR_H
