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
 * @brief Sensor adapter for DHT-family devices (temperature and humidity).
 * @ingroup sensors_module
 */
class DhtSensor : public Sensor {
public:
    /// DHT11 model type identifier, matches DHT library constant `DHT11`.
    static constexpr uint8_t SENSOR_MODEL_DHT11 = 11;
    /// DHT22 model type identifier, matches DHT library constant `DHT22`.
    static constexpr uint8_t SENSOR_MODEL_DHT22 = 22;
    /// DHT21 model type identifier, matches DHT library constant `DHT21`.
    static constexpr uint8_t SENSOR_MODEL_DHT21 = 21;
    /// AM2301 model type identifier, equivalent to DHT21.
    static constexpr uint8_t SENSOR_MODEL_AM2301 = 21;

private:
    DHT physicalDhtSensor; ///< Underlying DHT sensor instance.
    float cachedTemperatureInCelsius; ///< Last successful temperature reading.
    float cachedRelativeHumidityInPercentage; ///< Last successful humidity reading.

    static constexpr int SENSOR_ERROR_SUPPRESSION_IDENTIFIER = -1; ///< Identifier used to suppress error events.

    /// Minimum sampling interval for DHT11 model in milliseconds.
    /// @note DHT11 specification limits readings to 1 Hz (1000 ms).
    static constexpr uint32_t DHT11_MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 1000;
    /// Minimum sampling interval for DHT22/DHT21 model in milliseconds.
    /// @note DHT22 specification limits readings to 0.5 Hz (2000 ms).
    static constexpr uint32_t DHT22_MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 2000;
    /// Default sampling interval in milliseconds (conservative, covers all models).
    static constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS = 2000;

    /**
     * @brief Returns the minimum sampling interval for the given DHT model.
     * @param dhtModelType DHT library model constant (e.g., `DHT11`, `DHT22`).
     * @return Minimum sampling interval in milliseconds.
     * @note DHT22 and DHT21 return 2000 ms; DHT11 returns 1000 ms; unknown models return 2000 ms.
     */
    [[nodiscard]] static uint32_t minimumSamplingIntervalFor(uint8_t dhtModelType);

protected:
    /**
     * @brief Performs a DHT read cycle and updates cached measurements.
     * @param event Mutable event request/response payload.
     */
    void processEvent(Event& event) override;

    [[nodiscard]] uint32_t getDefaultSamplingIntervalInMilliseconds() const override;
    [[nodiscard]] uint32_t getMinimalSamplingIntervalInMilliseconds() const override;

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
    [[nodiscard]] float getTemperatureInCelsius() const;

    /**
     * @brief Returns the most recent successful relative humidity reading.
     * @return Relative humidity percentage (0-100).
     */
    [[nodiscard]] float getRelativeHumidityInPercentage() const;
};

#endif // DHT_SENSOR_H
