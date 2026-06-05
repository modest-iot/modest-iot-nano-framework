#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include "Sensor.h"
#include <DHT.h>

class DhtSensor : public Sensor {
private:
    DHT physicalDhtSensor;
    float cachedTemperatureInCelsius;
    float cachedRelativeHumidityInPercentage;

    static const int SENSOR_ERROR_SUPPRESSION_IDENTIFIER = -1;

protected:
    void processEvent(Event& event) override;

public:
    DhtSensor(int signalGpioPin, uint8_t dhtModelType, EventHandler* parentHandler = nullptr);

    float getTemperatureInCelsius() const;
    float getRelativeHumidityInPercentage() const;
};

#endif // DHT_SENSOR_H