/**
 * @file DhtSensor.cpp
 * @brief Implements DHT sampling with native single-wire protocol.
 * @date 2026-06-06
 * @version 1.0
 */

#include "DhtSensor.h"

DhtSensor::DhtSensor(const int signalGpioPin, const uint8_t dhtModelType, EventHandler* parentHandler)
    : Sensor(signalGpioPin, parentHandler),
      dhtModelType(dhtModelType),
      cachedTemperatureInCelsius(-999.0f),
      cachedRelativeHumidityInPercentage(-999.0f)
{
    pinMode(pin, INPUT_PULLUP);
}

bool DhtSensor::sendStartSignal() {
    // MCU pulls the data line LOW to initiate communication
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    delayMicroseconds(dhtModelType == SENSOR_MODEL_DHT11
            ? DHT11_START_SIGNAL_LOW_TIME_IN_MICROSECONDS
            : DHT22_START_SIGNAL_LOW_TIME_IN_MICROSECONDS);

    // MCU releases the line HIGH
    digitalWrite(pin, HIGH);
    delayMicroseconds(DHT_START_SIGNAL_HIGH_TIME_IN_MICROSECONDS);

    // Switch back to input to read sensor response
    pinMode(pin, INPUT_PULLUP);

    // Wait for sensor to pull the line LOW (response start)
    unsigned long start = micros();
    while (digitalRead(pin) == HIGH) {
        if (micros() - start > DHT_BIT_TIMEOUT_IN_MICROSECONDS)
            return false;
    }

    // Wait for sensor to release the line HIGH (response end)
    while (digitalRead(pin) == LOW) {
        if (micros() - start > DHT_BIT_TIMEOUT_IN_MICROSECONDS)
            return false;
    }

    return true;
}

uint8_t DhtSensor::readBit() {
    // Wait for the bit start LOW pulse
    while (digitalRead(pin) == HIGH) {
        // Should not happen in normal operation
    }
    while (digitalRead(pin) == LOW) {
        // Wait for LOW pulse to end
    }

    // Measure the HIGH pulse width to determine bit value
    unsigned long highStart = micros();
    while (digitalRead(pin) == HIGH) {
        if (micros() - highStart > DHT_BIT_TIMEOUT_IN_MICROSECONDS)
            return 0;
    }

    const unsigned long highDuration = micros() - highStart;
    return (highDuration > DHT_BIT_0_HIGH_MAX_TIME_IN_MICROSECONDS) ? 1 : 0;
}

bool DhtSensor::readData(uint8_t* data) {
    for (uint8_t byteIndex = 0; byteIndex < DHT_DATA_BYTE_COUNT; ++byteIndex) {
        uint8_t byteValue = 0;
        for (uint8_t bitIndex = 0; bitIndex < 8; ++bitIndex) {
            byteValue = (byteValue << 1) | readBit();
        }
        data[byteIndex] = byteValue;
    }
    return true;
}

bool DhtSensor::validateChecksum(const uint8_t* data) {
    const uint8_t checksum = static_cast<uint8_t>(
            data[DHT_HUMIDITY_HIGH_BYTE_INDEX] +
            data[DHT_HUMIDITY_LOW_BYTE_INDEX] +
            data[DHT_TEMPERATURE_HIGH_BYTE_INDEX] +
            data[DHT_TEMPERATURE_LOW_BYTE_INDEX]);
    return checksum == data[DHT_CHECKSUM_BYTE_INDEX];
}

void DhtSensor::processEvent(Event& event) {
    if (event.identifier == MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        uint8_t rawData[DHT_DATA_BYTE_COUNT] = {};

        if (!sendStartSignal() || !readData(rawData) || !validateChecksum(rawData)) {
            reportReadFailure(event);
            if (event.identifier != Sensor::SENSOR_FAULT_EVENT_IDENTIFIER) {
                event.identifier = SENSOR_ERROR_SUPPRESSION_IDENTIFIER;
            }
            return;
        }

        // Parse humidity and temperature based on sensor model
        const float rawHumidity = static_cast<float>(
                (rawData[DHT_HUMIDITY_HIGH_BYTE_INDEX] << 8) | rawData[DHT_HUMIDITY_LOW_BYTE_INDEX]);
        const float rawTemperature = static_cast<float>(
                (rawData[DHT_TEMPERATURE_HIGH_BYTE_INDEX] << 8) | rawData[DHT_TEMPERATURE_LOW_BYTE_INDEX]);

        // DHT22/DHT21 return values × 10 (one decimal place); DHT11 returns integer values
        const float humidity = (dhtModelType == SENSOR_MODEL_DHT11)
                ? static_cast<float>(rawData[DHT_HUMIDITY_HIGH_BYTE_INDEX])
                : rawHumidity / 10.0f;
        const float temperature = (dhtModelType == SENSOR_MODEL_DHT11)
                ? static_cast<float>(rawData[DHT_TEMPERATURE_HIGH_BYTE_INDEX])
                : rawTemperature / 10.0f;

        if (temperature == cachedTemperatureInCelsius && humidity == cachedRelativeHumidityInPercentage) {
            reportReadSuccess(event);
            event.identifier = SENSOR_ERROR_SUPPRESSION_IDENTIFIER;
            return;
        }

        reportReadSuccess(event);
        cachedTemperatureInCelsius = temperature;
        cachedRelativeHumidityInPercentage = humidity;
        event = Event(DATA_READ_EVENT_IDENTIFIER);
    }
}

float DhtSensor::getTemperatureInCelsius() const { return cachedTemperatureInCelsius; }
float DhtSensor::getRelativeHumidityInPercentage() const { return cachedRelativeHumidityInPercentage; }

uint32_t DhtSensor::minimumSamplingIntervalFor(const uint8_t dhtModelType) {
    if (dhtModelType == SENSOR_MODEL_DHT11) {
        return DHT11_MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS;
    }
    // DHT22, DHT21, AM2301 all have 2000 ms minimum
    return DHT22_MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

uint32_t DhtSensor::getDefaultSamplingIntervalInMilliseconds() const {
    return DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

uint32_t DhtSensor::getMinimalSamplingIntervalInMilliseconds() const {
    return minimumSamplingIntervalFor(dhtModelType);
}
