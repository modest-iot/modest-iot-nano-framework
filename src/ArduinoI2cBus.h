#ifndef ARDUINO_I2C_BUS_H
#define ARDUINO_I2C_BUS_H

#include "I2cBus.h"
#include <Wire.h>
#include <mutex>

/**
 * @brief Arduino-ESP32 `TwoWire` implementation of `I2cBus`.
 *
 * Transfer failures are returned to the caller without automatic restart or retry.
 * `TwoWire::end()`/`begin()` reinitializes the driver but does not pulse SCL to clear a
 * device holding the physical bus low.
 */
class ArduinoI2cBus final : public I2cBus {
public:
    explicit ArduinoI2cBus(TwoWire& wire = Wire);
    ~ArduinoI2cBus() override = default;

    ArduinoI2cBus(const ArduinoI2cBus&) = delete;
    ArduinoI2cBus& operator=(const ArduinoI2cBus&) = delete;

    I2cStatus start(const I2cBusConfiguration& configuration) override;
    bool isReady() const override;
    I2cResult write(std::uint8_t address, const std::uint8_t* bytes,
                    std::size_t length) override;
    I2cResult writeRead(std::uint8_t address,
                        const std::uint8_t* txBytes, std::size_t txLength,
                        std::uint8_t* rxBytes, std::size_t rxLength) override;

private:
    TwoWire& wire;
    mutable std::mutex transactionMutex;
    I2cBusConfiguration activeConfiguration{};
    bool started = false;

    static bool isValidConfiguration(const I2cBusConfiguration& configuration);
    static bool isValidAddress(std::uint8_t address);
    static I2cStatus mapTransmissionStatus(std::uint8_t status);
    static I2cResult makeResult(I2cStatus status, std::size_t transferred,
                                std::size_t expectedTransferred);
};

#endif // ARDUINO_I2C_BUS_H
