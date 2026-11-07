#include "RfidSensor.h"
#include <Arduino.h>
#include <SPI.h>
#include <cassert>
#include <cstring>
#include <vector>

namespace {
class EventRecorder : public EventHandler {
public:
    std::vector<Event> events;
    void on(const Event event) override {
        events.push_back(event);
    }
};

// Simulated MFRC522 state machine for testing
struct MockMFRC522State {
    uint8_t version = 0x92;
    std::unordered_map<uint8_t, uint8_t> regs;
    std::vector<uint8_t> fifo;
    bool cardPresent = false;
    std::vector<uint8_t> cardUid = {0xDE, 0xAD, 0xBE, 0xEF};

    MockMFRC522State() {
        regs[RfidSensor::VersionReg] = 0x92;
        regs[RfidSensor::CommandReg] = 0x00;
        regs[RfidSensor::ComIrqReg] = 0x00;
        regs[RfidSensor::DivIrqReg] = 0x00;
        regs[RfidSensor::FIFOLevelReg] = 0x00;
        regs[RfidSensor::ErrorReg] = 0x00;
        regs[RfidSensor::ControlReg] = 0x00;
    }
} mockState;

uint8_t mockTransferHook(const uint8_t data, SPIClass* spi) {
    if (spi->transactionByteIndex == 0) {
        // Address byte
        const uint8_t reg = (data >> 1) & 0x3F;
        spi->selectedRegister = reg;
        spi->isRead = (data & 0x80) != 0;
        return 0;
    }

    // Data phase (byteIndex >= 1)
    if (spi->isRead) {
        const uint8_t reg = spi->selectedRegister;
        if (reg == RfidSensor::FIFODataReg) {
            if (!mockState.fifo.empty()) {
                const uint8_t val = mockState.fifo.front();
                mockState.fifo.erase(mockState.fifo.begin());
                mockState.regs[RfidSensor::FIFOLevelReg] = static_cast<uint8_t>(mockState.fifo.size());
                return val;
            }
            return 0;
        }
        if (reg == RfidSensor::FIFOLevelReg) {
            return static_cast<uint8_t>(mockState.fifo.size());
        }
        if (mockState.regs.find(reg) != mockState.regs.end()) {
            return mockState.regs[reg];
        }
        return 0;
    } else {
        // Write phase
        const uint8_t reg = spi->selectedRegister;
        if (reg == RfidSensor::FIFODataReg) {
            mockState.fifo.push_back(data);
            mockState.regs[RfidSensor::FIFOLevelReg] = static_cast<uint8_t>(mockState.fifo.size());
            return 0;
        }
        if (reg == RfidSensor::FIFOLevelReg && (data & 0x80)) {
            mockState.fifo.clear();
            mockState.regs[RfidSensor::FIFOLevelReg] = 0;
            return 0;
        }
        if (reg == RfidSensor::CommandReg) {
            mockState.regs[reg] = data;
            if (data == RfidSensor::PCD_CalcCRC) {
                // Simulate CRC calculation over mockState.fifo
                uint16_t crc = 0x6363;
                for (uint8_t b : mockState.fifo) {
                    b ^= static_cast<uint8_t>(crc & 0x00FF);
                    b ^= (b << 4);
                    crc = (crc >> 8) ^ (static_cast<uint16_t>(b) << 8) ^ (static_cast<uint16_t>(b) << 3) ^ (static_cast<uint16_t>(b) >> 4);
                }
                mockState.regs[RfidSensor::CRCResultRegL] = static_cast<uint8_t>(crc & 0xFF);
                mockState.regs[RfidSensor::CRCResultRegH] = static_cast<uint8_t>((crc >> 8) & 0xFF);
                mockState.regs[RfidSensor::DivIrqReg] |= 0x04; // CRCIRq
            } else if (data == RfidSensor::PCD_Transceive) {
                // Simulate Transceive response based on sent command
                if (!mockState.cardPresent) {
                    mockState.regs[RfidSensor::ComIrqReg] = 0x01; // TimerIrq (timeout)
                    mockState.fifo.clear();
                } else {
                    if (!mockState.fifo.empty()) {
                        const uint8_t cmd = mockState.fifo[0];
                        if (cmd == RfidSensor::PICC_CMD_REQA || cmd == RfidSensor::PICC_CMD_WUPA) {
                            // ATQA response: 2 bytes
                            mockState.fifo.clear();
                            mockState.fifo.push_back(0x04);
                            mockState.fifo.push_back(0x00);
                            mockState.regs[RfidSensor::ComIrqReg] = 0x20; // RxIRq
                            mockState.regs[RfidSensor::FIFOLevelReg] = 2;
                            mockState.regs[RfidSensor::ControlReg] = 0x00;
                        } else if (cmd == RfidSensor::PICC_CMD_SEL_CL1) {
                            const uint8_t nvb = (mockState.fifo.size() > 1) ? mockState.fifo[1] : 0;
                            if (nvb == 0x20) {
                                // Anti-collision CL1: return 4 bytes UID + BCC
                                mockState.fifo.clear();
                                uint8_t bcc = 0;
                                if (mockState.cardUid.size() == 4) {
                                    for (int i = 0; i < 4; ++i) {
                                        mockState.fifo.push_back(mockState.cardUid[i]);
                                        bcc ^= mockState.cardUid[i];
                                    }
                                } else {
                                    // 7-byte UID: CL1 sends CT (0x88) + UID0..UID2
                                    mockState.fifo.push_back(RfidSensor::PICC_CMD_CT);
                                    bcc ^= RfidSensor::PICC_CMD_CT;
                                    for (int i = 0; i < 3; ++i) {
                                        mockState.fifo.push_back(mockState.cardUid[i]);
                                        bcc ^= mockState.cardUid[i];
                                    }
                                }
                                mockState.fifo.push_back(bcc);
                                mockState.regs[RfidSensor::ComIrqReg] = 0x20;
                                mockState.regs[RfidSensor::FIFOLevelReg] = 5;
                            } else if (nvb == 0x70) {
                                // Select CL1: return SAK (1 byte) + CRC (2 bytes)
                                mockState.fifo.clear();
                                const uint8_t sak = (mockState.cardUid.size() == 4) ? 0x08 : 0x04;
                                mockState.fifo.push_back(sak);
                                uint16_t crc = 0x6363;
                                const uint8_t b = sak ^ static_cast<uint8_t>(crc & 0x00FF);
                                const uint8_t bShifted = b ^ (b << 4);
                                crc = (crc >> 8) ^ (static_cast<uint16_t>(bShifted) << 8) ^ (static_cast<uint16_t>(bShifted) << 3) ^ (static_cast<uint16_t>(bShifted) >> 4);
                                mockState.fifo.push_back(static_cast<uint8_t>(crc & 0xFF));
                                mockState.fifo.push_back(static_cast<uint8_t>((crc >> 8) & 0xFF));
                                mockState.regs[RfidSensor::ComIrqReg] = 0x20;
                                mockState.regs[RfidSensor::FIFOLevelReg] = 3;
                            }
                        } else if (cmd == RfidSensor::PICC_CMD_SEL_CL2) {
                            const uint8_t nvb = (mockState.fifo.size() > 1) ? mockState.fifo[1] : 0;
                            if (nvb == 0x20) {
                                // Anti-collision CL2: return remaining 4 bytes UID + BCC
                                mockState.fifo.clear();
                                uint8_t bcc = 0;
                                for (int i = 3; i < 7; ++i) {
                                    mockState.fifo.push_back(mockState.cardUid[i]);
                                    bcc ^= mockState.cardUid[i];
                                }
                                mockState.fifo.push_back(bcc);
                                mockState.regs[RfidSensor::ComIrqReg] = 0x20;
                                mockState.regs[RfidSensor::FIFOLevelReg] = 5;
                            } else if (nvb == 0x70) {
                                // Select CL2: return SAK (0x00 for Ultralight) + CRC
                                mockState.fifo.clear();
                                const uint8_t sak = 0x00;
                                mockState.fifo.push_back(sak);
                                uint16_t crc = 0x6363;
                                const uint8_t b = sak ^ static_cast<uint8_t>(crc & 0x00FF);
                                const uint8_t bShifted = b ^ (b << 4);
                                crc = (crc >> 8) ^ (static_cast<uint16_t>(bShifted) << 8) ^ (static_cast<uint16_t>(bShifted) << 3) ^ (static_cast<uint16_t>(bShifted) >> 4);
                                mockState.fifo.push_back(static_cast<uint8_t>(crc & 0xFF));
                                mockState.fifo.push_back(static_cast<uint8_t>((crc >> 8) & 0xFF));
                                mockState.regs[RfidSensor::ComIrqReg] = 0x20;
                                mockState.regs[RfidSensor::FIFOLevelReg] = 3;
                            }
                        } else if (cmd == RfidSensor::PICC_CMD_HLTA) {
                            mockState.fifo.clear();
                            mockState.regs[RfidSensor::ComIrqReg] = 0x01; // Halt has no response
                        }
                    }
                }
            }
            return 0;
        }
        mockState.regs[reg] = data;
        return 0;
    }
}
} // namespace

[[nodiscard]] int main() {
    SPI.transferHook = mockTransferHook;

    EventRecorder recorder;
    RfidSensor sensor(5, 22, &recorder, SPI);

    assert(sensor.isInitialized());
    assert(!sensor.isTagPresent());
    assert(std::strcmp(sensor.getUid(), "") == 0);

    // 1. Polling with no tag in field
    sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
    assert(recorder.events.empty());
    assert(!sensor.isTagPresent());

    // 2. Present 4-byte UID tag (DE:AD:BE:EF)
    mockState.cardPresent = true;
    mockState.cardUid = {0xDE, 0xAD, 0xBE, 0xEF};
    sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);

    assert(recorder.events.size() == 1);
    assert(recorder.events[0].identifier == RfidSensor::TAG_DETECTED_EVENT_IDENTIFIER);
    assert(recorder.events[0].sourceId == 5);
    assert(sensor.isTagPresent());
    assert(std::strcmp(sensor.getUid(), "DE:AD:BE:EF") == 0);

    uint8_t uidLen = 0;
    const uint8_t* rawUid = sensor.getUidBytes(uidLen);
    assert(uidLen == 4);
    assert(rawUid[0] == 0xDE && rawUid[1] == 0xAD && rawUid[2] == 0xBE && rawUid[3] == 0xEF);

    // 3. Keep card in field - should suppress repeated detection events
    sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
    assert(recorder.events.size() == 1);
    assert(sensor.isTagPresent());

    // 4. Remove card from field
    mockState.cardPresent = false;
    sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);

    assert(recorder.events.size() == 2);
    assert(recorder.events[1].identifier == RfidSensor::TAG_REMOVED_EVENT_IDENTIFIER);
    assert(recorder.events[1].sourceId == 5);
    assert(!sensor.isTagPresent());
    assert(std::strcmp(sensor.getUid(), "") == 0);

    // 5. Subsequent poll with no card - no extra events
    sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);
    assert(recorder.events.size() == 2);

    // 6. Test 7-byte UID tag (NTAG/Ultralight: 04:12:34:56:78:9A:BC)
    mockState.cardPresent = true;
    mockState.cardUid = {0x04, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
    sensor.on(Sensor::MEASURE_DATA_REQUESTED_EVENT);

    assert(recorder.events.size() == 3);
    assert(recorder.events[2].identifier == RfidSensor::TAG_DETECTED_EVENT_IDENTIFIER);
    assert(sensor.isTagPresent());
    assert(std::strcmp(sensor.getUid(), "04:12:34:56:78:9A:BC") == 0);

    rawUid = sensor.getUidBytes(uidLen);
    assert(uidLen == 7);
    assert(rawUid[0] == 0x04 && rawUid[6] == 0xBC);

    // 7. Test powerDown and powerUp
    sensor.powerDown();
    assert((mockState.regs[RfidSensor::CommandReg] & 0x10) != 0);
    sensor.powerUp();
    assert((mockState.regs[RfidSensor::CommandReg] & 0x10) == 0);

    // 8. Test calculateCrc directly
    uint8_t testPayload[] = {0x12, 0x34, 0x56};
    uint8_t crcOut[2] = {0, 0};
    RfidSensor::StatusCode crcResult = sensor.calculateCrc(testPayload, sizeof(testPayload), crcOut);
    assert(crcResult == RfidSensor::STATUS_OK);
    assert(crcOut[0] != 0 || crcOut[1] != 0);

    // 9. Test direct requestA and wakeUpA
    mockState.cardPresent = true;
    uint8_t atqa[2];
    uint8_t atqaSize = sizeof(atqa);
    assert(sensor.requestA(atqa, &atqaSize) == RfidSensor::STATUS_OK);
    assert(atqaSize == 2);
    atqaSize = sizeof(atqa);
    assert(sensor.wakeUpA(atqa, &atqaSize) == RfidSensor::STATUS_OK);

    // 10. Test initialization failure with invalid VersionReg
    mockState.regs[RfidSensor::VersionReg] = 0x00;
    RfidSensor failedSensor(5, 22, &recorder, SPI);
    assert(!failedSensor.isInitialized());

    return 0;
}
