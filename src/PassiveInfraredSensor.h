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
 * @brief Passive Infrared (PIR) motion sensor that detects motion transitions.
 *
 * Uses interrupt-driven operation (CHANGE edge) for zero-latency motion detection.
 * The GPIO interrupt fires on both RISING and FALLING edges, and processEvent()
 * distinguishes between motion detected and motion stopped.
 * @ingroup sensors_module
 */
class PassiveInfraredSensor : public Sensor {
private:
    bool lastState; ///< The last known state of the PIR sensor.

protected:
    /**
     * @brief Detects motion transitions from the GPIO interrupt trigger.
     *
     * Called from the sampler task via processTriggeredSensors() after the GPIO
     * interrupt fires. Compares the current pin state with the last known state
     * and emits either MOTION_DETECTED or MOTION_STOPPED events.
     * @param event Mutable event request/response payload.
     * @runs_on sampler
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

    /// Default sampling interval in milliseconds (used only for hybrid registration).
    /// @note In interrupt mode, this value is not used for polling.
    static constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS = 100;
    /// Minimum sampling interval in milliseconds (used only for hybrid registration).
    static constexpr uint32_t MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 20;

    /**
     * @brief Constructs a PIR sensor with an optional propagation target.
     * @param signalGpioPin Input GPIO pin connected to the PIR sensor output.
     * @param parentHandler Optional upstream handler for propagated events.
     * @note Configures the pin as INPUT. Register via
     * appendSensorToTriggeringScheduler() for interrupt-driven operation.
     */
    explicit PassiveInfraredSensor(int signalGpioPin, EventHandler* parentHandler = nullptr);
    ~PassiveInfraredSensor() override = default;
};

#endif // PASSIVE_INFRARED_SENSOR_H
