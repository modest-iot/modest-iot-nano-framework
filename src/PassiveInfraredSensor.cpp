/**
 * @file PassiveInfraredSensor.cpp
 * @brief Implements PIR sensor sampling and motion-detection event emission.
 */

#include "PassiveInfraredSensor.h"
#include <Arduino.h>

const Event PassiveInfraredSensor::MOTION_DETECTED_EVENT = Event(MOTION_DETECTED_EVENT_IDENTIFIER);
const Event PassiveInfraredSensor::MOTION_STOPPED_EVENT = Event(MOTION_STOPPED_EVENT_IDENTIFIER);

PassiveInfraredSensor::PassiveInfraredSensor(const int signalGpioPin, EventHandler* parentHandler)
    : Sensor(signalGpioPin, parentHandler),
      lastState(LOW)
{
    pinMode(pin, INPUT);
}

void PassiveInfraredSensor::processEvent(Event& event) {
    if (event.identifier == MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        if (const bool currentState = digitalRead(pin); currentState != lastState) {
            lastState = currentState;

            if (currentState == HIGH) {
                event = Event(MOTION_DETECTED_EVENT_IDENTIFIER, pin);
                return;
            } else {
                event = Event(MOTION_STOPPED_EVENT_IDENTIFIER, pin);
                return;
            }
        }
        event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER; // Suppress
    }
}
