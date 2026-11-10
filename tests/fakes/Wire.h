#pragma once
#include <cstdint>
#include <vector>

class FakeWire {
public:
    std::vector<uint8_t> commands;
    int failureAt = -1;
    int requests = 0;
    int byteCount = 2;
    int readFailureAt = -1;
    uint16_t raw = 0;
    int readIndex = 0;
    int transmissionCount = 0; ///< Tracks number of beginTransmission calls (W19).
    std::vector<uint8_t> readBuffer; ///< Pre-loaded data for multi-byte reads (PthSensor).
    bool useReadBuffer = false; ///< If true, read() returns from readBuffer instead of raw.

    static void begin() {}
    void beginTransmission(const uint8_t) { ++transmissionCount; }
    void write(const uint8_t command) { commands.push_back(command); }
    [[nodiscard]] int endTransmission(bool stop = false) const {
        (void)stop;
        return static_cast<int>(commands.size()) == failureAt ? 1 : 0;
    }
    uint8_t requestFrom(const uint8_t, const uint8_t) { ++requests; readIndex = 0; return useReadBuffer ? static_cast<uint8_t>(readBuffer.size()) : static_cast<uint8_t>(byteCount); }
    int available() { return useReadBuffer ? static_cast<int>(readBuffer.size()) - readIndex : byteCount - readIndex; }
    int read() {
        const int index = readIndex++;
        if (index == readFailureAt) {
            return -1;
        }
        if (useReadBuffer && index < static_cast<int>(readBuffer.size())) {
            return readBuffer[index];
        }
        return index == 0 ? (raw >> 8) : (raw & 0xFF);
    }
};

inline FakeWire Wire;
