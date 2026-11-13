/**
 * @file RfidSensor.cpp
 * @brief Implements MFRC522 SPI register communication, ISO14443A card polling, and event dispatching.
 * @copyright (c) 2025-2026 Angel Velasquez
 */

#include "RfidSensor.h"
#include <cstdio>
#include <cstring>

const Event RfidSensor::TAG_DETECTED_EVENT = Event(TAG_DETECTED_EVENT_IDENTIFIER);
const Event RfidSensor::TAG_REMOVED_EVENT = Event(TAG_REMOVED_EVENT_IDENTIFIER);

RfidSensor::RfidSensor(
    const int chipSelectGpioPin,
    const int resetGpioPin,
    EventHandler* parentHandler,
    SPIClass& spiInstance)
    : Sensor(chipSelectGpioPin, parentHandler),
      resetGpioPin(resetGpioPin),
      spi(spiInstance),
      cachedUidLength(0),
      initialized(false),
      tagCurrentlyPresent(false),
      lastNotifiedPresence(false) {
    cachedUidString[0] = '\0';
    lastNotifiedUidString[0] = '\0';
    for (uint8_t i = 0; i < MAX_UID_BYTES; ++i) {
        cachedUidBytes[i] = 0;
    }
    spi.begin();
    initialize();
}

bool RfidSensor::initialize() {
    initialized = false;
    tagCurrentlyPresent = false;
    lastNotifiedPresence = false;
    clearUidCache();

    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);

    if (resetGpioPin >= 0) {
        pinMode(resetGpioPin, OUTPUT);
        digitalWrite(resetGpioPin, LOW);
        delayMicroseconds(2);
        digitalWrite(resetGpioPin, HIGH);
        delay(50);
    }

    // Direct soft reset
    writeRegister(CommandReg, PCD_SoftReset);
    delay(50);

    // Turn off Crypto1
    clearRegisterBitMask(Status2Reg, 0x08);

    // Setup timer: TAuto=1, prescaler 0x0D3E, reload 1000 (25ms)
    writeRegister(TModeReg, 0x80);
    writeRegister(TPrescalerReg, 0xA9);
    writeRegister(TReloadRegH, 0x03);
    writeRegister(TReloadRegL, 0xE8);

    // 100% ASK modulation
    writeRegister(TxASKReg, 0x40);

    // Preset CRC 0x6363
    writeRegister(ModeReg, 0x3D);

    // Turn antenna on
    setAntennaOn(true);

    const uint8_t version = readRegister(VersionReg);
    if (version == 0x00 || version == 0xFF) {
        return false;
    }

    initialized = true;
    return true;
}

bool RfidSensor::isInitialized() const {
    return initialized;
}

void RfidSensor::writeRegister(const uint8_t reg, const uint8_t value) {
    spi.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    digitalWrite(pin, LOW);
    spi.transfer(static_cast<uint8_t>(((reg & 0x3F) << 1) & 0x7E));
    spi.transfer(value);
    digitalWrite(pin, HIGH);
    spi.endTransaction();
}

void RfidSensor::writeRegister(const uint8_t reg, const uint8_t count, const uint8_t* values) {
    if (count == 0 || values == nullptr) return;
    spi.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    digitalWrite(pin, LOW);
    spi.transfer(static_cast<uint8_t>(((reg & 0x3F) << 1) & 0x7E));
    for (uint8_t i = 0; i < count; ++i) {
        spi.transfer(values[i]);
    }
    digitalWrite(pin, HIGH);
    spi.endTransaction();
}

uint8_t RfidSensor::readRegister(const uint8_t reg) const {
    spi.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    digitalWrite(pin, LOW);
    spi.transfer(static_cast<uint8_t>(((reg & 0x3F) << 1) | 0x80));
    const uint8_t value = spi.transfer(0x00);
    digitalWrite(pin, HIGH);
    spi.endTransaction();
    return value;
}

void RfidSensor::readRegister(const uint8_t reg, const uint8_t count, uint8_t* values, const uint8_t rxAlign) const {
    if (count == 0 || values == nullptr) return;
    (void)rxAlign;
    spi.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    digitalWrite(pin, LOW);
    const uint8_t address = static_cast<uint8_t>(((reg & 0x3F) << 1) | 0x80);
    spi.transfer(address);
    for (uint8_t i = 0; i < count; ++i) {
        if (i == count - 1) {
            values[i] = spi.transfer(0x00);
        } else {
            values[i] = spi.transfer(address);
        }
    }
    digitalWrite(pin, HIGH);
    spi.endTransaction();
}

void RfidSensor::setRegisterBitMask(const uint8_t reg, const uint8_t mask) {
    const uint8_t current = readRegister(reg);
    writeRegister(reg, current | mask);
}

void RfidSensor::clearRegisterBitMask(const uint8_t reg, const uint8_t mask) {
    const uint8_t current = readRegister(reg);
    writeRegister(reg, static_cast<uint8_t>(current & (~mask)));
}

void RfidSensor::setAntennaOn(const bool enable) {
    if (enable) {
        const uint8_t current = readRegister(TxControlReg);
        if ((current & 0x03) != 0x03) {
            writeRegister(TxControlReg, current | 0x03);
        }
    } else {
        clearRegisterBitMask(TxControlReg, 0x03);
    }
}

RfidSensor::StatusCode RfidSensor::communicateWithPicc(
    const uint8_t command,
    const uint8_t waitIrq,
    const uint8_t* sendData,
    const uint8_t sendLen,
    uint8_t* backData,
    uint8_t* backLen,
    uint8_t* validBits,
    const uint8_t rxAlign,
    const bool checkCrc) {

    const uint8_t txLastBits = validBits ? *validBits : 0;
    const uint8_t bitFraming = static_cast<uint8_t>((rxAlign << 4) + txLastBits);

    writeRegister(CommandReg, PCD_Idle);
    writeRegister(ComIrqReg, 0x7F);
    writeRegister(FIFOLevelReg, 0x80);
    writeRegister(FIFODataReg, sendLen, sendData);
    writeRegister(BitFramingReg, bitFraming);
    writeRegister(CommandReg, command);

    if (command == PCD_Transceive) {
        setRegisterBitMask(BitFramingReg, 0x80);
    }

    const unsigned long startTime = millis();
    bool completed = false;

    while (millis() - startTime < 35) {
        const uint8_t irq = readRegister(ComIrqReg);
        if (irq & waitIrq) {
            completed = true;
            break;
        }
        if (irq & 0x01) { // TimerIrq
            return STATUS_TIMEOUT;
        }
        yield();
    }

    if (!completed) {
        return STATUS_TIMEOUT;
    }

    clearRegisterBitMask(BitFramingReg, 0x80);

    const uint8_t errorRegValue = readRegister(ErrorReg);
    if (errorRegValue & 0x13) { // BufferOvfl, ParityErr, ProtocolErr
        return STATUS_ERROR;
    }
    if (errorRegValue & 0x08) { // CollErr
        return STATUS_COLLISION;
    }

    if (backData != nullptr && backLen != nullptr) {
        const uint8_t fifoLevel = readRegister(FIFOLevelReg);
        if (fifoLevel > *backLen) {
            return STATUS_NO_ROOM;
        }
        *backLen = fifoLevel;
        readRegister(FIFODataReg, fifoLevel, backData, rxAlign);

        const uint8_t _validBits = readRegister(ControlReg) & 0x07;
        if (validBits != nullptr) {
            *validBits = _validBits;
        }
    }

    if (checkCrc && backData != nullptr && backLen != nullptr) {
        if (*backLen < 2) {
            return STATUS_CRC_WRONG;
        }
        uint8_t crcBuffer[2];
        const StatusCode crcStatus = calculateCrc(backData, *backLen - 2, crcBuffer);
        if (crcStatus != STATUS_OK) {
            return crcStatus;
        }
        if (backData[*backLen - 2] != crcBuffer[0] || backData[*backLen - 1] != crcBuffer[1]) {
            return STATUS_CRC_WRONG;
        }
    }

    return STATUS_OK;
}

RfidSensor::StatusCode RfidSensor::calculateCrc(const uint8_t* data, const uint8_t length, uint8_t* result) {
    if (data == nullptr || result == nullptr) {
        return STATUS_INVALID;
    }
    writeRegister(CommandReg, PCD_Idle);
    writeRegister(DivIrqReg, 0x04);
    writeRegister(FIFOLevelReg, 0x80);
    writeRegister(FIFODataReg, length, data);
    writeRegister(CommandReg, PCD_CalcCRC);

    const unsigned long startTime = millis();
    while (millis() - startTime < 15) {
        const uint8_t n = readRegister(DivIrqReg);
        if (n & 0x04) {
            writeRegister(CommandReg, PCD_Idle);
            result[0] = readRegister(CRCResultRegL);
            result[1] = readRegister(CRCResultRegH);
            return STATUS_OK;
        }
        yield();
    }
    return STATUS_TIMEOUT;
}

RfidSensor::StatusCode RfidSensor::requestAOrWakeUpA(const uint8_t command, uint8_t* bufferAtqa, uint8_t* bufferSize) {
    if (bufferAtqa == nullptr || bufferSize == nullptr || *bufferSize < 2) {
        return STATUS_NO_ROOM;
    }
    clearRegisterBitMask(CollReg, 0x80);
    uint8_t validBits = 7;
    const StatusCode status = communicateWithPicc(PCD_Transceive, 0x30, &command, 1, bufferAtqa, bufferSize, &validBits);
    if (status != STATUS_OK) {
        return status;
    }
    if (*bufferSize != 2 || validBits != 0) {
        return STATUS_ERROR;
    }
    return STATUS_OK;
}

RfidSensor::StatusCode RfidSensor::requestA(uint8_t* bufferAtqa, uint8_t* bufferSize) {
    return requestAOrWakeUpA(PICC_CMD_REQA, bufferAtqa, bufferSize);
}

RfidSensor::StatusCode RfidSensor::wakeUpA(uint8_t* bufferAtqa, uint8_t* bufferSize) {
    return requestAOrWakeUpA(PICC_CMD_WUPA, bufferAtqa, bufferSize);
}

RfidSensor::StatusCode RfidSensor::haltA() {
    uint8_t buffer[4];
    buffer[0] = PICC_CMD_HLTA;
    buffer[1] = 0x00;
    const StatusCode status = calculateCrc(buffer, 2, &buffer[2]);
    if (status != STATUS_OK) {
        return status;
    }
    return communicateWithPicc(PCD_Transceive, 0x30, buffer, sizeof(buffer));
}

RfidSensor::StatusCode RfidSensor::readCardSerial(uint8_t* outUidBytes, uint8_t& outLength) {
    uint8_t cascadeLevel = 1;
    outLength = 0;

    while (cascadeLevel <= 3) {
        uint8_t cmd;
        if (cascadeLevel == 1) {
            cmd = PICC_CMD_SEL_CL1;
        } else if (cascadeLevel == 2) {
            cmd = PICC_CMD_SEL_CL2;
        } else {
            cmd = PICC_CMD_SEL_CL3;
        }

        uint8_t sendBuffer[9];
        sendBuffer[0] = cmd;
        sendBuffer[1] = 0x20;

        uint8_t responseBuffer[5];
        uint8_t responseSize = sizeof(responseBuffer);
        uint8_t validBits = 0;

        clearRegisterBitMask(CollReg, 0x80);

        StatusCode status = communicateWithPicc(PCD_Transceive, 0x30, sendBuffer, 2, responseBuffer, &responseSize, &validBits);
        if (status != STATUS_OK || responseSize != 5) {
            return (status != STATUS_OK) ? status : STATUS_ERROR;
        }

        const uint8_t bcc = responseBuffer[0] ^ responseBuffer[1] ^ responseBuffer[2] ^ responseBuffer[3];
        if (bcc != responseBuffer[4]) {
            return STATUS_CRC_WRONG;
        }

        sendBuffer[0] = cmd;
        sendBuffer[1] = 0x70;
        for (uint8_t i = 0; i < 5; ++i) {
            sendBuffer[2 + i] = responseBuffer[i];
        }
        status = calculateCrc(sendBuffer, 7, &sendBuffer[7]);
        if (status != STATUS_OK) {
            return status;
        }

        uint8_t sakBuffer[3];
        uint8_t sakSize = sizeof(sakBuffer);
        status = communicateWithPicc(PCD_Transceive, 0x30, sendBuffer, 9, sakBuffer, &sakSize, nullptr, 0, true);
        if (status != STATUS_OK || sakSize < 1) {
            return (status != STATUS_OK) ? status : STATUS_ERROR;
        }

        const uint8_t sak = sakBuffer[0];

        if (responseBuffer[0] == PICC_CMD_CT) {
            for (uint8_t i = 1; i < 4; ++i) {
                if (outLength < MAX_UID_BYTES) {
                    outUidBytes[outLength++] = responseBuffer[i];
                }
            }
            cascadeLevel++;
        } else {
            for (uint8_t i = 0; i < 4; ++i) {
                if (outLength < MAX_UID_BYTES) {
                    outUidBytes[outLength++] = responseBuffer[i];
                }
            }
            if ((sak & 0x04) == 0) {
                break;
            }
            cascadeLevel++;
        }
    }

    return (outLength > 0) ? STATUS_OK : STATUS_ERROR;
}

void RfidSensor::updateUidCache(const uint8_t* rawBytes, const uint8_t length) {
    cachedUidLength = (length > MAX_UID_BYTES) ? MAX_UID_BYTES : length;
    for (uint8_t i = 0; i < cachedUidLength; ++i) {
        cachedUidBytes[i] = rawBytes[i];
    }
    cachedUidString[0] = '\0';
    size_t offset = 0;
    for (uint8_t i = 0; i < cachedUidLength; ++i) {
        if (i > 0 && offset < sizeof(cachedUidString) - 1) {
            cachedUidString[offset++] = ':';
        }
        offset += snprintf(cachedUidString + offset, sizeof(cachedUidString) - offset, "%02X", cachedUidBytes[i]);
    }
}

void RfidSensor::clearUidCache() {
    cachedUidLength = 0;
    cachedUidString[0] = '\0';
    for (uint8_t i = 0; i < MAX_UID_BYTES; ++i) {
        cachedUidBytes[i] = 0;
    }
}

void RfidSensor::processEvent(Event& event) {
    if (event.identifier != MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        return;
    }

    if (!initialized) {
        if (!initialize()) {
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
            return;
        }
    }

    uint8_t bufferAtqa[2];
    uint8_t bufferSize = sizeof(bufferAtqa);

    StatusCode status = requestA(bufferAtqa, &bufferSize);
    if (status != STATUS_OK) {
        bufferSize = sizeof(bufferAtqa);
        status = wakeUpA(bufferAtqa, &bufferSize);
    }

    if (status == STATUS_OK) {
        uint8_t rawUid[MAX_UID_BYTES];
        uint8_t uidLength = 0;
        const StatusCode readStatus = readCardSerial(rawUid, uidLength);
        if (readStatus == STATUS_OK && uidLength > 0) {
            tagCurrentlyPresent = true;
            updateUidCache(rawUid, uidLength);
            haltA();

            if (!lastNotifiedPresence || strcmp(cachedUidString, lastNotifiedUidString) != 0) {
                lastNotifiedPresence = true;
                strncpy(lastNotifiedUidString, cachedUidString, sizeof(lastNotifiedUidString) - 1);
                lastNotifiedUidString[sizeof(lastNotifiedUidString) - 1] = '\0';
                event = Event(TAG_DETECTED_EVENT_IDENTIFIER, pin);
                return;
            }

            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
            return;
        }
    }

    if (lastNotifiedPresence) {
        tagCurrentlyPresent = false;
        lastNotifiedPresence = false;
        lastNotifiedUidString[0] = '\0';
        clearUidCache();
        event = Event(TAG_REMOVED_EVENT_IDENTIFIER, pin);
        return;
    }

    tagCurrentlyPresent = false;
    event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
}

bool RfidSensor::isTagPresent() const {
    return tagCurrentlyPresent;
}

const char* RfidSensor::getUid() const {
    return cachedUidString;
}

const uint8_t* RfidSensor::getUidBytes(uint8_t& outLength) const {
    outLength = cachedUidLength;
    return cachedUidBytes;
}

void RfidSensor::powerDown() {
    setAntennaOn(false);
    setRegisterBitMask(CommandReg, 0x10);
}

void RfidSensor::powerUp() {
    clearRegisterBitMask(CommandReg, 0x10);
    delay(1);
    setAntennaOn(true);
}

uint32_t RfidSensor::getDefaultSamplingIntervalInMilliseconds() const {
    return DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

uint32_t RfidSensor::getMinimalSamplingIntervalInMilliseconds() const {
    return MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS;
}
