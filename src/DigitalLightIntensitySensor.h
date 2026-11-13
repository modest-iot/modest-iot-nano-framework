#ifndef DIGITAL_LIGHT_INTENSITY_SENSOR_H
#define DIGITAL_LIGHT_INTENSITY_SENSOR_H

/**
 * @file DigitalLightIntensitySensor.h
 * @brief Declares the BH1750 digital illuminance sensor adapter.
 * @ingroup sensors_module
 * @copyright (c) 2025-2026 Angel Velasquez
 */

#include "Sensor.h"
#include <Arduino.h>

/**
 * @brief Event-driven adapter for a GY-302 / BH1750 digital illuminance sensor.
 *
 * The sensor is placed in continuous high-resolution measurement mode. On each
 * @ref Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER request, a sample is read
 * once the first conversion has had time to complete. The adapter emits an event
 * for the first successful sample and thereafter only when the light state changes.
 *
 * A reading at or above the configured threshold is considered light; a reading
 * below it is considered absent. Failed reads do not alter the last successful
 * reading or state. Check hasValidSample() before using the cached accessors.
 *
 * Initialization is attempted by the constructor and can be retried with
 * initialize(). isInitialized() reports whether the most recent initialization
 * commands succeeded; it does not continuously verify device connectivity.
 *
 * @note The GY-302 ADDR pin selects @ref DEFAULT_I2C_ADDRESS (low/open) or
 *       @ref ALTERNATE_I2C_ADDRESS (high). The constructor calls Wire.begin(),
 *       which initializes the shared Arduino I2C interface.
 * @see Sensor
 */
class DigitalLightIntensitySensor : public Sensor {
private:
    uint8_t i2cAddress; ///< Configured BH1750 7-bit I2C address.
    float lightDetectedThresholdInLux; ///< Inclusive lux threshold for the light-detected state.
    float cachedIlluminanceInLux; ///< Illuminance from the most recent successful sample, in lux.
    bool lastState; ///< Light state from the most recent successful sample.
    bool initialized; ///< Whether all commands in the most recent initialization attempt succeeded.
    bool validSample; ///< Whether a successful sample has been read since initialization.
    unsigned long conversionStartedAt; ///< millis() timestamp when the latest initialization started conversion.

    /// BH1750 power-on command.
    static constexpr uint8_t BH1750_POWER_ON_INSTRUCTION = 0x01;
    /// BH1750 data-register reset command.
    static constexpr uint8_t BH1750_RESET_INSTRUCTION = 0x07;
    /// BH1750 continuous high-resolution measurement-mode command.
    static constexpr uint8_t BH1750_SET_CONTINUOUS_HIGH_RESOLUTION_MODE_INSTRUCTION = 0x10;
    /// Datasheet conversion factor from raw measurement units to lux.
    static constexpr float BH1750_RAW_TO_LUX_DIVISOR = 1.2f;
    /// Minimum wait before reading the first conversion, in milliseconds.
    static constexpr unsigned long BH1750_FIRST_CONVERSION_TIME_IN_MILLISECONDS = 180;

    /**
     * @brief Sends one command byte to the configured I2C address.
     * @param instruction BH1750 command byte.
     * @return @c true if the I2C transmission completed successfully.
     */
    [[nodiscard]] bool writeInstruction(uint8_t instruction) const;

    /**
     * @brief Reads one raw measurement and converts it to lux.
     * @param illuminanceInLux Output receiving the converted reading on success.
     * @return @c true if both measurement bytes were received; otherwise @c false.
     * @note On failure, the output argument is not modified.
     */
    bool readIlluminanceInLux(float& illuminanceInLux) const;

protected:
    /**
     * @brief Handles measurement requests and emits state-transition events.
     * @param event Request event to inspect and, when applicable, replace with a
     *              light-state event or the propagation-suppression identifier.
     * @details Non-measurement events are left unchanged. A measurement request is
     *          suppressed while uninitialized, before the first conversion is ready,
     *          on read failure, or when the sampled light state has not changed.
     */
    void processEvent(Event& event) override;

    [[nodiscard]] uint32_t getDefaultSamplingIntervalInMilliseconds() const override;
    [[nodiscard]] uint32_t getMinimalSamplingIntervalInMilliseconds() const override;

public:
    /// Default BH1750 address when ADDR is low or left open.
    static constexpr uint8_t DEFAULT_I2C_ADDRESS = 0x23;
    /// Alternate BH1750 address when ADDR is high.
    static constexpr uint8_t ALTERNATE_I2C_ADDRESS = 0x5C;
    /// Default inclusive threshold for classifying a reading as light, in lux.
    static constexpr float DEFAULT_LIGHT_DETECTED_THRESHOLD_IN_LUX = 10.0f;

    /// Default sampling interval in milliseconds.
    /// @note Based on BH1750 high-resolution conversion time (~180 ms minimum).
    static constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS = 1000;
    /// Minimum sampling interval in milliseconds.
    /// @note BH1750 continuous high-resolution mode requires ~180 ms per conversion.
    static constexpr uint32_t MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 180;

    /// Event identifier emitted when illuminance is at or above the threshold.
    static constexpr int LIGHT_DETECTED_EVENT_IDENTIFIER = 50;
    /// Event identifier emitted when illuminance is below the threshold.
    static constexpr int LIGHT_ABSENT_EVENT_IDENTIFIER = 51;
    /// Prebuilt light-detected event; emitted events use the configured source ID.
    static const Event LIGHT_DETECTED_EVENT;
    /// Prebuilt light-absent event; emitted events use the configured source ID.
    static const Event LIGHT_ABSENT_EVENT;
    /// Event identifier used internally to prevent propagation of a request.
    static constexpr int PROPAGATION_SUPPRESSION_IDENTIFIER = -1;

    /**
     * @brief Constructs and initializes a GY-302 / BH1750 adapter.
     * @param signalGpioPin Framework source identifier attached to emitted events;
     *                       this sensor communicates over I2C and does not use it
     *                       as a GPIO pin.
     * @param i2cAddress Sensor's 7-bit I2C address; use
     *                   @ref DEFAULT_I2C_ADDRESS or @ref ALTERNATE_I2C_ADDRESS.
     * @param lightDetectedThresholdInLux Inclusive illuminance threshold, in lux.
     * @param parentHandler Optional handler that receives emitted transition events.
     * @note Calls Wire.begin() and attempts initialize(); inspect isInitialized()
     *       to determine whether initialization succeeded.
     */
    explicit DigitalLightIntensitySensor(int signalGpioPin, uint8_t i2cAddress = DEFAULT_I2C_ADDRESS,
                                         float lightDetectedThresholdInLux = DEFAULT_LIGHT_DETECTED_THRESHOLD_IN_LUX,
                                         EventHandler* parentHandler = nullptr);

    /// Releases the adapter; the shared Wire interface is not shut down.
    ~DigitalLightIntensitySensor() override = default;

    /**
     * @brief Retries the BH1750 initialization sequence.
     * @return @c true if the power-on, reset, and measurement-mode commands succeed.
     * @details Clears the valid-sample flag before sending commands. On success,
     *          starts the first-conversion delay; on failure, the adapter remains
     *          uninitialized until a later successful retry.
     */
    bool initialize();

    /**
     * @brief Reports the result of the most recent initialization attempt.
     * @return @c true if its initialization commands all succeeded.
     * @note Does not indicate whether the sensor remains connected after initialization.
     */
    [[nodiscard]] bool isInitialized() const;

    /**
     * @brief Reports whether a successful sample is available.
     * @return @c true after at least one successful sample since initialization.
     */
    [[nodiscard]] bool hasValidSample() const;

    /**
     * @brief Returns the state from the most recent successful sample.
     * @return @c true if the sample was at or above the configured threshold.
     * @warning Check hasValidSample() first; before the first valid sample this
     *          returns the default state.
     */
    [[nodiscard]] bool isLightDetected() const;

    /**
     * @brief Returns the illuminance from the most recent successful sample.
     * @return Illuminance in lux.
     * @warning Check hasValidSample() first; before the first valid sample this
     *          returns the default cached value.
     */
    [[nodiscard]] float getIlluminanceInLux() const;
};

#endif // DIGITAL_LIGHT_INTENSITY_SENSOR_H
