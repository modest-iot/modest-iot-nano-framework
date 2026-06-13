/**
 * @file UltrasonicSensor.cpp
 * @brief Implements pulse-echo distance measurement for ultrasonic sensors.
 * @date 2026-06-06
 * @version 1.0
 */

#include "UltrasonicSensor.h"
#include <Arduino.h>

UltrasonicSensor::UltrasonicSensor(int triggerGpioPin, int echoGpioPin, EventHandler* parentHandler)
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

        long pulseTravelDurationInMicroseconds = static_cast<long>(pulseIn(echoGpioPin, HIGH, TIMEOUT_LIMIT_IN_MICROSECONDS));
        
        if (pulseTravelDurationInMicroseconds == TRACKING_FAILURE_VALUATION) {
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
            return;
        }

        float calculatedDistance = static_cast<float>(pulseTravelDurationInMicroseconds) * SOUND_SPEED_DISTANCE_COEFFICIENT;

        if (calculatedDistance == cachedDistanceInCentimeters) {
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
            return;
        }

        cachedDistanceInCentimeters = calculatedDistance;
        
        // Mutates to the exact same base notification event!
        event = Event(DATA_READ_EVENT_IDENTIFIER, pin);
    }
}

float UltrasonicSensor::getDistanceInCentimeters() const { return cachedDistanceInCentimeters; }