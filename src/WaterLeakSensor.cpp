/**
 * @file WaterLeakSensor.cpp
 * @brief Implements the water leak detection sensor.
 * @ingroup sensors_module
 * @copyright (c) 2025-2026 Angel Velasquez
 *
 * @author Angel Velasquez
 * @date 2026-11-10
 * @version 1.0
 */

#include "WaterLeakSensor.h"

const Event WaterLeakSensor::WATER_LEAK_DETECTED_EVENT = Event(WATER_LEAK_DETECTED_EVENT_IDENTIFIER);
const Event WaterLeakSensor::WATER_LEAK_CLEARED_EVENT = Event(WATER_LEAK_CLEARED_EVENT_IDENTIFIER);

WaterLeakSensor::WaterLeakSensor(const int signalGpioPin, EventHandler* parentHandler)
    : Sensor(signalGpioPin, parentHandler)
    , lastState(false)
{
    pinMode(pin, INPUT_PULLUP);
}

bool WaterLeakSensor::isWaterLeakDetected() const {
    return lastState;
}

void WaterLeakSensor::processEvent(Event& event) {
    if (const int currentState = digitalRead(pin); currentState == LOW) {
        if (!lastState) {
            // Transition from no leak to leak
            lastState = true;
            event.identifier = WATER_LEAK_DETECTED_EVENT_IDENTIFIER;
            event.sourceId = pin;
        } else {
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
        }
    } else {
        if (lastState) {
            // Transition from leak to no leak
            lastState = false;
            event.identifier = WATER_LEAK_CLEARED_EVENT_IDENTIFIER;
            event.sourceId = pin;
        } else {
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
        }
    }
}

uint32_t WaterLeakSensor::getDefaultSamplingIntervalInMilliseconds() const {
    return DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

uint32_t WaterLeakSensor::getMinimalSamplingIntervalInMilliseconds() const {
    return MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS;
}
