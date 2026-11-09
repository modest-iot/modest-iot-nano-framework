/**
 * @file SoilMoistureSensor.cpp
 * @brief Implements analogRead-based soil moisture detection with change detection.
 * @date 2026-11-09
 * @version 1.0
 */

#include "SoilMoistureSensor.h"
#include <Arduino.h>
#include <cmath>


SoilMoistureSensor::SoilMoistureSensor(const int analogInputGpioPin, EventHandler* parentHandler)
    : Sensor(analogInputGpioPin, parentHandler),
      cachedMoistureLevel(INITIAL_CACHED_MOISTURE_LEVEL),
      lastNotifiedMoistureLevel(INITIAL_LAST_NOTIFIED_MOISTURE_LEVEL) {
    pinMode(pin, INPUT);
}

void SoilMoistureSensor::processEvent(Event& event) {
    if (event.identifier == MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        cachedMoistureLevel = analogRead(pin);

        if (std::fabs(static_cast<float>(cachedMoistureLevel) - static_cast<float>(lastNotifiedMoistureLevel)) > MOISTURE_CHANGE_EPSILON) {
            lastNotifiedMoistureLevel = cachedMoistureLevel;
            reportReadSuccess(event);
            event = Event(DATA_READ_EVENT_IDENTIFIER, pin);
            return;
        }

        reportReadSuccess(event);
        event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
    }
}

uint32_t SoilMoistureSensor::getDefaultSamplingIntervalInMilliseconds() const {
    return DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

uint32_t SoilMoistureSensor::getMinimalSamplingIntervalInMilliseconds() const {
    return MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

int SoilMoistureSensor::getMoistureLevel() const { return cachedMoistureLevel; }
