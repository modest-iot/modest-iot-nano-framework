/**
 * @file DhtSensor.cpp
 * @brief Implements DHT sampling and cached temperature/humidity updates.
 * @date 2026-06-06
 * @version 1.0
 */

#include "DhtSensor.h"

DhtSensor::DhtSensor(const int signalGpioPin, const uint8_t dhtModelType, EventHandler* parentHandler)
    : Sensor(signalGpioPin, parentHandler),
      physicalDhtSensor(signalGpioPin, dhtModelType),
      cachedTemperatureInCelsius(-999.0f),
      cachedRelativeHumidityInPercentage(-999.0f)
{
    physicalDhtSensor.begin();
}

void DhtSensor::processEvent(Event& event) {
    if (event.identifier == MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        const float rawHumidity = physicalDhtSensor.readHumidity();
        const float rawTemperature = physicalDhtSensor.readTemperature();

        if (isnan(rawHumidity) || isnan(rawTemperature)) {
            reportReadFailure(event);
            if (event.identifier != Sensor::SENSOR_FAULT_EVENT_IDENTIFIER) {
                event.identifier = SENSOR_ERROR_SUPPRESSION_IDENTIFIER;
            }
            return;
        }

        if (rawTemperature == cachedTemperatureInCelsius && rawHumidity == cachedRelativeHumidityInPercentage) {
            reportReadSuccess(event);
            event.identifier = SENSOR_ERROR_SUPPRESSION_IDENTIFIER;
            return;
        }

        reportReadSuccess(event);
        cachedTemperatureInCelsius = rawTemperature;
        cachedRelativeHumidityInPercentage = rawHumidity;
        event = Event(DATA_READ_EVENT_IDENTIFIER);
    }
}

float DhtSensor::getTemperatureInCelsius() const { return cachedTemperatureInCelsius; }
float DhtSensor::getRelativeHumidityInPercentage() const { return cachedRelativeHumidityInPercentage; }

uint32_t DhtSensor::minimumSamplingIntervalFor(uint8_t dhtModelType) {
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
    return DHT22_MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS;
}