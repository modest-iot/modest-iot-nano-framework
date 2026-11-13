#pragma once
#include <time.h>
#include <cstdint>
#include <vector>
#include <unordered_map>

#define MSBFIRST 1
#define SPI_MODE0 0

class SPISettings {
public:
    uint32_t clock;
    uint8_t bitOrder;
    uint8_t dataMode;
    SPISettings() : clock(4000000), bitOrder(MSBFIRST), dataMode(SPI_MODE0) {}
    SPISettings(const uint32_t c, const uint8_t b, const uint8_t d) : clock(c), bitOrder(b), dataMode(d) {}
};

class SPIClass {
public:
    std::unordered_map<uint8_t, uint8_t> registers;
    std::vector<uint8_t> fifoData;
    uint8_t selectedRegister = 0;
    bool isRead = false;
    bool inTransaction = false;
    int transactionByteIndex = 0;

    uint8_t (*transferHook)(uint8_t data, SPIClass* self) = nullptr;

    static void begin() {}
    void beginTransaction(const SPISettings&) {
        inTransaction = true;
        transactionByteIndex = 0;
        selectedRegister = 0;
        isRead = false;
    }
    void endTransaction() {
        inTransaction = false;
        transactionByteIndex = 0;
        selectedRegister = 0;
        isRead = false;
    }

    uint8_t transfer(const uint8_t data) {
        if (!inTransaction) return 0;
        uint8_t res = 0;
        if (transferHook) {
            res = transferHook(data, this);
        }
        transactionByteIndex++;
        return res;
    }
};

inline SPIClass SPI;
