#ifndef ENVIRONMENTAL_GAS_SENSOR_H
#define ENVIRONMENTAL_GAS_SENSOR_H

/**
 * @file EnvironmentalGasSensor.h
 * @brief Declares the BME680-compatible environmental gas sensor adapter.
 * @ingroup sensors_module
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
    // BME680 gas-specific calibration coefficients
    int8_t digG1;
    uint8_t digG2;
    int8_t digG3;

    /// Cached gas resistance from the most recent successful sample.
    float cachedGasResistanceInKOhms;
    
    /// Last-notified gas resistance for change detection.
    float lastNotifiedGasResistanceInKOhms;
    
    /// Gas resistance change detection threshold.
    static constexpr float GAS_RESISTANCE_CHANGE_EPSILON = 1000.0f;
    
    /// Initial gas resistance cached value.
    static constexpr float INITIAL_CACHED_GAS_RESISTANCE_IN_KOHMS = 0.0f;
    
    /// Initial gas resistance last-notified value (sentinel to ensure first reading emits event).
    static constexpr float INITIAL_LAST_NOTIFIED_GAS_RESISTANCE_IN_KOHMS = -1.0f;

    /// BME680 chip-specific configuration constants.
    static constexpr uint8_t BME680_CHIP_ID_VALUE = 0x61;
    static constexpr uint8_t BME680_MODE_FORCED = 0x01;
    static constexpr uint8_t BME680_REGISTER_GAS_DATA = 0x2A;
    static constexpr uint8_t BME680_REGISTER_CALIBRATION_GAS = 0xE3;
    static constexpr uint8_t BME680_CALIBRATION_GAS_BYTE_COUNT = 3;
    static constexpr uint8_t BME680_RAW_GAS_DATA_BYTE_COUNT = 3;

    /// BME680 gas resistance lookup table constants (from Bosch datasheet).
    static constexpr int32_t BME680_GAS_CONST_ARRAY_0 = 2600;
    static constexpr int32_t BME680_GAS_CONST_ARRAY_1 = 2560;
    static constexpr int32_t BME680_GAS_CONST_ARRAY_2 = 2520;
    static constexpr int32_t BME680_GAS_CONST_ARRAY_3 = 2480;
    static constexpr int32_t BME680_GAS_CONST_ARRAY_4 = 2440;
    static constexpr int32_t BME680_GAS_CONST_ARRAY_5 = 2400;
    static constexpr int32_t BME680_GAS_CONST_ARRAY_6 = 2360;
    static constexpr int32_t BME680_GAS_CONST_ARRAY_7 = 2320;
    static constexpr int32_t BME680_GAS_CONST_ARRAY_8 = 2280;
    static constexpr int32_t BME680_GAS_CONST_ARRAY_9 = 2240;
    static constexpr int32_t BME680_GAS_CONST_ARRAY_10 = 2200;
    static constexpr int32_t BME680_GAS_CONST_ARRAY_11 = 2160;
    static constexpr int32_t BME680_GAS_CONST_ARRAY_12 = 2120;
    static constexpr int32_t BME680_GAS_CONST_ARRAY_13 = 2080;
    static constexpr int32_t BME680_GAS_CONST_ARRAY_14 = 2040;
    static constexpr int32_t BME680_GAS_CONST_ARRAY_15 = 2000;

    /// BME680 gas resistance calculation constants.
    static constexpr int32_t BME680_GAS_SHIFT_15 = 15;
    static constexpr int32_t BME680_GAS_SHIFT_24 = 24;
    static constexpr int32_t BME680_GAS_SHIFT_9 = 9;
    static constexpr int32_t BME680_GAS_SHIFT_1 = 1;
    static constexpr float BME680_OHMS_TO_KOHMS = 1000.0f;

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
     * @runs_on sampler
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
