#ifndef ARDUINO_I2C_BUS_H
#define ARDUINO_I2C_BUS_H

/**
 * @file ArduinoI2cBus.h
 * @brief Declares the Arduino-ESP32 implementation of the shared I2C bus contract.
 * @ingroup hardware_io_module
 */

#include "I2cBus.h"
#include <Wire.h>
#include <mutex>

/**
 * @brief Arduino-ESP32 `TwoWire` implementation of `I2cBus`.
 *
 * Transfer failures are returned to the caller without automatic restart or retry.
 * `TwoWire::end()`/`begin()` reinitializes the driver but does not pulse SCL to clear a
 * device holding the physical bus low. The configured timeout is applied to individual
 * `TwoWire` transfers, but waiting for the transaction mutex has no hard time bound or
 * fairness guarantee. An incomplete `requestFrom()` is reported as `ShortRead` because
 * the supported API exposes a byte count rather than a distinct read-timeout result.
 * Starting again with the same configuration is idempotent; a different configuration
 * is rejected after startup rather than silently reconfiguring the active bus.
 * @ingroup hardware_io_module
 */
class ArduinoI2cBus final : public I2cBus {
public:
    /**
     * @brief Creates a backend around a caller-owned Arduino @c TwoWire instance.
     * @param wire Bus instance to use; defaults to the global @c Wire object.
     * @note The supplied @c TwoWire instance must outlive this backend.
     */
    explicit ArduinoI2cBus(TwoWire& wire = Wire);
    ~ArduinoI2cBus() override = default;

    ArduinoI2cBus(const ArduinoI2cBus&) = delete;
    ArduinoI2cBus& operator=(const ArduinoI2cBus&) = delete;

    /** @copydoc I2cBus::start */
    I2cStatus start(const I2cBusConfiguration& configuration) override;
    /** @copydoc I2cBus::isReady */
    bool isReady() const override;
    /** @copydoc I2cBus::write */
    I2cResult write(std::uint8_t address, const std::uint8_t* bytes,
                    std::size_t length) override;
    /** @copydoc I2cBus::writeRead */
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
