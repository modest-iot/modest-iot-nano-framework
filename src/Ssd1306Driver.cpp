/**
 * @file Ssd1306Driver.cpp
 * @brief Implements the SSD1306 OLED I2C protocol for 128x64 monochrome displays.
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-10
 * @version 1.0
 */

#include "Ssd1306Driver.h"
#include <Arduino.h>

Ssd1306Driver::Ssd1306Driver(const uint8_t i2cAddress, const uint8_t width, const uint8_t height)
    : bus(nullptr),
      i2cAddress(i2cAddress),
      displayWidth(width),
      displayHeight(height),
      displayOn(false),
      initialized(false),
      initializing(false),
      lastI2cResult{I2cStatus::NotStarted, 0, 0} {
}

Ssd1306Driver::Ssd1306Driver(I2cBus& bus, const uint8_t i2cAddress, const uint8_t width, const uint8_t height)
    : bus(&bus),
      i2cAddress(i2cAddress),
      displayWidth(width),
      displayHeight(height),
      displayOn(false),
      initialized(false),
      initializing(false),
      lastI2cResult{I2cStatus::NotStarted, 0, 0} {
}

I2cResult Ssd1306Driver::sendBytes(const uint8_t* bytes, const size_t length) const {
    if (bus != nullptr) {
        lastI2cResult = bus->write(i2cAddress, bytes, length);
        return lastI2cResult;
    }

    if (bytes == nullptr || length == 0) {
        lastI2cResult = {I2cStatus::InvalidArgument, 0, length};
        return lastI2cResult;
    }

    Wire.beginTransmission(i2cAddress);
    const size_t written = Wire.write(bytes, length);
    const uint8_t wireStatus = Wire.endTransmission(true);
    const I2cStatus status = written == length ? mapWireStatus(wireStatus) : I2cStatus::InvalidArgument;
    lastI2cResult = {status, status == I2cStatus::Ok ? length : 0, length};
    return lastI2cResult;
}

bool Ssd1306Driver::sendCommandByte(const uint8_t command) const {
    if (bus != nullptr && !initialized && !initializing) {
        lastI2cResult = {I2cStatus::NotStarted, 0, 2};
        return false;
    }

    const uint8_t commandBytes[] = {SSD1306_COMMAND_PREFIX, command};
    if (sendBytes(commandBytes, sizeof(commandBytes)).isInvalidOrIncomplete()) {
        return false;
    }
    delayMicroseconds(SSD1306_I2C_COMMAND_DELAY_IN_MICROSECONDS);
    return true;
}

void Ssd1306Driver::initialize() {
    (void)start();
}

bool Ssd1306Driver::start() {
    return initializeSequence();
}

bool Ssd1306Driver::initializeSequence() {
    initialized = false;
    displayOn = false;
    if (bus != nullptr && !bus->isReady()) {
        lastI2cResult = {I2cStatus::NotStarted, 0, 2};
        return false;
    }
    initializing = true;

    // SSD1306 initialization sequence for 128x64 OLED display.
    // Based on the SSD1306 datasheet initialization recommendations.

    if (!sendCommandByte(SSD1306_SET_DISPLAY_OFF) ||
        !sendCommandByte(SSD1306_SET_DISPLAY_CLOCK_DIV) ||
        !sendCommandByte(SSD1306_CLOCK_DIVIDE_RATIO_DEFAULT)) {
        initializing = false;
        return false;
    }

    if (!sendCommandByte(SSD1306_SET_MULTIPLEX_RATIO) ||
        !sendCommandByte(displayHeight - 1)) { // Multiplex ratio: 63 for 128x64, 31 for 128x32
        initializing = false;
        return false;
    }

    if (!sendCommandByte(SSD1306_SET_DISPLAY_OFFSET) ||
        !sendCommandByte(SSD1306_DISPLAY_OFFSET_NONE) ||
        !sendCommandByte(SSD1306_SET_DISPLAY_START_LINE) ||
        !sendCommandByte(SSD1306_SET_CONTRAST_CONTROL) ||
        !sendCommandByte(SSD1306_CONTRAST_DEFAULT) ||
        !sendCommandByte(SSD1306_SET_SEGMENT_REMAP) ||
        !sendCommandByte(SSD1306_SET_CHARGE_PUMP) ||
        !sendCommandByte(SSD1306_CHARGE_PUMP_ENABLE) ||
        !sendCommandByte(SSD1306_SET_MEMORY_ADDRESSING_MODE) ||
        !sendCommandByte(SSD1306_ADDRESSING_MODE_HORIZONTAL) ||
        !sendCommandByte(SSD1306_SET_DEACTIVATE_SCROLL) ||
        !sendCommandByte(SSD1306_SET_NORMAL_DISPLAY)) {
        initializing = false;
        return false;
    }

    // Clear display by sending all zeros
    uint8_t clearBuffer[SSD1306_MAX_DISPLAY_WIDTH];
    for (uint8_t i = 0; i < displayWidth; i++) {
        clearBuffer[i] = SSD1306_CLEAR_BUFFER_FILL_VALUE;
    }
    for (uint8_t page = 0; page < displayHeight / 8; page++) {
        if (!sendCommandByte(SSD1306_SET_PAGE_ADDRESS | page) ||
            !sendCommandByte(SSD1306_SET_LOWER_COLUMN_ADDRESS) ||
            !sendCommandByte(SSD1306_SET_HIGHER_COLUMN_ADDRESS)) {
            initializing = false;
            return false;
        }
        sendData(clearBuffer, displayWidth);
        if (lastI2cResult.isInvalidOrIncomplete()) {
            initializing = false;
            return false;
        }
    }

    if (!sendCommandByte(SSD1306_SET_DISPLAY_ON)) {
        initializing = false;
        return false;
    }
    initializing = false;
    displayOn = true;
    initialized = true;
    return true;
}

void Ssd1306Driver::sendData(const uint8_t* data, const size_t length) const {
    if (bus != nullptr && !initialized && !initializing) {
        lastI2cResult = {I2cStatus::NotStarted, 0, length + 1};
        return;
    }
    if (length > 0 && data == nullptr) {
        lastI2cResult = {I2cStatus::InvalidArgument, 0, length + 1};
        return;
    }

    size_t offset = 0;
    do {
        uint8_t bytes[SSD1306_MAX_DATA_BYTES_PER_TRANSFER + 1];
        const size_t remaining = length - offset;
        const size_t chunkLength = remaining > SSD1306_MAX_DATA_BYTES_PER_TRANSFER
                                       ? SSD1306_MAX_DATA_BYTES_PER_TRANSFER
                                       : remaining;
        bytes[0] = SSD1306_DATA_PREFIX;
        for (size_t i = 0; i < chunkLength; i++) {
            bytes[i + 1] = data[offset + i];
        }
        const I2cResult result = sendBytes(bytes, chunkLength + 1);
        if (result.isInvalidOrIncomplete()) {
            return;
        }
        offset += chunkLength;
    } while (offset < length);
}

void Ssd1306Driver::sendFrameBuffer(const uint8_t* frameBuffer, const uint8_t width) const {
    if (frameBuffer == nullptr) {
        lastI2cResult = {I2cStatus::InvalidArgument, 0, 0};
        return;
    }

    // Send the frame buffer page by page using page addressing mode.
    const uint8_t numberOfPages = displayHeight / 8;
    for (uint8_t page = 0; page < numberOfPages; page++) {
        if (!sendCommandByte(SSD1306_SET_PAGE_ADDRESS | page) ||
            !sendCommandByte(SSD1306_SET_LOWER_COLUMN_ADDRESS) ||
            !sendCommandByte(SSD1306_SET_HIGHER_COLUMN_ADDRESS)) {
            return;
        }
        const uint8_t* pageData = frameBuffer + (page * width);
        sendData(pageData, width);
        if (lastI2cResult.isInvalidOrIncomplete()) {
            return;
        }
    }
}

void Ssd1306Driver::setContrast(const uint8_t contrast) const {
    if (sendCommandByte(SSD1306_SET_CONTRAST_CONTROL)) {
        (void)sendCommandByte(contrast);
    }
}

void Ssd1306Driver::setInvert(const bool invert) const {
    (void)sendCommandByte(invert ? SSD1306_SET_INVERT_DISPLAY : SSD1306_SET_NORMAL_DISPLAY);
}

void Ssd1306Driver::setDisplayOn(const bool enable) {
    if (bus != nullptr && !initialized) {
        lastI2cResult = {I2cStatus::NotStarted, 0, 2};
        return;
    }
    if (sendCommandByte(enable ? SSD1306_SET_DISPLAY_ON : SSD1306_SET_DISPLAY_OFF)) {
        displayOn = enable;
    }
}

bool Ssd1306Driver::isDisplayOn() const {
    return displayOn;
}

uint8_t Ssd1306Driver::getWidth() const {
    return displayWidth;
}

uint8_t Ssd1306Driver::getHeight() const {
    return displayHeight;
}

I2cResult Ssd1306Driver::getLastI2cResult() const {
    return lastI2cResult;
}

bool Ssd1306Driver::isInitialized() const {
    return initialized;
}

I2cStatus Ssd1306Driver::mapWireStatus(const uint8_t status) {
    switch (status) {
        case 0: return I2cStatus::Ok;
        case 2: return I2cStatus::AddressNack;
        case 3: return I2cStatus::DataNack;
        case 5: return I2cStatus::Timeout;
        case 1: return I2cStatus::InvalidArgument;
        default: return I2cStatus::BusError;
    }
}
