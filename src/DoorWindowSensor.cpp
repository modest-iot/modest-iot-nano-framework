/**
 * @file DoorWindowSensor.cpp
 * @brief Implements the door/window open/close detection sensor.
 * @ingroup sensors_module
 * @copyright (c) 2025-2026 Angel Velasquez
 *
 * @author Angel Velasquez
 * @date 2026-11-10
 * @version 1.0
 */

#include "DoorWindowSensor.h"

const Event DoorWindowSensor::DOOR_OPENED_EVENT = Event(DOOR_OPENED_EVENT_IDENTIFIER);
const Event DoorWindowSensor::DOOR_CLOSED_EVENT = Event(DOOR_CLOSED_EVENT_IDENTIFIER);

DoorWindowSensor::DoorWindowSensor(const int signalGpioPin, EventHandler* parentHandler)
    : Sensor(signalGpioPin, parentHandler)
    , lastState(false)
{
    pinMode(pin, INPUT);
}

bool DoorWindowSensor::isDoorOpen() const {
    return lastState;
}

void DoorWindowSensor::processEvent(Event& event) {
    if (const int currentState = digitalRead(pin); currentState == HIGH) {
        if (!lastState) {
            // Transition from closed to open
            lastState = true;
            event.identifier = DOOR_OPENED_EVENT_IDENTIFIER;
            event.sourceId = pin;
        } else {
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
        }
    } else {
        if (lastState) {
            // Transition from open to closed
            lastState = false;
            event.identifier = DOOR_CLOSED_EVENT_IDENTIFIER;
            event.sourceId = pin;
        } else {
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
        }
    }
}

uint32_t DoorWindowSensor::getDefaultSamplingIntervalInMilliseconds() const {
    return DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

uint32_t DoorWindowSensor::getMinimalSamplingIntervalInMilliseconds() const {
    return MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS;
}
