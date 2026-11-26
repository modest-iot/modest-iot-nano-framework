#include <cassert>
#include <cstdint>

#include "ArduinoI2cBus.h"

namespace {
constexpr I2cBusConfiguration TEST_CONFIG{21, 22, 400000};

void testNotStartedTransactionsAreRejected() {
    FakeWire wire;
    ArduinoI2cBus bus(wire);
    const std::uint8_t command = 0x01;

    const I2cResult result = bus.write(0x23, &command, 1);

    assert(result.status == I2cStatus::NotStarted);
    assert(result.transferred == 0);
    assert(result.expectedTransferred == 1);
    assert(wire.transmissionCount == 0);
}

void testStartIsIdempotentAndRejectsReconfiguration() {
    FakeWire wire;
    ArduinoI2cBus bus(wire);

    assert(bus.start(TEST_CONFIG) == I2cStatus::Ok);
    assert(bus.isReady());
    assert(wire.beginCount == 1);
    assert(wire.lastSdaPin == TEST_CONFIG.sdaPin);
    assert(wire.lastSclPin == TEST_CONFIG.sclPin);
    assert(wire.lastFrequency == TEST_CONFIG.frequencyHz);
    assert(bus.start(TEST_CONFIG) == I2cStatus::Ok);
    assert(wire.beginCount == 1);

    const I2cBusConfiguration otherConfig{19, 23, 100000};
    assert(bus.start(otherConfig) == I2cStatus::InvalidArgument);
    assert(wire.beginCount == 1);
}

void testStartFailureLeavesBusNotReady() {
    FakeWire wire;
    wire.beginResult = false;
    ArduinoI2cBus bus(wire);

    assert(bus.start(TEST_CONFIG) == I2cStatus::BusError);
    assert(!bus.isReady());
}

void testWriteReportsTransferAndNackStatus() {
    FakeWire wire;
    ArduinoI2cBus bus(wire);
    const std::uint8_t command[] = {0x10, 0x20};
    assert(bus.start(TEST_CONFIG) == I2cStatus::Ok);

    I2cResult result = bus.write(0x23, command, sizeof(command));
    assert(result.ok());
    assert(result.transferred == sizeof(command));
    assert(result.expectedTransferred == sizeof(command));
    assert(wire.lastTransmissionAddress == 0x23);

    wire.transmissionResult = 2;
    result = bus.write(0x23, command, sizeof(command));
    assert(result.status == I2cStatus::AddressNack);
    assert(result.isInvalidOrIncomplete());
    assert(result.expectedTransferred == sizeof(command));

    wire.transmissionResult = 3;
    result = bus.write(0x23, command, sizeof(command));
    assert(result.status == I2cStatus::DataNack);

    wire.transmissionResult = 5;
    result = bus.write(0x23, command, sizeof(command));
    assert(result.status == I2cStatus::Timeout);

    wire.transmissionResult = 4;
    result = bus.write(0x23, command, sizeof(command));
    assert(result.status == I2cStatus::BusError);
}

void testWriteReadIsCombinedAndChecksShortReads() {
    FakeWire wire;
    wire.useReadBuffer = true;
    wire.pushReadBuffer(0xAB);
    wire.pushReadBuffer(0xCD);
    ArduinoI2cBus bus(wire);
    const std::uint8_t registerAddress = 0x12;
    std::uint8_t received[2]{};
    assert(bus.start(TEST_CONFIG) == I2cStatus::Ok);
    wire.clearHistory();

    I2cResult result = bus.writeRead(0x23, &registerAddress, 1, received, sizeof(received));

    assert(result.ok());
    assert(result.transferred == sizeof(received));
    assert(result.expectedTransferred == sizeof(received));
    assert(received[0] == 0xAB && received[1] == 0xCD);
    assert(wire.lastTransmissionAddress == 0x23);
    assert(wire.lastRequestAddress == 0x23);
    assert(wire.operations[0] == 'T');
    assert(wire.operations[1] == 'W');
    assert(wire.operations[2] == 'E');
    assert(wire.operations[3] == 'R');
    assert(!wire.lastEndTransmissionStop);
    assert(wire.lastStop);

    wire.clearReadBuffer();
    wire.pushReadBuffer(0xEF);
    result = bus.writeRead(0x23, &registerAddress, 1, received, sizeof(received));
    assert(result.status == I2cStatus::ShortRead);
    assert(result.transferred == 1);
    assert(result.expectedTransferred == sizeof(received));
    assert(result.isInvalidOrIncomplete());
}

void testInvalidArgumentsAndPartialWritesAreObservable() {
    FakeWire wire;
    ArduinoI2cBus bus(wire);
    const std::uint8_t command = 0x01;
    std::uint8_t response = 0;
    assert(bus.start(TEST_CONFIG) == I2cStatus::Ok);

    assert(bus.write(0x80, &command, 1).status == I2cStatus::InvalidArgument);
    assert(bus.write(0x23, nullptr, 1).status == I2cStatus::InvalidArgument);
    assert(bus.write(0x23, &command, 0).status == I2cStatus::InvalidArgument);
    assert(bus.writeRead(0x23, nullptr, 1, &response, 1).status == I2cStatus::InvalidArgument);
    assert(bus.writeRead(0x23, nullptr, 0, nullptr, 1).status == I2cStatus::InvalidArgument);

    wire.writeLimit = wire.commandCount;
    const I2cResult result = bus.write(0x23, &command, 1);
    assert(result.status == I2cStatus::InvalidArgument);
    assert(result.expectedTransferred == 1);
}
} // namespace

int main() {
    testNotStartedTransactionsAreRejected();
    testStartIsIdempotentAndRejectsReconfiguration();
    testStartFailureLeavesBusNotReady();
    testWriteReportsTransferAndNackStatus();
    testWriteReadIsCombinedAndChecksShortReads();
    testInvalidArgumentsAndPartialWritesAreObservable();
    return 0;
}