#ifndef ULTRASONIC_SENSOR_H
#define ULTRASONIC_SENSOR_H

#include "Sensor.h"

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
    void processEvent(Event& event) override;

public:
    UltrasonicSensor(int triggerGpioPin, int echoGpioPin, EventHandler* parentHandler = nullptr);

    float getDistanceInCentimeters() const; 
};

#endif // ULTRASONIC_SENSOR_H