/**
 * @file DigitalLightIntensitySensor.cpp
 * @brief Implements GY-302 / BH1750 sampling and light-state transition events.
 */

#include "DigitalLightIntensitySensor.h"
#include <Wire.h>

const Event DigitalLightIntensitySensor::LIGHT_DETECTED_EVENT = Event(LIGHT_DETECTED_EVENT_IDENTIFIER);
const Event DigitalLightIntensitySensor::LIGHT_ABSENT_EVENT = Event(LIGHT_ABSENT_EVENT_IDENTIFIER);

DigitalLightIntensitySensor::DigitalLightIntensitySensor(
    const int signalGpioPin,
    const uint8_t i2cAddress,
    const float lightDetectedThresholdInLux,
    EventHandler *parentHandler)
    : Sensor(signalGpioPin, parentHandler),
      i2cAddress(i2cAddress),
      lightDetectedThresholdInLux(lightDetectedThresholdInLux),
      cachedIlluminanceInLux(0.0f),
      lastState(false),
      initialized(false),
      validSample(false),
      conversionStartedAt(0) {
    Wire.begin();
    initialize();
}

bool DigitalLightIntensitySensor::initialize() {
    initialized = false;
    validSample = false;
    if (!writeInstruction(BH1750_POWER_ON_INSTRUCTION) ||
        !writeInstruction(BH1750_RESET_INSTRUCTION) ||
        !writeInstruction(BH1750_SET_CONTINUOUS_HIGH_RESOLUTION_MODE_INSTRUCTION)) {
        return false;
    }
    conversionStartedAt = millis();
    initialized = true;
    return true;
}

bool DigitalLightIntensitySensor::writeInstruction(const uint8_t instruction) const {
    Wire.beginTransmission(i2cAddress);
    Wire.write(instruction);
    return Wire.endTransmission() == 0;
}

bool DigitalLightIntensitySensor::readIlluminanceInLux(float &illuminanceInLux) const {
    if (constexpr uint8_t requestedByteCount = 2;
        Wire.requestFrom(i2cAddress, requestedByteCount) != requestedByteCount) {
        return false;
    }

    const uint16_t rawMeasurement = (static_cast<uint16_t>(Wire.read()) << 8) | static_cast<uint16_t>(Wire.read());
    illuminanceInLux = static_cast<float>(rawMeasurement) / BH1750_RAW_TO_LUX_DIVISOR;
    return true;
}

void DigitalLightIntensitySensor::processEvent(Event &event) {
    if (event.identifier != MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        return;
    }

    if (!initialized || (!validSample && millis() - conversionStartedAt < BH1750_FIRST_CONVERSION_TIME_MS)) {
        event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
        return;
    }

    float currentIlluminanceInLux = 0.0f;
    if (!readIlluminanceInLux(currentIlluminanceInLux)) {
        event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
        return;
    }

    cachedIlluminanceInLux = currentIlluminanceInLux;
    if (const bool currentState = currentIlluminanceInLux >= lightDetectedThresholdInLux;
        !validSample || currentState != lastState) {
        validSample = true;
        lastState = currentState;
        if (currentState) {
            event = Event(LIGHT_DETECTED_EVENT_IDENTIFIER, pin);
            return;
        }
        event = Event(LIGHT_ABSENT_EVENT_IDENTIFIER, pin);
        return;
    }

    event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
}

bool DigitalLightIntensitySensor::isInitialized() const {
    return initialized;
}

bool DigitalLightIntensitySensor::hasValidSample() const {
    return validSample;
}

bool DigitalLightIntensitySensor::isLightDetected() const {
    return lastState;
}

float DigitalLightIntensitySensor::getIlluminanceInLux() const {
    return cachedIlluminanceInLux;
}
