#ifndef ULTRASONIC_SENSOR_H
#define ULTRASONIC_SENSOR_H

/**
 * @file UltrasonicSensor.h
 * @brief Declares an HC-SR04 style ultrasonic distance sensor adapter.
 * @ingroup sensors_module
 * @date 2026-06-06
 * @version 1.0
 */

#include "Sensor.h"

/**
 * @name Sensors: Environmental & Distance
 * @ingroup sensors_module
 * @{
 */

/**
 * @brief Distance sensor that triggers an ultrasonic pulse and measures echo time.
 * @ingroup sensors_module
 */
class UltrasonicSensor : public Sensor {
private:
    int echoGpioPin; ///< GPIO pin for echo reception.
    float cachedDistanceInCentimeters; ///< Last successful distance reading in cm.

    static constexpr int TRIGGER_HIGH_DURATION_IN_MICROSECONDS = 10; ///< Trigger pulse duration.
    static constexpr int TRIGGER_LOW_INITIAL_DELAY_IN_MICROSECONDS = 2; ///< Pre-trigger delay.
    static constexpr long TIMEOUT_LIMIT_IN_MICROSECONDS = 30000; ///< Echo timeout limit.
    
    static constexpr float SOUND_SPEED_DISTANCE_COEFFICIENT = 0.01715f; ///< Sound speed distance coefficient.
    static constexpr int PROPAGATION_SUPPRESSION_IDENTIFIER = -1; ///< Propagation suppression ID.
    static constexpr long TRACKING_FAILURE_VALUATION = 0; ///< Valuation on failure.

protected:
    /**
     * @brief Performs a ranging cycle and updates the cached distance value.
     * @param event Mutable event request/response payload.
     */
    void processEvent(Event& event) override;

public:
    /**
     * @brief Constructs an ultrasonic distance sensor.
     * @param triggerGpioPin Output pin used to trigger the sonar pulse.
     * @param echoGpioPin Input pin used to capture echo return timing.
     * @param parentHandler Optional upstream handler for propagated events.
     */
    UltrasonicSensor(int triggerGpioPin, int echoGpioPin, EventHandler* parentHandler = nullptr);

    /**
     * @brief Returns the most recent successful distance estimate.
     * @return Distance in centimeters.
     */
    [[nodiscard]] float getDistanceInCentimeters() const;
};

/** @} */ // End of Sensors: Environmental & Distance

#endif // ULTRASONIC_SENSOR_H