#include "ArduinoI2cBus.h"

#include <algorithm>

ArduinoI2cBus::ArduinoI2cBus(TwoWire& wireInstance)
    : wire(wireInstance) {}

I2cStatus ArduinoI2cBus::start(const I2cBusConfiguration& configuration) {
    std::lock_guard lock(transactionMutex);
    if (!isValidConfiguration(configuration)) {
        return I2cStatus::InvalidArgument;
    }

    if (started) {
        return configuration.sdaPin == activeConfiguration.sdaPin &&
                       configuration.sclPin == activeConfiguration.sclPin &&
                       configuration.frequencyInHertz == activeConfiguration.frequencyInHertz
                   ? I2cStatus::Ok
                   : I2cStatus::InvalidArgument;
    }

    if (!wire.begin(configuration.sdaPin, configuration.sclPin, configuration.frequencyInHertz)) {
        return I2cStatus::BusError;
    }

    activeConfiguration = configuration;
    started = true;
    return I2cStatus::Ok;
}

bool ArduinoI2cBus::isReady() const {
    std::lock_guard lock(transactionMutex);
    return started;
}

I2cResult ArduinoI2cBus::write(const std::uint8_t address, const std::uint8_t* bytes,
                               const std::size_t length) {
    const std::size_t expectedTransferred = length;
    if (!isValidAddress(address) || bytes == nullptr || length == 0) {
        return makeResult(I2cStatus::InvalidArgument, 0, expectedTransferred);
    }

    std::lock_guard lock(transactionMutex);
    if (!started) {
        return makeResult(I2cStatus::NotStarted, 0, expectedTransferred);
    }

    wire.beginTransmission(address);
    const std::size_t acceptedBytes = wire.write(bytes, length);
    const std::uint8_t transmissionStatus = wire.endTransmission(true);
    if (acceptedBytes != length) {
        return makeResult(I2cStatus::InvalidArgument, 0, expectedTransferred);
    }

    const I2cStatus status = mapTransmissionStatus(transmissionStatus);
    return makeResult(status, status == I2cStatus::Ok ? length : 0, expectedTransferred);
}

I2cResult ArduinoI2cBus::writeRead(const std::uint8_t address,
                                   const std::uint8_t* txBytes, const std::size_t txLength,
                                   std::uint8_t* rxBytes, const std::size_t rxLength) {
    const std::size_t expectedTransferred = rxLength;
    if (!isValidAddress(address) || (txLength > 0 && txBytes == nullptr) ||
        rxBytes == nullptr || rxLength == 0) {
        return makeResult(I2cStatus::InvalidArgument, 0, expectedTransferred);
    }

    std::lock_guard lock(transactionMutex);
    if (!started) {
        return makeResult(I2cStatus::NotStarted, 0, expectedTransferred);
    }

    if (txLength > 0) {
        wire.beginTransmission(address);
        const std::size_t acceptedBytes = wire.write(txBytes, txLength);
        const std::uint8_t transmissionStatus = wire.endTransmission(false);
        if (acceptedBytes != txLength) {
            return makeResult(I2cStatus::InvalidArgument, 0, expectedTransferred);
        }

        if (const I2cStatus status = mapTransmissionStatus(transmissionStatus); status != I2cStatus::Ok) {
            return makeResult(status, 0, expectedTransferred);
        }
    }

    const std::size_t reportedBytes = wire.requestFrom(address, rxLength, true);
    const std::size_t availableBytes = std::min(reportedBytes, rxLength);
    std::size_t receivedBytes = 0;
    while (receivedBytes < availableBytes && wire.available() > 0) {
        const int value = wire.read();
        if (value < 0) {
            break;
        }
        rxBytes[receivedBytes++] = static_cast<std::uint8_t>(value);
    }

    if (reportedBytes != rxLength || receivedBytes != rxLength) {
        return makeResult(I2cStatus::ShortRead, receivedBytes, expectedTransferred);
    }
    return makeResult(I2cStatus::Ok, receivedBytes, expectedTransferred);
}

bool ArduinoI2cBus::isValidConfiguration(const I2cBusConfiguration& configuration) {
    const bool usesDefaultPins = configuration.sdaPin == -1 && configuration.sclPin == -1;
    const bool usesExplicitPins = configuration.sdaPin >= 0 && configuration.sclPin >= 0 &&
                                  configuration.sdaPin != configuration.sclPin;
    return (usesDefaultPins || usesExplicitPins) && configuration.frequencyInHertz > 0;
}

bool ArduinoI2cBus::isValidAddress(const std::uint8_t address) {
    return address <= 0x7F;
}

I2cStatus ArduinoI2cBus::mapTransmissionStatus(const std::uint8_t status) {
    switch (status) {
        case 0:
            return I2cStatus::Ok;
        case 1:
            return I2cStatus::InvalidArgument;
        case 2:
            return I2cStatus::AddressNack;
        case 3:
            return I2cStatus::DataNack;
        case 5:
            return I2cStatus::Timeout;
        default:
            return I2cStatus::BusError;
    }
}

I2cResult ArduinoI2cBus::makeResult(const I2cStatus status,
                                    const std::size_t transferred,
                                    const std::size_t expectedTransferred) {
    return {status, transferred, expectedTransferred};
}
