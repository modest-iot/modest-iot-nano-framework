/**
 * @file UltrasonicSensor.cpp
 * @brief Implements pulse-echo distance measurement for ultrasonic sensors.
 * @date 2026-06-06
 * @version 1.0
 */

#include "UltrasonicSensor.h"
#include <Arduino.h>

#ifdef ARDUINO_ARCH_ESP32
#include "esp_timer.h"
#endif

UltrasonicSensor::UltrasonicSensor(const int triggerGpioPin, const int echoGpioPin, EventHandler* parentHandler)
    : Sensor(triggerGpioPin, parentHandler),
      echoGpioPin(echoGpioPin),
      cachedDistanceInCentimeters(INITIAL_CACHED_DISTANCE),
      echoStarted(false),
      echoStartTime(0),
      echoDurationNanoseconds(0),
      echoComplete(false) {
    pinMode(pin, OUTPUT);
    pinMode(echoGpioPin, INPUT);
    attachInterruptArg(echoGpioPin, &UltrasonicSensor::onEchoInterrupt, this, CHANGE);
}

UltrasonicSensor::~UltrasonicSensor() {
    detachInterrupt(echoGpioPin);
}

void IRAM_ATTR UltrasonicSensor::onEchoInterrupt(void* sensorInstance) {
    const auto sensor = static_cast<UltrasonicSensor*>(sensorInstance);
    if (sensor == nullptr)
        return;

    if (const int echoLevel = digitalRead(sensor->echoGpioPin); echoLevel == HIGH) {
        // Rising edge: echo started, record start time
        sensor->echoStartTime = esp_timer_get_time() * NANOSECONDS_PER_MICROSECOND;
        sensor->echoStarted = true;
        sensor->echoComplete = false;
    } else if (sensor->echoStarted) {
        // Falling edge: echo ended, compute duration
        const int64_t endTime = esp_timer_get_time() * NANOSECONDS_PER_MICROSECOND;
        sensor->echoDurationNanoseconds = endTime - sensor->echoStartTime;
        sensor->echoStarted = false;
        sensor->echoComplete = true;
    }
}

void UltrasonicSensor::processEvent(Event& event) {
    if (event.identifier == MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        // Reset echo state for this measurement cycle
        echoComplete = false;
        echoStarted = false;
        echoDurationNanoseconds = 0;

        // Trigger the ultrasonic pulse
        digitalWrite(pin, LOW);
        delayMicroseconds(TRIGGER_LOW_INITIAL_DELAY_IN_MICROSECONDS);
        digitalWrite(pin, HIGH);
        delayMicroseconds(TRIGGER_HIGH_DURATION_IN_MICROSECONDS);
        digitalWrite(pin, LOW);

        // Wait for echo measurement to complete (non-blocking poll with timeout)
        const unsigned long startTime = millis();
        while (!echoComplete && millis() - startTime < static_cast<unsigned long>(
                   TIMEOUT_LIMIT_IN_MICROSECONDS / 1000)) {
            // Brief yield to allow ISR to execute
        }

        if (!echoComplete || echoDurationNanoseconds <= 0) {
            reportReadFailure(event);
            if (event.identifier != SENSOR_FAULT_EVENT_IDENTIFIER) {
                event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
            }
            return;
        }

        // Convert nanoseconds to microseconds for distance calculation
        const long pulseTravelDurationInMicroseconds = echoDurationNanoseconds / NANOSECONDS_PER_MICROSECOND;

        const float calculatedDistance = static_cast<float>(pulseTravelDurationInMicroseconds) * SOUND_SPEED_DISTANCE_COEFFICIENT;

        if (calculatedDistance == cachedDistanceInCentimeters) {
            reportReadSuccess(event);
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
            return;
        }

        reportReadSuccess(event);
        cachedDistanceInCentimeters = calculatedDistance;
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