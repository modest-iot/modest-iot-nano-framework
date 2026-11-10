#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

/**
 * @file DhtSensor.h
 * @brief Declares a DHT temperature and humidity sensor with native single-wire protocol.
 * @ingroup sensors_module
 * @date 2026-06-06
 * @version 1.0
 */

#include "Sensor.h"

/**
 * @brief Sensor adapter for DHT-family devices (temperature and humidity).
 *
 * Implements the DHT single-wire protocol natively using digitalRead/digitalWrite
 * with microsecond-level timing. Supports DHT11, DHT22, DHT21, and AM2301 models.
 *
 * @ingroup sensors_module
 */
class DhtSensor : public Sensor {
public:
    /// DHT11 model type identifier.
    static constexpr uint8_t SENSOR_MODEL_DHT11 = 11;
    /// DHT22 model type identifier.
    static constexpr uint8_t SENSOR_MODEL_DHT22 = 22;
    /// DHT21 model type identifier.
    static constexpr uint8_t SENSOR_MODEL_DHT21 = 21;
    /// AM2301 model type identifier, equivalent to DHT21.
    static constexpr uint8_t SENSOR_MODEL_AM2301 = 21;

private:
    uint8_t dhtModelType; ///< DHT model constant (DHT11, DHT22, DHT21, AM2301).
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

    // DHT single-wire protocol timing constants (microseconds)
    static constexpr uint32_t DHT22_START_SIGNAL_LOW_TIME_IN_MICROSECONDS = 1000; ///< DHT22 start signal LOW duration.
    static constexpr uint32_t DHT11_START_SIGNAL_LOW_TIME_IN_MICROSECONDS = 18000; ///< DHT11 start signal LOW duration.
    static constexpr uint32_t DHT_START_SIGNAL_HIGH_TIME_IN_MICROSECONDS = 40; ///< MCU release HIGH duration after start.
    static constexpr uint32_t DHT_RESPONSE_LOW_TIME_IN_MICROSECONDS = 80; ///< Sensor response LOW duration.
    static constexpr uint32_t DHT_RESPONSE_HIGH_TIME_IN_MICROSECONDS = 80; ///< Sensor response HIGH duration.
    static constexpr uint32_t DHT_BIT_LOW_TIME_IN_MICROSECONDS = 50; ///< Bit start LOW duration.
    static constexpr uint32_t DHT_BIT_0_HIGH_MAX_TIME_IN_MICROSECONDS = 40; ///< Maximum HIGH duration for bit 0.
    static constexpr uint32_t DHT_BIT_1_HIGH_MAX_TIME_IN_MICROSECONDS = 70; ///< Minimum HIGH duration for bit 1.
    static constexpr uint32_t DHT_BIT_TIMEOUT_IN_MICROSECONDS = 120; ///< Maximum time to wait for a bit.

    // DHT data format constants
    static constexpr uint8_t DHT_DATA_BYTE_COUNT = 5; ///< Total bytes in DHT response (humidity×2 + temp×2 + checksum).
    static constexpr uint8_t DHT_DATA_BIT_COUNT = 40; ///< Total bits in DHT response (5 bytes × 8 bits).
    static constexpr uint8_t DHT_HUMIDITY_HIGH_BYTE_INDEX = 0; ///< Index of humidity high byte.
    static constexpr uint8_t DHT_HUMIDITY_LOW_BYTE_INDEX = 1; ///< Index of humidity low byte.
    static constexpr uint8_t DHT_TEMPERATURE_HIGH_BYTE_INDEX = 2; ///< Index of temperature high byte.
    static constexpr uint8_t DHT_TEMPERATURE_LOW_BYTE_INDEX = 3; ///< Index of temperature low byte.
    static constexpr uint8_t DHT_CHECKSUM_BYTE_INDEX = 4; ///< Index of checksum byte.

    /**
     * @brief Sends the start signal to initiate DHT communication.
     * @return true if the sensor responded with the expected LOW+HIGH pulse, false otherwise.
     * @runs_on sampler task
     */
    bool sendStartSignal();

    /**
     * @brief Reads a single bit from the DHT data line by measuring HIGH pulse width.
     * @return 0 if the HIGH pulse was short (~26-28µs), 1 if long (~70µs).
     * @runs_on sampler task
     */
    uint8_t readBit();

    /**
     * @brief Reads all 40 bits from the DHT sensor and stores them in the provided buffer.
     * @param data Buffer of 5 bytes to store the raw DHT data.
     * @return true if all 40 bits were read successfully, false on timeout.
     * @runs_on sampler task
     */
    bool readData(uint8_t* data);

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
     * @param dhtModelType DHT model constant (for example SENSOR_MODEL_DHT11, SENSOR_MODEL_DHT22).
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

    /**
     * @brief Validates the DHT checksum (sum of first 4 bytes equals the 5th byte).
     * @param data 5-byte buffer containing DHT response data.
     * @return true if checksum is valid, false otherwise.
     */
    static bool validateChecksum(const uint8_t* data);

    /**
     * @brief Returns the minimum sampling interval for the given DHT model.
     * @param dhtModelType DHT model constant (e.g., SENSOR_MODEL_DHT11, SENSOR_MODEL_DHT22).
     * @return Minimum sampling interval in milliseconds.
     * @note DHT22 and DHT21 return 2000 ms; DHT11 returns 1000 ms; unknown models return 2000 ms.
     */
    [[nodiscard]] static uint32_t minimumSamplingIntervalFor(uint8_t dhtModelType);
};

#endif // DHT_SENSOR_H
