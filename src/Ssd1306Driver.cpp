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
    : i2cAddress(i2cAddress),
      displayWidth(width),
      displayHeight(height),
      displayOn(false) {
}

void Ssd1306Driver::sendCommandByte(const uint8_t command) const {
    Wire.beginTransmission(i2cAddress);
    Wire.write(SSD1306_COMMAND_PREFIX);
    Wire.write(command);
    (void)Wire.endTransmission(true);
    delayMicroseconds(SSD1306_I2C_COMMAND_DELAY_IN_MICROSECONDS);
}

void Ssd1306Driver::initialize() {
    // SSD1306 initialization sequence for 128x64 OLED display.
    // Based on the SSD1306 datasheet initialization recommendations.

    sendCommandByte(SSD1306_SET_DISPLAY_OFF);
    sendCommandByte(SSD1306_SET_DISPLAY_CLOCK_DIV);
    sendCommandByte(SSD1306_CLOCK_DIVIDE_RATIO_DEFAULT);

    sendCommandByte(SSD1306_SET_MULTIPLEX_RATIO);
    sendCommandByte(displayHeight - 1); // Multiplex ratio: 63 for 128x64, 31 for 128x32

    sendCommandByte(SSD1306_SET_DISPLAY_OFFSET);
    sendCommandByte(SSD1306_DISPLAY_OFFSET_NONE);

    sendCommandByte(SSD1306_SET_DISPLAY_START_LINE); // Set display start line to 0

    sendCommandByte(SSD1306_SET_CONTRAST_CONTROL);
    sendCommandByte(SSD1306_CONTRAST_DEFAULT);

    sendCommandByte(SSD1306_SET_SEGMENT_REMAP); // Column address 0 is segment 0

    sendCommandByte(SSD1306_SET_CHARGE_PUMP);
    sendCommandByte(SSD1306_CHARGE_PUMP_ENABLE);

    sendCommandByte(SSD1306_SET_MEMORY_ADDRESSING_MODE);
    sendCommandByte(SSD1306_ADDRESSING_MODE_HORIZONTAL);

    sendCommandByte(SSD1306_SET_DEACTIVATE_SCROLL);
    sendCommandByte(SSD1306_SET_NORMAL_DISPLAY);

    // Clear display by sending all zeros
    uint8_t clearBuffer[SSD1306_MAX_DISPLAY_WIDTH];
    for (uint8_t i = 0; i < displayWidth; i++) {
        clearBuffer[i] = SSD1306_CLEAR_BUFFER_FILL_VALUE;
    }
    for (uint8_t page = 0; page < displayHeight / 8; page++) {
        sendCommandByte(SSD1306_SET_PAGE_ADDRESS | page);
        sendCommandByte(SSD1306_SET_LOWER_COLUMN_ADDRESS);
        sendCommandByte(SSD1306_SET_HIGHER_COLUMN_ADDRESS);
        sendData(clearBuffer, displayWidth);
    }

    sendCommandByte(SSD1306_SET_DISPLAY_ON);
    displayOn = true;
}

void Ssd1306Driver::sendData(const uint8_t* data, const size_t length) const {
    Wire.beginTransmission(i2cAddress);
    Wire.write(SSD1306_DATA_PREFIX);
    for (size_t i = 0; i < length; i++) {
        Wire.write(data[i]);
    }
    (void)Wire.endTransmission(true);
}

void Ssd1306Driver::sendFrameBuffer(const uint8_t* frameBuffer, const uint8_t width) const {
    // Send the frame buffer page by page using page addressing mode.
    const uint8_t numberOfPages = displayHeight / 8;
    for (uint8_t page = 0; page < numberOfPages; page++) {
        sendCommandByte(SSD1306_SET_PAGE_ADDRESS | page);
        sendCommandByte(SSD1306_SET_LOWER_COLUMN_ADDRESS);
        sendCommandByte(SSD1306_SET_HIGHER_COLUMN_ADDRESS);
        const uint8_t* pageData = frameBuffer + (page * width);
        sendData(pageData, width);
    }
}

void Ssd1306Driver::setContrast(const uint8_t contrast) const {
    sendCommandByte(SSD1306_SET_CONTRAST_CONTROL);
    sendCommandByte(contrast);
}

void Ssd1306Driver::setInvert(const bool invert) const {
    sendCommandByte(invert ? SSD1306_SET_INVERT_DISPLAY : SSD1306_SET_NORMAL_DISPLAY);
}

void Ssd1306Driver::setDisplayOn(const bool enable) {
    displayOn = enable;
    sendCommandByte(enable ? SSD1306_SET_DISPLAY_ON : SSD1306_SET_DISPLAY_OFF);
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
