#ifndef PTH_SENSOR_H
#define PTH_SENSOR_H

/**
 * @file PthSensor.h
 * @brief Declares the BME280-compatible pressure, temperature, and humidity sensor adapter.
 * @ingroup sensors_module
 */

#include "BmeEnvironmentalSensor.h"

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
 * @see BmeEnvironmentalSensor
 */
class PthSensor : public BmeEnvironmentalSensor {
private:
    /// BME280 chip-specific configuration constants.
    static constexpr uint8_t BME280_CHIP_ID_VALUE = 0x60;
    static constexpr uint8_t BME280_MODE_NORMAL = 0x03;

protected:
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
};

#endif // PTH_SENSOR_H
