#ifndef I2C_BUS_H
#define I2C_BUS_H

#include <cstddef>
#include <cstdint>

struct I2cBusConfiguration {
    int sdaPin;
    int sclPin;
    std::uint32_t frequencyInHertz;
};

enum class I2cStatus : std::uint8_t {
    Ok,
    NotStarted,
    InvalidArgument,
    AddressNack,
    DataNack,
    Timeout,
    BusError,
    ShortRead
};

struct I2cResult {
    I2cStatus status;
    std::size_t transferred;
    std::size_t expectedTransferred;

    bool ok() const { return status == I2cStatus::Ok; }
    bool isInvalidOrIncomplete() const {
        return !ok() || transferred != expectedTransferred;
    }
};

class I2cBus {
public:
    virtual ~I2cBus() = default;
    virtual I2cStatus start(const I2cBusConfiguration& configuration) = 0;
    virtual bool isReady() const = 0;
    virtual I2cResult write(std::uint8_t address, const std::uint8_t* bytes,
                            std::size_t length) = 0;
    virtual I2cResult writeRead(std::uint8_t address,
                                const std::uint8_t* txBytes, std::size_t txLength,
                                std::uint8_t* rxBytes, std::size_t rxLength) = 0;
};

#endif // I2C_BUS_H
