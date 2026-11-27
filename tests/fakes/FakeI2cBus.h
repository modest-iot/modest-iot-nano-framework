#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <utility>
#include <vector>

#include "I2cBus.h"

class FakeI2cBus final : public I2cBus {
public:
    struct ReadResponse {
        std::vector<std::uint8_t> bytes;
        I2cStatus status = I2cStatus::Ok;
    };

    I2cStatus startStatus = I2cStatus::Ok;
    I2cStatus nextWriteStatus = I2cStatus::Ok;
    bool ready = false;
    int startCount = 0;
    int writeCount = 0;
    int writeReadCount = 0;
    std::uint8_t lastAddress = 0;
    std::vector<std::uint8_t> lastWrite;
    std::vector<std::uint8_t> lastRegisterPointer;
    std::size_t lastReadLength = 0;
    std::deque<ReadResponse> readResponses;

    I2cStatus start(const I2cBusConfiguration&) override {
        ++startCount;
        ready = startStatus == I2cStatus::Ok;
        return startStatus;
    }

    bool isReady() const override {
        return ready;
    }

    I2cResult write(const std::uint8_t address, const std::uint8_t* bytes,
                    const std::size_t length) override {
        ++writeCount;
        lastAddress = address;
        lastWrite.assign(bytes, bytes + length);
        const I2cStatus status = ready ? nextWriteStatus : I2cStatus::NotStarted;
        nextWriteStatus = I2cStatus::Ok;
        return {status, status == I2cStatus::Ok ? length : 0, length};
    }

    I2cResult writeRead(const std::uint8_t address, const std::uint8_t* txBytes,
                        const std::size_t txLength, std::uint8_t* rxBytes,
                        const std::size_t rxLength) override {
        ++writeReadCount;
        lastAddress = address;
        lastRegisterPointer.clear();
        if (txLength > 0) {
            lastRegisterPointer.assign(txBytes, txBytes + txLength);
        }
        lastReadLength = rxLength;
        if (!ready) {
            return {I2cStatus::NotStarted, 0, rxLength};
        }

        if (readResponses.empty()) {
            std::fill(rxBytes, rxBytes + rxLength, 0);
            return {I2cStatus::Ok, rxLength, rxLength};
        }

        ReadResponse response = std::move(readResponses.front());
        readResponses.pop_front();
        const std::size_t count = std::min(rxLength, response.bytes.size());
        std::copy_n(response.bytes.begin(), count, rxBytes);
        const I2cStatus status = response.status == I2cStatus::Ok && count != rxLength
                                     ? I2cStatus::ShortRead
                                     : response.status;
        return {status, count, rxLength};
    }

    void queueRead(std::vector<std::uint8_t> bytes, const I2cStatus status = I2cStatus::Ok) {
        readResponses.push_back({std::move(bytes), status});
    }
};
