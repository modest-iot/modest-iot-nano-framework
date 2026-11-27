/**
 * @file DigitalLightIntensitySensor.cpp
 * @brief Implements BH1750 initialization, sampling, and transition-event handling.
 * @copyright (c) 2025-2026 Angel Velasquez
 */

#include "DigitalLightIntensitySensor.h"
#include <Wire.h>

const Event DigitalLightIntensitySensor::LIGHT_DETECTED_EVENT = Event(LIGHT_DETECTED_EVENT_IDENTIFIER);
const Event DigitalLightIntensitySensor::LIGHT_ABSENT_EVENT = Event(LIGHT_ABSENT_EVENT_IDENTIFIER);

namespace {
I2cStatus mapWireStatus(const uint8_t status) {
    switch (status) {
        case 0: return I2cStatus::Ok;
        case 1: return I2cStatus::InvalidArgument;
        case 2: return I2cStatus::AddressNack;
        case 3: return I2cStatus::DataNack;
        case 5: return I2cStatus::Timeout;
        default: return I2cStatus::BusError;
    }
}
}

DigitalLightIntensitySensor::DigitalLightIntensitySensor(
    const int signalGpioPin,
    const uint8_t i2cAddress,
    const float lightDetectedThresholdInLux,
    EventHandler *parentHandler)
    : Sensor(signalGpioPin, parentHandler),
      bus(nullptr),
      lastI2cResult{I2cStatus::Ok, 0, 0},
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

DigitalLightIntensitySensor::DigitalLightIntensitySensor(
    I2cBus& bus,
    const int signalGpioPin,
    const uint8_t i2cAddress,
    const float lightDetectedThresholdInLux,
    EventHandler* parentHandler)
    : Sensor(signalGpioPin, parentHandler),
      bus(&bus),
      lastI2cResult{I2cStatus::Ok, 0, 0},
      i2cAddress(i2cAddress),
      lightDetectedThresholdInLux(lightDetectedThresholdInLux),
      cachedIlluminanceInLux(0.0f),
      lastState(false),
      initialized(false),
      validSample(false),
      conversionStartedAt(0) {}

/**
 * @details Each attempt clears the valid-sample flag. The conversion timer is
 *          restarted only after the complete command sequence succeeds.
 */
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

bool DigitalLightIntensitySensor::start() {
    return initialize();
}

bool DigitalLightIntensitySensor::writeInstruction(const uint8_t instruction) const {
    if (bus != nullptr) {
        lastI2cResult = bus->write(i2cAddress, &instruction, 1);
        return !lastI2cResult.isInvalidOrIncomplete();
    }

    Wire.beginTransmission(i2cAddress);
    const std::size_t written = Wire.write(instruction);
    const I2cStatus status = written == 1 ? mapWireStatus(Wire.endTransmission())
                                          : I2cStatus::InvalidArgument;
    lastI2cResult = {status, status == I2cStatus::Ok ? 1U : 0U, 1};
    return status == I2cStatus::Ok;
}

/**
 * @details The BH1750 returns the most significant byte first. Failed or
 *          incomplete reads leave the output argument unchanged.
 */
bool DigitalLightIntensitySensor::readIlluminanceInLux(float &illuminanceInLux) const {
    uint8_t data[2]{};
    if (bus != nullptr) {
        lastI2cResult = bus->writeRead(i2cAddress, nullptr, 0, data, sizeof(data));
        if (lastI2cResult.isInvalidOrIncomplete()) {
            return false;
        }
    } else {
        if (Wire.requestFrom(i2cAddress, static_cast<uint8_t>(sizeof(data))) != sizeof(data)) {
            lastI2cResult = {I2cStatus::ShortRead, 0, sizeof(data)};
            return false;
        }

        const int highByte = Wire.read();
        const int lowByte = Wire.read();
        if (highByte < 0 || lowByte < 0) {
            lastI2cResult = {I2cStatus::ShortRead, highByte < 0 ? 0U : 1U, sizeof(data)};
            return false;
        }
        data[0] = static_cast<uint8_t>(highByte);
        data[1] = static_cast<uint8_t>(lowByte);
        lastI2cResult = {I2cStatus::Ok, sizeof(data), sizeof(data)};
    }

    if (lastI2cResult.isInvalidOrIncomplete()) {
        return false;
    }

    const uint16_t rawMeasurement = (static_cast<uint16_t>(data[0]) << 8) | data[1];
    illuminanceInLux = static_cast<float>(rawMeasurement) / BH1750_RAW_TO_LUX_DIVISOR;
    return true;
}

/**
 * @details The first conversion is deferred for the BH1750's conversion interval.
 *          Successful measurements update the cached lux value; an event is
 *          propagated for the first successful measurement and whenever the
 *          threshold classification changes. Other requests, premature reads,
 *          failed reads, and unchanged states do not propagate.
 */
void DigitalLightIntensitySensor::processEvent(Event &event) {
    if (event.identifier != MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        return;
    }

    if (!initialized || (!validSample && millis() - conversionStartedAt < BH1750_FIRST_CONVERSION_TIME_IN_MILLISECONDS)) {
        event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
        return;
    }

    float currentIlluminanceInLux = 0.0f;
    if (!readIlluminanceInLux(currentIlluminanceInLux)) {
        reportReadFailure(event);
        if (event.identifier != Sensor::SENSOR_FAULT_EVENT_IDENTIFIER) {
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
        }
        return;
    }

    cachedIlluminanceInLux = currentIlluminanceInLux;
    if (const bool currentState = currentIlluminanceInLux >= lightDetectedThresholdInLux;
        !validSample || currentState != lastState) {
        reportReadSuccess(event);
        validSample = true;
        lastState = currentState;
        if (currentState) {
            event = Event(LIGHT_DETECTED_EVENT_IDENTIFIER, pin);
            return;
        }
        event = Event(LIGHT_ABSENT_EVENT_IDENTIFIER, pin);
        return;
    }

    reportReadSuccess(event);
    event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
}

bool DigitalLightIntensitySensor::isInitialized() const {
    return initialized;
}

I2cResult DigitalLightIntensitySensor::getLastI2cResult() const {
    return lastI2cResult;
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

uint32_t DigitalLightIntensitySensor::getDefaultSamplingIntervalInMilliseconds() const {
    return DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

uint32_t DigitalLightIntensitySensor::getMinimalSamplingIntervalInMilliseconds() const {
    return MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS;
}
