#ifndef DIGITAL_LIGHT_INTENSITY_SENSOR_H
#define DIGITAL_LIGHT_INTENSITY_SENSOR_H

#include "Sensor.h"
#include <Arduino.h>

/**
 * @brief GY-302 / BH1750 I2C digital illuminance sensor.
 * @ingroup sensors_module
 *
 * Samples the BH1750 in continuous high-resolution mode and emits light/dark
 * transition events when illuminance crosses a lux threshold. The first valid
 * sample emits an event even if it is dark. Initialization runs at construction;
 * check isInitialized() and call initialize() to retry if it fails. Read failures
 * preserve the last valid sample, and initialization status reflects command results
 * rather than ongoing connectivity.
 * The GY-302 ADDR pin selects 0x23 (low/open) or 0x5C (high).
 */
class DigitalLightIntensitySensor : public Sensor {
private:
    uint8_t i2cAddress; ///< BH1750 7-bit I2C address.
    float lightDetectedThresholdInLux; ///< Illuminance above which light is considered present.
    float cachedIlluminanceInLux; ///< Last successful illuminance reading.
    bool lastState; ///< Last known light-detected state.
    bool initialized; ///< Whether the initialization commands succeeded.
    bool validSample; ///< Whether a sample has been read since initialization.
    unsigned long conversionStartedAt; ///< Start time of the first conversion.

    static constexpr uint8_t BH1750_POWER_ON_INSTRUCTION = 0x01;
    static constexpr uint8_t BH1750_RESET_INSTRUCTION = 0x07;
    static constexpr uint8_t BH1750_SET_CONTINUOUS_HIGH_RESOLUTION_MODE_INSTRUCTION = 0x10;
    static constexpr float BH1750_RAW_TO_LUX_DIVISOR = 1.2f;
    static constexpr unsigned long BH1750_FIRST_CONVERSION_TIME_MS = 180;

    [[nodiscard]] bool writeInstruction(uint8_t instruction) const;
    bool readIlluminanceInLux(float& illuminanceInLux) const;

protected:
    /**
     * @brief Samples BH1750 illuminance and detects light-state transitions.
     * @param event Mutable event request/response payload.
     */
    void processEvent(Event& event) override;

public:
    static constexpr uint8_t DEFAULT_I2C_ADDRESS = 0x23;
    static constexpr uint8_t ALTERNATE_I2C_ADDRESS = 0x5C;
    static constexpr float DEFAULT_LIGHT_DETECTED_THRESHOLD_IN_LUX = 10.0f;

    /** @brief Identifier emitted when light is detected. */
    static constexpr int LIGHT_DETECTED_EVENT_IDENTIFIER = 50;
    /** @brief Identifier emitted when light is absent. */
    static constexpr int LIGHT_ABSENT_EVENT_IDENTIFIER = 51;
    /** @brief Prebuilt event instance for light-detected notifications. */
    static const Event LIGHT_DETECTED_EVENT;
    /** @brief Prebuilt event instance for light-absent notifications. */
    static const Event LIGHT_ABSENT_EVENT;
    static constexpr int PROPAGATION_SUPPRESSION_IDENTIFIER = -1; ///< Identifier for propagation suppression.

    /**
     * @brief Constructs a GY-302 / BH1750 sensor adapter.
     * @param signalGpioPin The GPIO pin connected to the sensor's signal line.
     * @param i2cAddress The 7-bit I2C address of the sensor (0x23 or 0x5C).
     * @param lightDetectedThresholdInLux Illuminance threshold used for light/dark events.
     * @param parentHandler Optional upstream handler for propagated events.
     */
    explicit DigitalLightIntensitySensor(int signalGpioPin, uint8_t i2cAddress = DEFAULT_I2C_ADDRESS,
                                         float lightDetectedThresholdInLux = DEFAULT_LIGHT_DETECTED_THRESHOLD_IN_LUX,
                                         EventHandler* parentHandler = nullptr);
    ~DigitalLightIntensitySensor() override = default;

    /** @brief Retries initialization; invalidates the previous sample even on failure.
     * @return True if all initialization commands succeeded.
     */
    bool initialize();

    /** @brief Returns whether the initialization commands succeeded. */
    [[nodiscard]] bool isInitialized() const;

    /** @brief Returns whether a successful sample is available since initialization. */
    [[nodiscard]] bool hasValidSample() const;

    /** @brief Returns the last sampled light state; check hasValidSample() first. */
    [[nodiscard]] bool isLightDetected() const;

    /** @brief Returns the most recent successful illuminance in lux; check hasValidSample() first. */
    [[nodiscard]] float getIlluminanceInLux() const;
};

#endif // DIGITAL_LIGHT_INTENSITY_SENSOR_H
