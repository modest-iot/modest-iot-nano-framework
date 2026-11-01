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
    void begin() {}
    void beginTransmission(uint8_t) {}
    void write(uint8_t command) { commands.push_back(command); }
    int endTransmission() { return static_cast<int>(commands.size()) == failureAt ? 1 : 0; }
    uint8_t requestFrom(uint8_t, uint8_t) { ++requests; readIndex = 0; return byteCount; }
    int read() {
        const int index = readIndex++;
        if (index == readFailureAt) {
            return -1;
        }
        return index == 0 ? raw >> 8 : raw & 0xff;
    }
};
extern FakeWire Wire;