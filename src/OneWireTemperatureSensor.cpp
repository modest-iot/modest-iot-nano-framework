/**
 * @file OneWireTemperatureSensor.cpp
 * @brief Implements the 1-Wire temperature sensor with native DS18B20 protocol.
 * @copyright (c) 2025-2026 Angel Velasquez
 *
 * This file contains the 1-Wire bit-banging protocol implementation for DS18B20
 * and compatible digital temperature sensors. All timing values are from the
 * DS18B20 datasheet and 1-Wire Specification.
 *
 * @ingroup sensors_module
 * @date 2026-11-10
 * @version 1.0
 */

#include <Arduino.h>
#include <cmath>
#include "OneWireTemperatureSensor.h"
/**
 * @brief Constructs a 1-Wire temperature sensor.
 * @param dataGpioPin GPIO pin connected to the 1-Wire data line.
 * @param parentHandler Optional upstream handler for propagated events.
 */
OneWireTemperatureSensor::OneWireTemperatureSensor(
        int dataGpioPin,
        EventHandler* parentHandler)
    : Sensor(dataGpioPin, parentHandler),
      cachedTemperatureInCelsius(INITIAL_CACHED_TEMPERATURE_IN_CELSIUS),
      lastNotifiedTemperatureInCelsius(INITIAL_LAST_NOTIFIED_TEMPERATURE_IN_CELSIUS) {
    pinMode(pin, INPUT_PULLUP);
}

/**
 * @brief Returns the most recent successful temperature reading.
 * @return Temperature in degrees Celsius.
 */
float OneWireTemperatureSensor::getTemperatureInCelsius() const {
    return cachedTemperatureInCelsius;
}

/**
 * @brief Sends a 1-Wire reset pulse and checks for device presence.
 *
 * The master pulls the bus LOW for 480µs, then releases it. A slave device
 * responds by pulling the bus LOW for 60-240µs (presence pulse).
 *
 * @return true if a device responded with a presence pulse, false otherwise.
 * @runs_on{sampler}
 */
bool OneWireTemperatureSensor::sendResetPulse() const {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    delayMicroseconds(ONE_WIRE_RESET_PULSE_LOW_TIME_IN_MICROSECONDS);
    digitalWrite(pin, HIGH);
    pinMode(pin, INPUT_PULLUP);
    delayMicroseconds(ONE_WIRE_RESET_PULSE_HIGH_TIME_IN_MICROSECONDS);
    const bool presenceDetected = (digitalRead(pin) == LOW);
    delayMicroseconds(ONE_WIRE_PRESENCE_DETECT_TIME_IN_MICROSECONDS);
    return presenceDetected;
}

/**
 * @brief Writes a single bit on the 1-Wire bus.
 *
 * For bit 0: pull LOW for the full slot duration (60µs).
 * For bit 1: pull LOW for 6µs, then release HIGH.
 *
 * @param bitValue Bit to write (0 or 1).
 * @runs_on{sampler}
 */
void OneWireTemperatureSensor::writeBit(const bool bitValue) const {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    if (bitValue) {
        delayMicroseconds(ONE_WIRE_WRITE_1_SLOT_TIME_IN_MICROSECONDS);
        digitalWrite(pin, HIGH);
    } else {
        delayMicroseconds(ONE_WIRE_WRITE_0_SLOT_TIME_IN_MICROSECONDS);
        digitalWrite(pin, HIGH);
    }
    pinMode(pin, INPUT_PULLUP);
    delayMicroseconds(ONE_WIRE_WRITE_RECOVERY_TIME_IN_MICROSECONDS);
}

/**
 * @brief Reads a single bit from the 1-Wire bus.
 *
 * The master pulls LOW for 1µs, then releases and samples after 9µs.
 * If the slave is writing a 1, the bus is HIGH. If writing 0, the bus is LOW.
 *
 * @return true if the bit is 1, false if the bit is 0.
 * @runs_on{sampler}
 */
bool OneWireTemperatureSensor::readBit() const {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    digitalWrite(pin, HIGH);
    pinMode(pin, INPUT_PULLUP);
    delayMicroseconds(ONE_WIRE_READ_SAMPLE_TIME_IN_MICROSECONDS);
    const bool bitValue = (digitalRead(pin) == HIGH);
    delayMicroseconds(ONE_WIRE_READ_SLOT_TIME_IN_MICROSECONDS);
    return bitValue;
}

/**
 * @brief Writes a byte on the 1-Wire bus (LSB first).
 * @param data Byte to write.
 * @runs_on{sampler}
 */
void OneWireTemperatureSensor::writeByte(const uint8_t data) const {
    for (uint8_t i = 0; i < 8; i++) {
        writeBit((data >> i) & 0x01);
    }
}

/**
 * @brief Reads a byte from the 1-Wire bus (LSB first).
 * @return The byte read from the bus.
 * @runs_on{sampler}
 */
uint8_t OneWireTemperatureSensor::readByte() const {
    uint8_t data = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if (readBit()) {
            data |= (0x01 << i);
        }
    }
    return data;
}

/**
 * @brief Starts a temperature conversion and waits for completion.
 *
 * Sends SKIP_ROM command (for single-device bus) followed by CONVERT_T command.
 * Then waits for the conversion to complete (up to 750ms at 12-bit resolution).
 *
 * @runs_on{sampler}
 */
void OneWireTemperatureSensor::startTemperatureConversion() const {
    (void)sendResetPulse();
    writeByte(DS18B20_SKIP_ROM_COMMAND);
    writeByte(DS18B20_CONVERT_T_COMMAND);
    delay(DS18B20_CONVERSION_TIME_IN_MILLISECONDS);
}

/**
 * @brief Reads the scratchpad and computes the temperature.
 *
 * Sends SKIP_ROM and READ_SCRATCHPAD commands, then reads the first two bytes
 * (temperature LSB and MSB). Combines them into a 16-bit signed value and
 * converts to Celsius using the 0.0625°C per LSB resolution.
 *
 * @return Temperature in degrees Celsius.
 * @runs_on{sampler}
 */
float OneWireTemperatureSensor::readTemperatureFromScratchpad() const {
    (void)sendResetPulse();
    writeByte(DS18B20_SKIP_ROM_COMMAND);
    writeByte(DS18B20_READ_SCRATCHPAD_COMMAND);
    const uint8_t tempLsb = readByte();
    const uint8_t tempMsb = readByte();
    // Skip remaining scratchpad bytes
    for (uint8_t i = 2; i < DS18B20_SCRATCHPAD_BYTE_COUNT; i++) {
        (void)readByte();
    }
    // Combine into 16-bit signed integer (two's complement)
    int16_t rawTemperature = (static_cast<int16_t>(tempMsb) << 8) | tempLsb;
    return static_cast<float>(rawTemperature) * DS18B20_TEMPERATURE_RESOLUTION;
}

/**
 * @brief Performs a temperature read and updates cached measurement.
 *
 * On MEASURE_DATA_REQUESTED_EVENT, starts a temperature conversion, reads the
 * result, and compares with the last-notified value using epsilon-based change
 * detection. Emits DATA_READ_EVENT only if the temperature changed by more than
 * TEMPERATURE_CHANGE_EPSILON (0.1°C).
 */
void OneWireTemperatureSensor::processEvent(Event& event) {
    if (event.identifier == MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        startTemperatureConversion();
        cachedTemperatureInCelsius = readTemperatureFromScratchpad();
        reportReadSuccess(event);

        if (std::fabs(cachedTemperatureInCelsius - lastNotifiedTemperatureInCelsius)
                > TEMPERATURE_CHANGE_EPSILON) {
            lastNotifiedTemperatureInCelsius = cachedTemperatureInCelsius;
            event.identifier = DATA_READ_EVENT_IDENTIFIER;
            event.sourceId = pin;
        } else {
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
        }
    }
}

/**
 * @brief Returns the default sampling interval.
 * @return 1000 ms (1 second).
 */
uint32_t OneWireTemperatureSensor::getDefaultSamplingIntervalInMilliseconds() const {
    return DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

/**
 * @brief Returns the minimum sampling interval.
 * @return 100 ms.
 */
uint32_t OneWireTemperatureSensor::getMinimalSamplingIntervalInMilliseconds() const {
    return MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS;
}
