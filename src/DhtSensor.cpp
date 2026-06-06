/**
 * @file DhtSensor.cpp
 * @brief Implements DHT sampling and cached temperature/humidity updates.
 */

#include "DhtSensor.h"

DhtSensor::DhtSensor(int signalGpioPin, uint8_t dhtModelType, EventHandler* parentHandler)
    : Sensor(signalGpioPin, parentHandler),
      physicalDhtSensor(signalGpioPin, dhtModelType),
      cachedTemperatureInCelsius(-999.0f),
      cachedRelativeHumidityInPercentage(-999.0f)
{
    physicalDhtSensor.begin();
}

void DhtSensor::processEvent(Event& event) {
    if (event.identifier == MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        float rawHumidity = physicalDhtSensor.readHumidity();
        float rawTemperature = physicalDhtSensor.readTemperature();

        if (isnan(rawHumidity) || isnan(rawTemperature)) {
            event.identifier = SENSOR_ERROR_SUPPRESSION_IDENTIFIER;
            return;
        }

        if (rawTemperature == cachedTemperatureInCelsius && rawHumidity == cachedRelativeHumidityInPercentage) {
            event.identifier = SENSOR_ERROR_SUPPRESSION_IDENTIFIER;
            return;
        }

        cachedTemperatureInCelsius = rawTemperature;
        cachedRelativeHumidityInPercentage = rawHumidity;
        event = DATA_READ_EVENT;
    }
}

float DhtSensor::getTemperatureInCelsius() const { return cachedTemperatureInCelsius; }
float DhtSensor::getRelativeHumidityInPercentage() const { return cachedRelativeHumidityInPercentage; }