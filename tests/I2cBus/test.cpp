#include <cassert>
#include <chrono>
#include <cstdint>
#include <future>
#include <thread>

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
    assert(wire.lastFrequency == TEST_CONFIG.frequencyInHertz);
    assert(wire.timeOutMilliseconds == TEST_CONFIG.timeoutInMilliseconds);
    assert(wire.timeOutSetCount == 1);
    assert(bus.start(TEST_CONFIG) == I2cStatus::Ok);
    assert(wire.beginCount == 1);
    assert(wire.timeOutSetCount == 1);

    const I2cBusConfiguration otherConfig{19, 23, 100000};
    assert(bus.start(otherConfig) == I2cStatus::InvalidArgument);
    assert(wire.beginCount == 1);

    const I2cBusConfiguration differentTimeout{21, 22, 400000, 25};
    assert(bus.start(differentTimeout) == I2cStatus::InvalidArgument);
    assert(wire.timeOutSetCount == 1);
}

void testStartRejectsZeroTimeout() {
    FakeWire wire;
    ArduinoI2cBus bus(wire);
    const I2cBusConfiguration invalidConfig{21, 22, 400000, 0};

    assert(bus.start(invalidConfig) == I2cStatus::InvalidArgument);
    assert(wire.beginCount == 0);
    assert(wire.timeOutSetCount == 0);
}

void testStartAppliesConfiguredTimeout() {
    FakeWire wire;
    ArduinoI2cBus bus(wire);
    const I2cBusConfiguration config{21, 22, 400000, 25};

    assert(bus.start(config) == I2cStatus::Ok);
    assert(wire.timeOutMilliseconds == config.timeoutInMilliseconds);
    assert(wire.timeOutSetCount == 1);
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
    assert(result.transferred == 0);
    assert(result.expectedTransferred == sizeof(command));

    wire.transmissionResult = 4;
    result = bus.write(0x23, command, sizeof(command));
    assert(result.status == I2cStatus::BusError);
}

void testTransferFailuresDoNotAutomaticallyRestartOrRetry() {
    FakeWire wire;
    ArduinoI2cBus bus(wire);
    const std::uint8_t command = 0x10;
    const std::uint8_t registerAddress = 0x12;
    std::uint8_t received = 0;
    assert(bus.start(TEST_CONFIG) == I2cStatus::Ok);

    wire.transmissionResult = 5;
    I2cResult result = bus.write(0x23, &command, 1);
    assert(result.status == I2cStatus::Timeout);
    assert(bus.isReady());
    assert(wire.beginCount == 1);
    assert(wire.transmissionCount == 1);
    assert(wire.endTransmissionCount == 1);

    wire.transmissionResult = 0;
    wire.byteCount = 0;
    result = bus.writeRead(0x23, &registerAddress, 1, &received, 1);
    assert(result.status == I2cStatus::ShortRead);
    assert(wire.requests == 1);
    assert(wire.beginCount == 1);
    assert(wire.transmissionCount == 2);
    assert(wire.endTransmissionCount == 2);
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

    wire.useReadBuffer = false;
    wire.byteCount = 0;
    result = bus.writeRead(0x23, &registerAddress, 1, received, sizeof(received));
    assert(result.status == I2cStatus::ShortRead);
    assert(result.transferred == 0);
    assert(result.expectedTransferred == sizeof(received));
}

void testWriteReadSerializesConcurrentTransactions() {
    FakeWire wire;
    wire.useReadBuffer = true;
    wire.pushReadBuffer(0xAB);
    wire.pushReadBuffer(0xCD);
    ArduinoI2cBus bus(wire);
    const std::uint8_t registerAddress = 0x12;
    const std::uint8_t command = 0x34;
    std::uint8_t received[2]{};
    assert(bus.start(TEST_CONFIG) == I2cStatus::Ok);
    wire.clearHistory();
    wire.blockNextOperation('E');

    I2cResult combinedResult{};
    I2cResult writeResult{};
    std::promise<void> combinedCallFinished;
    std::promise<void> secondCallStarted;
    std::promise<void> secondCallFinished;
    std::future<void> combinedFinished = combinedCallFinished.get_future();
    std::future<void> secondStarted = secondCallStarted.get_future();
    std::future<void> secondFinished = secondCallFinished.get_future();

    std::thread combinedCall([&] {
        combinedResult = bus.writeRead(0x23, &registerAddress, 1, received, sizeof(received));
        combinedCallFinished.set_value();
    });
    const bool combinedCallBlocked = wire.waitUntilBlocked(std::chrono::seconds(1));
    if (!combinedCallBlocked) {
        wire.releaseOperation();
        combinedCall.join();
        assert(combinedCallBlocked);
        return;
    }

    std::thread secondCall([&] {
        secondCallStarted.set_value();
        writeResult = bus.write(0x23, &command, 1);
        secondCallFinished.set_value();
    });
    secondStarted.wait();
    const bool combinedCallFinishedBeforeRelease =
        combinedFinished.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
    const bool secondCallFinishedBeforeRelease =
        secondFinished.wait_for(std::chrono::milliseconds(50)) == std::future_status::ready;

    wire.releaseOperation();
    combinedCall.join();
    secondCall.join();

    assert(!combinedCallFinishedBeforeRelease);
    assert(!secondCallFinishedBeforeRelease);
    assert(combinedFinished.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready);
    assert(secondFinished.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready);
    assert(combinedResult.ok());
    assert(writeResult.ok());
    assert(received[0] == 0xAB && received[1] == 0xCD);
    assert(wire.operationCount == 7);
    assert(wire.operations[0] == 'T');
    assert(wire.operations[1] == 'W');
    assert(wire.operations[2] == 'E');
    assert(wire.operations[3] == 'R');
    assert(wire.operations[4] == 'T');
    assert(wire.operations[5] == 'W');
    assert(wire.operations[6] == 'E');
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
    testStartRejectsZeroTimeout();
    testStartAppliesConfiguredTimeout();
    testStartFailureLeavesBusNotReady();
    testWriteReportsTransferAndNackStatus();
    testTransferFailuresDoNotAutomaticallyRestartOrRetry();
    testWriteReadIsCombinedAndChecksShortReads();
    testWriteReadSerializesConcurrentTransactions();
    testInvalidArgumentsAndPartialWritesAreObservable();
    return 0;
}