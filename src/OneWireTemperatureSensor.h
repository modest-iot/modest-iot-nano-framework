#ifndef ONE_WIRE_TEMPERATURE_SENSOR_H
#define ONE_WIRE_TEMPERATURE_SENSOR_H

/**
 * @file OneWireTemperatureSensor.h
 * @brief Declares a 1-Wire temperature sensor with native DS18B20 protocol.
 * @ingroup sensors_module
 * @date 2026-11-10
 * @version 1.0
 */

#include "Sensor.h"

/**
 * @brief Digital temperature sensor using the 1-Wire protocol (DS18B20-compatible).
 *
 * Implements the 1-Wire single-bus protocol natively using digitalRead/digitalWrite
 * with microsecond-level timing. Supports temperature conversion and scratchpad
 * reading for DS18B20 and compatible devices.
 *
 * @ingroup sensors_module
 */
class OneWireTemperatureSensor : public Sensor {
protected:
    float cachedTemperatureInCelsius; ///< Last successful temperature reading.
    float lastNotifiedTemperatureInCelsius; ///< Last temperature that triggered a DATA_READ_EVENT.

    /// --- Change Detection ---
    static constexpr float TEMPERATURE_CHANGE_EPSILON = 0.1f; ///< Minimum temperature change to emit event (°C).
    static constexpr float INITIAL_CACHED_TEMPERATURE_IN_CELSIUS = 0.0f; ///< Initial cached temperature value.
    static constexpr float INITIAL_LAST_NOTIFIED_TEMPERATURE_IN_CELSIUS = -1.0f; ///< Initial last-notified sentinel (protected for tests).
    static constexpr int PROPAGATION_SUPPRESSION_IDENTIFIER = -1; ///< Identifier for propagation suppression (protected for tests).

    /// --- Sampling Intervals ---
    static constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS = 1000; ///< Default sampling interval.
    static constexpr uint32_t MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 100; ///< Minimum sampling interval.

private:
    /// --- 1-Wire Protocol Timing Constants (microseconds) ---
    /// @note All timing values from the DS18B20 datasheet, 1-Wire Specification.
    static constexpr uint32_t ONE_WIRE_RESET_PULSE_LOW_TIME_IN_MICROSECONDS = 480; ///< Master reset pulse LOW duration.
    static constexpr uint32_t ONE_WIRE_RESET_PULSE_HIGH_TIME_IN_MICROSECONDS = 70; ///< Master release HIGH duration after reset pulse.
    static constexpr uint32_t ONE_WIRE_PRESENCE_DETECT_TIME_IN_MICROSECONDS = 240; ///< Slave presence pulse LOW duration.
    static constexpr uint32_t ONE_WIRE_WRITE_0_SLOT_TIME_IN_MICROSECONDS = 60; ///< Write-0 slot total duration.
    static constexpr uint32_t ONE_WIRE_WRITE_1_SLOT_TIME_IN_MICROSECONDS = 6; ///< Write-1 slot LOW duration.
    static constexpr uint32_t ONE_WIRE_WRITE_RECOVERY_TIME_IN_MICROSECONDS = 2; ///< Recovery gap between write slots.
    static constexpr uint32_t ONE_WIRE_READ_SAMPLE_TIME_IN_MICROSECONDS = 9; ///< Read slot sample time after master pull-down.
    static constexpr uint32_t ONE_WIRE_READ_SLOT_TIME_IN_MICROSECONDS = 60; ///< Read slot total duration.

    /// --- DS18B20 Command Constants ---
    static constexpr uint8_t DS18B20_CONVERT_T_COMMAND = 0x44; ///< Start temperature conversion.
    static constexpr uint8_t DS18B20_READ_SCRATCHPAD_COMMAND = 0xBE; ///< Read scratchpad (9 bytes).
    static constexpr uint8_t DS18B20_SKIP_ROM_COMMAND = 0xCC; ///< Skip ROM (address all devices on bus).

    /// --- Scratchpad Layout ---
    static constexpr uint8_t DS18B20_SCRATCHPAD_BYTE_COUNT = 9; ///< Total bytes in DS18B20 scratchpad.
    static constexpr uint8_t DS18B20_TEMPERATURE_LSB_BYTE_INDEX = 0; ///< Index of temperature LSB byte.
    static constexpr uint8_t DS18B20_TEMPERATURE_MSB_BYTE_INDEX = 1; ///< Index of temperature MSB byte.

    /// --- Temperature Conversion ---
    static constexpr float DS18B20_TEMPERATURE_RESOLUTION = 0.0625f; ///< Temperature resolution (°C per LSB at 12-bit).
    static constexpr uint32_t DS18B20_CONVERSION_TIME_IN_MILLISECONDS = 750; ///< Max conversion time at 12-bit resolution.

    /**
     * @brief Sends a 1-Wire reset pulse and checks for presence.
     * @return true if a device responded with a presence pulse, false otherwise.
     * @runs_on sampler task
     */
    bool sendResetPulse() const;

    /**
     * @brief Writes a single bit on the 1-Wire bus.
     * @param bitValue Bit to write (0 or 1).
     * @runs_on sampler task
     */
    void writeBit(bool bitValue) const;

    /**
     * @brief Reads a single bit from the 1-Wire bus.
     * @return true if the bit is 1, false if the bit is 0.
     * @runs_on sampler task
     */
    bool readBit() const;

    /**
     * @brief Writes a byte on the 1-Wire bus (LSB first).
     * @param data Byte to write.
     * @runs_on sampler task
     */
    void writeByte(uint8_t data) const;

    /**
     * @brief Reads a byte from the 1-Wire bus (LSB first).
     * @return The byte read from the bus.
     * @runs_on sampler task
     */
    uint8_t readByte() const;

    /**
     * @brief Starts a temperature conversion and waits for completion.
     * @runs_on sampler task
     */
    void startTemperatureConversion() const;

    /**
     * @brief Reads the scratchpad and computes the temperature.
     * @return Temperature in degrees Celsius.
     * @runs_on sampler task
     */
    float readTemperatureFromScratchpad() const;

protected:
    /**
     * @brief Performs a temperature read and updates cached measurement.
     * @param event Mutable event request/response payload.
     */
    void processEvent(Event& event) override;

    [[nodiscard]] uint32_t getDefaultSamplingIntervalInMilliseconds() const override;
    [[nodiscard]] uint32_t getMinimalSamplingIntervalInMilliseconds() const override;

public:
    /**
     * @brief Constructs a 1-Wire temperature sensor.
     * @param dataGpioPin GPIO pin connected to the 1-Wire data line.
     * @param parentHandler Optional upstream handler for propagated events.
     */
    OneWireTemperatureSensor(int dataGpioPin, EventHandler* parentHandler = nullptr);

    /**
     * @brief Returns the most recent successful temperature reading.
     * @return Temperature in degrees Celsius.
     */
    [[nodiscard]] float getTemperatureInCelsius() const;
};

#endif // ONE_WIRE_TEMPERATURE_SENSOR_H
