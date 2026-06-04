#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include "Sensor.h"
#include <DHT.h>

/**
 * @brief Concrete sensor for DHT temperature and humidity readings.
 *
 * The sensor reacts to a measurement request event, reads the underlying DHT device, caches the
 * latest valid values, and emits a data-read event when fresh data is available.
 */
class DhtSensor : public Sensor {
private:
    /**
     * @brief Wrapped hardware driver instance.
     */
    DHT physicalDhtSensor;
    /**
     * @brief Last successfully read temperature in Celsius.
     */
    float cachedTemperatureInCelsius;
    /**
     * @brief Last successfully read relative humidity in percent.
     */
    float cachedRelativeHumidityInPercentage;

    /**
     * @brief Internal event marker used to suppress propagation when a measurement fails.
     */
    static const int SENSOR_ERROR_SUPPRESSION_IDENTIFIER = -1;

protected:
    /**
     * @brief Handles a measurement request by reading and caching the physical DHT sensor.
     * @param event Event being processed.
     */
    void processEvent(Event& event) override;

public:


    /**
     * @brief Creates a DHT sensor wrapper and starts the underlying hardware driver.
     * @param signalGpioPin GPIO pin connected to the DHT sensor.
     * @param dhtModelType DHT model constant expected by the Adafruit DHT library.
     * @param parentHandler Optional handler to receive sensor events.
     */
    DhtSensor(int signalGpioPin, uint8_t dhtModelType, EventHandler* parentHandler = nullptr);

    /**
     * @brief Returns the most recently cached temperature value.
     * @return Temperature in Celsius, or the default sentinel value if no valid reading exists yet.
     */
    float getTemperatureInCelsius() const;

    /**
     * @brief Returns the most recently cached relative humidity value.
     * @return Relative humidity in percent, or the default sentinel value if no valid reading exists yet.
     */
    float getRelativeHumidityInPercentage() const;
};

#endif // DHT_SENSOR_H
