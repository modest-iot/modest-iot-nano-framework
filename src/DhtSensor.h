#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

/**
 * @file DhtSensor.h
 * @brief Declares a DHT temperature and humidity sensor adapter.
 * @ingroup sensors_module
 * @date 2026-06-06
 * @version 1.0
 */

#include "Sensor.h"
#include <DHT.h>

/**
 * @name Sensors: Environmental & Distance
 * @ingroup sensors_module
 * @{
 */

/**
 * @brief Sensor adapter for DHT-family devices (temperature + humidity).
 * @ingroup sensors_module
 */
class DhtSensor : public Sensor {
private:
    DHT physicalDhtSensor;
    float cachedTemperatureInCelsius;
    float cachedRelativeHumidityInPercentage;

    static const int SENSOR_ERROR_SUPPRESSION_IDENTIFIER = -1;

protected:
    /**
     * @brief Performs a DHT read cycle and updates cached measurements.
     * @param event Mutable event request/response payload.
     */
    void processEvent(Event& event) override;

public:
    /**
     * @brief Constructs a DHT sensor wrapper.
     * @param signalGpioPin Data pin connected to the DHT module.
     * @param dhtModelType DHT library model constant (for example `DHT11`, `DHT22`).
     * @param parentHandler Optional upstream handler for propagated events.
     */
    DhtSensor(int signalGpioPin, uint8_t dhtModelType, EventHandler* parentHandler = nullptr);

    /**
     * @brief Returns the most recent successful temperature reading.
     * @return Temperature in degrees Celsius.
     */
    float getTemperatureInCelsius() const;

    /**
     * @brief Returns the most recent successful relative humidity reading.
     * @return Relative humidity percentage (0-100).
     */
    float getRelativeHumidityInPercentage() const;
};

/** @} */ // End of Sensors: Environmental & Distance

#endif // DHT_SENSOR_H