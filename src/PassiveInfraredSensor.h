#ifndef PASSIVE_INFRARED_SENSOR_H
#define PASSIVE_INFRARED_SENSOR_H

/**
 * @file PassiveInfraredSensor.h
 * @brief Declares a passive infrared (PIR) motion sensor.
 * @ingroup sensors_module
 * @date 2026-06-06
 * @version 1.0
 */

#include "Sensor.h"

/**
 * @name Sensors: Digital & Debounced Input
 * @ingroup sensors_module
 * @{
 */

/**
 * @brief Passive Infrared (PIR) motion sensor that detects motion transitions.
 * @ingroup sensors_module
 */
class PassiveInfraredSensor : public Sensor {
private:
    bool lastState; ///< The last known state of the PIR sensor.

protected:
    /**
     * @brief Samples the PIR sensor state and detects motion transitions.
     * @param event Mutable event request/response payload.
     */
    void processEvent(Event& event) override;

    [[nodiscard]] uint32_t getDefaultSamplingIntervalInMilliseconds() const override;
    [[nodiscard]] uint32_t getMinimalSamplingIntervalInMilliseconds() const override;

public:
    /** @brief Identifier emitted when motion is detected. */
    static constexpr int MOTION_DETECTED_EVENT_IDENTIFIER = 30;
    static constexpr int MOTION_STOPPED_EVENT_IDENTIFIER = 31;
    /** @brief Prebuilt event instance for motion-detected notifications. */
    static const Event MOTION_DETECTED_EVENT;
    /** @brief Prebuilt event instance for motion-stopped notifications. */
    static const Event MOTION_STOPPED_EVENT;
    static constexpr int PROPAGATION_SUPPRESSION_IDENTIFIER = -1; ///< Identifier for propagation suppression.

    /// Default sampling interval in milliseconds.
    /// @note PIR module hold time typically requires ~50 ms polling.
    static constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS = 100;
    /// Minimum sampling interval in milliseconds.
    /// @note Below 20 ms the PIR output cannot settle reliably.
    static constexpr uint32_t MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 20;

    /**
     * @brief Constructs a PIR sensor with an optional propagation target.
     * @param signalGpioPin Input GPIO pin connected to the PIR sensor output.
     * @param parentHandler Optional upstream handler for propagated events.
     */
    PassiveInfraredSensor(int signalGpioPin, EventHandler* parentHandler = nullptr);
    ~PassiveInfraredSensor() override = default;
};

/** @} */ // End of Sensors: Digital & Debounced Input

#endif // PASSIVE_INFRARED_SENSOR_H
