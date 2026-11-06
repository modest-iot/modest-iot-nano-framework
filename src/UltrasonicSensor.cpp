/**
 * @file UltrasonicSensor.cpp
 * @brief Implements pulse-echo distance measurement for ultrasonic sensors.
 * @date 2026-06-06
 * @version 1.0
 */

#include "UltrasonicSensor.h"
#include <Arduino.h>

UltrasonicSensor::UltrasonicSensor(const int triggerGpioPin, const int echoGpioPin, EventHandler* parentHandler)
    : Sensor(triggerGpioPin, parentHandler), echoGpioPin(echoGpioPin), cachedDistanceInCentimeters(-1.0f) {
    pinMode(pin, OUTPUT);
    pinMode(echoGpioPin, INPUT);
}

void UltrasonicSensor::processEvent(Event& event) {
    // Both DHT and Ultrasonic sensors listen for the exact same unified base ID!
    if (event.identifier == MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        digitalWrite(pin, LOW);
        delayMicroseconds(TRIGGER_LOW_INITIAL_DELAY_IN_MICROSECONDS);
        
        digitalWrite(pin, HIGH);
        delayMicroseconds(TRIGGER_HIGH_DURATION_IN_MICROSECONDS);
        digitalWrite(pin, LOW);

        const long pulseTravelDurationInMicroseconds = static_cast<long>(pulseIn(echoGpioPin, HIGH, TIMEOUT_LIMIT_IN_MICROSECONDS));
        
        if (pulseTravelDurationInMicroseconds == TRACKING_FAILURE_VALUATION) {
            reportReadFailure(event);
            if (event.identifier != Sensor::SENSOR_FAULT_EVENT_IDENTIFIER) {
                event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
            }
            return;
        }

        const float calculatedDistance = static_cast<float>(pulseTravelDurationInMicroseconds) * SOUND_SPEED_DISTANCE_COEFFICIENT;

        if (calculatedDistance == cachedDistanceInCentimeters) {
            reportReadSuccess(event);
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
            return;
        }

        reportReadSuccess(event);
        cachedDistanceInCentimeters = calculatedDistance;
        
        // Mutates to the exact same base notification event!
        event = Event(DATA_READ_EVENT_IDENTIFIER, pin);
    }
}

float UltrasonicSensor::getDistanceInCentimeters() const { return cachedDistanceInCentimeters; }

uint32_t UltrasonicSensor::getDefaultSamplingIntervalInMilliseconds() const {
    return DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

uint32_t UltrasonicSensor::getMinimalSamplingIntervalInMilliseconds() const {
    return MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS;
}