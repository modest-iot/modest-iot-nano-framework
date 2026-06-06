#ifndef ULTRASONIC_SENSOR_H
#define ULTRASONIC_SENSOR_H

/**
 * @file UltrasonicSensor.h
 * @brief Declares an HC-SR04 style ultrasonic distance sensor adapter.
 */

#include "Sensor.h"

/**
 * @brief Distance sensor that triggers an ultrasonic pulse and measures echo time.
 */
class UltrasonicSensor : public Sensor {
private:
    int echoGpioPin;
    float cachedDistanceInCentimeters;

    static const int TRIGGER_HIGH_DURATION_IN_MICROSECONDS = 10;
    static const int TRIGGER_LOW_INITIAL_DELAY_IN_MICROSECONDS = 2;
    static const long TIMEOUT_LIMIT_IN_MICROSECONDS = 30000; 
    
    static constexpr float SOUND_SPEED_DISTANCE_COEFFICIENT = 0.01715f;
    static const int PROPAGATION_SUPPRESSION_IDENTIFIER = -1;
    static const long TRACKING_FAILURE_VALUATION = 0;

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
    float getDistanceInCentimeters() const;
};

#endif // ULTRASONIC_SENSOR_H

