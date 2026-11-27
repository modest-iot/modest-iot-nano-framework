#pragma once
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <mutex>

class FakeWire {
public:
    uint8_t commands[64];
    int commandCount = 0;
    int failureAt = -1;
    int requests = 0;
    int byteCount = 2;
    int readFailureAt = -1;
    uint16_t raw = 0;
    int readIndex = 0;
    int transmissionCount = 0; ///< Tracks number of beginTransmission calls (W19).
    uint8_t readBuffer[64]; ///< Pre-loaded data for multi-byte reads (PthSensor).
    int readBufferSize = 0; ///< Size of readBuffer.
    bool useReadBuffer = false; ///< If true, read() returns from readBuffer instead of raw.
    bool beginResult = true;
    std::uint16_t timeOutMilliseconds = 0;
    int timeOutSetCount = 0;
    int transmissionResult = 0;
    int writeLimit = -1;
    int beginCount = 0;
    int endTransmissionCount = 0;
    int operationCount = 0;
    int requestedLength = 0;
    uint8_t lastTransmissionAddress = 0;
    uint8_t lastRequestAddress = 0;
    bool lastStop = true;
    bool lastEndTransmissionStop = true;
    int lastSdaPin = -1;
    int lastSclPin = -1;
    uint32_t lastFrequency = 0;
    char operations[128]{};

    void pushReadBuffer(uint8_t b) { if (readBufferSize < 64) readBuffer[readBufferSize++] = b; }
    void clearCommands() { commandCount = 0; }
    void clearReadBuffer() { readBufferSize = 0; }

    void blockNextOperation(const char operation) {
        std::lock_guard<std::mutex> lock(blockingState->mutex);
        blockingState->blockedOperation = operation;
        blockingState->operationBlocked = false;
        blockingState->operationReleased = false;
        blockingState->shouldBlockOperation = true;
    }

    bool waitUntilBlocked(const std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(blockingState->mutex);
        return blockingState->condition.wait_for(lock, timeout, [this] {
            return blockingState->operationBlocked;
        });
    }

    void releaseOperation() {
        {
            std::lock_guard<std::mutex> lock(blockingState->mutex);
            blockingState->operationReleased = true;
        }
        blockingState->condition.notify_all();
    }

    bool begin() {
        ++beginCount;
        recordOperation('B');
        return beginResult;
    }

    bool begin(const int sdaPin, const int sclPin, const uint32_t frequencyHz) {
        ++beginCount;
        lastSdaPin = sdaPin;
        lastSclPin = sclPin;
        lastFrequency = frequencyHz;
        recordOperation('B');
        return beginResult;
    }

    void setTimeOut(const std::uint16_t timeoutMilliseconds) {
        timeOutMilliseconds = timeoutMilliseconds;
        ++timeOutSetCount;
    }

    void beginTransmission(const uint8_t address) {
        ++transmissionCount;
        lastTransmissionAddress = address;
        recordOperation('T');
    }

    std::size_t write(const uint8_t command) {
        recordOperation('W');
        if (writeLimit >= 0 && commandCount >= writeLimit) return 0;
        if (commandCount < static_cast<int>(sizeof(commands))) {
            commands[commandCount] = command;
        }
        ++commandCount;
        return 1;
    }

    std::size_t write(const uint8_t* bytes, const std::size_t length) {
        if (bytes == nullptr) return 0;
        std::size_t written = 0;
        while (written < length && write(bytes[written]) == 1) {
            ++written;
        }
        return written;
    }

    [[nodiscard]] int endTransmission(const bool stop = true) {
        ++endTransmissionCount;
        lastStop = stop;
        lastEndTransmissionStop = stop;
        recordOperation('E');
        waitAtOperation('E');
        if (commandCount == failureAt) return 1;
        return transmissionResult;
    }

    std::size_t requestFrom(const uint8_t address, const std::size_t quantity,
                            const bool stop = true) {
        ++requests;
        lastRequestAddress = address;
        requestedLength = static_cast<int>(quantity);
        lastStop = stop;
        recordOperation('R');
        readIndex = 0;
        const std::size_t available = useReadBuffer
                                          ? static_cast<std::size_t>(readBufferSize)
                                          : static_cast<std::size_t>(byteCount);
        return available < quantity ? available : quantity;
    }

    int available() { return useReadBuffer ? readBufferSize - readIndex : byteCount - readIndex; }
    int read() {
        const int index = readIndex++;
        if (index == readFailureAt) {
            return -1;
        }
        if (useReadBuffer && index < readBufferSize) {
            return readBuffer[index];
        }
        return index == 0 ? (raw >> 8) : (raw & 0xFF);
    }

    void clearHistory() {
        operationCount = 0;
        endTransmissionCount = 0;
        requests = 0;
        transmissionCount = 0;
        lastEndTransmissionStop = true;
        for (char& operation : operations) operation = '\0';
    }

private:
    struct BlockingState {
        std::mutex mutex;
        std::condition_variable condition;
        char blockedOperation = '\0';
        bool shouldBlockOperation = false;
        bool operationBlocked = false;
        bool operationReleased = false;
    };

    std::shared_ptr<BlockingState> blockingState = std::make_shared<BlockingState>();

    void recordOperation(const char operation) {
        if (operationCount < static_cast<int>(sizeof(operations))) {
            operations[operationCount++] = operation;
        }
    }

    void waitAtOperation(const char operation) {
        std::unique_lock<std::mutex> lock(blockingState->mutex);
        if (!blockingState->shouldBlockOperation || blockingState->blockedOperation != operation) {
            return;
        }

        blockingState->shouldBlockOperation = false;
        blockingState->operationBlocked = true;
        blockingState->condition.notify_all();
        blockingState->condition.wait(lock, [this] { return blockingState->operationReleased; });
    }
};

using TwoWire = FakeWire;
inline FakeWire Wire;
