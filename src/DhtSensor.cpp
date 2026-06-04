#include "DhtSensor.h"

const Event DhtSensor::DATA_READ_EVENT = Event(DhtSensor::DATA_READ_EVENT_IDENTIFIER);
const Event DhtSensor::MEASURE_DATA_REQUESTED_EVENT = Event(DhtSensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);

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

        // Guard against hardware bus noise or disconnected sensor faults
        if (isnan(rawHumidity) || isnan(rawTemperature)) {
            event.identifier = SENSOR_ERROR_SUPPRESSION_IDENTIFIER;
            return;
        }

        // CONDITIONAL MUTATION OPTIMIZATION:
        // If the physical environment hasn't changed, kill the event propagation instantly.
        // This saves massive CPU cycles by keeping the Device mediator task sound asleep.
        if (rawTemperature == cachedTemperatureInCelsius && rawHumidity == cachedRelativeHumidityInPercentage) {
            event.identifier = SENSOR_ERROR_SUPPRESSION_IDENTIFIER;
            return;
        }

        // A true physical mutation has occurred! Update the single-source-of-truth cache
        cachedTemperatureInCelsius = rawTemperature;
        cachedRelativeHumidityInPercentage = rawHumidity;

        // Mutate the incoming request event into an upstream data confirmation signal
        event = DATA_READ_EVENT;
    }
}

float DhtSensor::getTemperatureInCelsius() const { return cachedTemperatureInCelsius; }
float DhtSensor::getRelativeHumidityInPercentage() const { return cachedRelativeHumidityInPercentage; }
