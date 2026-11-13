#pragma once
#include <cstdint>

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
    void pushReadBuffer(uint8_t b) { if (readBufferSize < 64) readBuffer[readBufferSize++] = b; }
    void clearCommands() { commandCount = 0; }
    void clearReadBuffer() { readBufferSize = 0; }

    static void begin() {}
    void beginTransmission(const uint8_t) { ++transmissionCount; }
    void write(const uint8_t command) { if (commandCount < 64) commands[commandCount++] = command; }
    [[nodiscard]] int endTransmission(bool stop = false) const {
        (void)stop;
        return commandCount == failureAt ? 1 : 0;
    }
    uint8_t requestFrom(const uint8_t, const uint8_t) { ++requests; readIndex = 0; return useReadBuffer ? static_cast<uint8_t>(readBufferSize) : static_cast<uint8_t>(byteCount); }
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
};

inline FakeWire Wire;
