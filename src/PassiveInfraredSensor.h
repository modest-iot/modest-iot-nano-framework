#ifndef PASSIVE_INFRARED_SENSOR_H
#define PASSIVE_INFRARED_SENSOR_H

#include "Sensor.h"

/**
 * @brief Passive Infrared (PIR) motion sensor.
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

public:
    /** @brief Identifier emitted when motion is detected. */
    static constexpr int MOTION_DETECTED_EVENT_IDENTIFIER = 30;
    static constexpr int MOTION_STOPPED_EVENT_IDENTIFIER = 31;
    /** @brief Prebuilt event instance for motion-detected notifications. */
    static const Event MOTION_DETECTED_EVENT;
    /** @brief Prebuilt event instance for motion-stopped notifications. */
    static const Event MOTION_STOPPED_EVENT;
    static constexpr int PROPAGATION_SUPPRESSION_IDENTIFIER = -1; ///< Identifier for propagation suppression.
    /**
     * @brief Constructs a PIR sensor with an optional propagation target.
     * @param signalGpioPin Input GPIO pin connected to the PIR sensor output.
     * @param parentHandler Optional upstream handler for propagated events.
     */
    PassiveInfraredSensor(int signalGpioPin, EventHandler* parentHandler = nullptr);
    ~PassiveInfraredSensor() override = default;
};

#endif // PASSIVE_INFRARED_SENSOR_H
