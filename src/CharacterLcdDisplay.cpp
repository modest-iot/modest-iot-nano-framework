/**
 * @file CharacterLcdDisplay.cpp
 * @brief Implements buffered text rendering and backlight control for I2C Liquid Crystal displays.
 * Composes Hd44780Driver for native HD44780 4-bit mode protocol via Wire with PCF8574 backpack.
 * @date 2026-06-06
 * @version 1.0
 */

#include "CharacterLcdDisplay.h"
#include <cstring>

const Command CharacterLcdDisplay::UPDATE_TEXT_COMMAND = Command(UPDATE_TEXT_COMMAND_IDENTIFIER);
const Command CharacterLcdDisplay::TURN_BACKLIGHT_ON_COMMAND = Command(TURN_BACKLIGHT_ON_COMMAND_IDENTIFIER);
const Command CharacterLcdDisplay::TURN_BACKLIGHT_OFF_COMMAND = Command(TURN_BACKLIGHT_OFF_COMMAND_IDENTIFIER);

CharacterLcdDisplay::CharacterLcdDisplay(const uint8_t i2cAddress, const uint8_t columnCount, const uint8_t rowCount,
                                         const bool backlightInitialState, CommandHandler* parentHandler)
    : Actuator(static_cast<int>(i2cAddress), parentHandler),
      driver(i2cAddress, backlightInitialState),
      maximumRowCount(rowCount),
      maximumColumnCount(columnCount)
{
    const size_t absoluteAllocationSize = static_cast<size_t>(maximumRowCount) * (maximumColumnCount + 1);
    characterScreenMatrixBuffer = new char[absoluteAllocationSize];

    for (uint8_t rowIndex = 0; rowIndex < maximumRowCount; ++rowIndex) {
        char* rowHeadPointer = characterScreenMatrixBuffer + (rowIndex * (maximumColumnCount + 1));
        memset(rowHeadPointer, ' ', maximumColumnCount);
        rowHeadPointer[maximumColumnCount] = '\0';
    }

    driver.initialize();
}

CharacterLcdDisplay::~CharacterLcdDisplay() {
    if (characterScreenMatrixBuffer != nullptr) {
        delete[] characterScreenMatrixBuffer;
        characterScreenMatrixBuffer = nullptr;
    }
}

void CharacterLcdDisplay::setLineBuffer(const uint8_t rowIndex, const char* nullTerminatedString) const {
    if (rowIndex >= maximumRowCount || nullTerminatedString == nullptr) {
        return;
    }

    char* rowHeadPointer = characterScreenMatrixBuffer + (rowIndex * (maximumColumnCount + 1));
    const uint8_t stringLength = strlen(nullTerminatedString);

    for (uint8_t columnIndex = 0; columnIndex < maximumColumnCount; ++columnIndex) {
        if (columnIndex < stringLength) {
            rowHeadPointer[columnIndex] = nullTerminatedString[columnIndex];
        } else {
            rowHeadPointer[columnIndex] = ' ';
        }
    }
    rowHeadPointer[maximumColumnCount] = '\0';
}

void CharacterLcdDisplay::setLineBuffer(const char* multiLineArrayPointer[]) const {
    if (multiLineArrayPointer == nullptr) return;
    for (uint8_t rowIndex = 0; rowIndex < maximumRowCount; ++rowIndex) {
        setLineBuffer(rowIndex, multiLineArrayPointer[rowIndex]);
    }
}

void CharacterLcdDisplay::executeCommand(Command command) {
    if (command == UPDATE_TEXT_COMMAND) {
        commitBuffersToPhysicalScreen();
    }
    else if (command == TURN_BACKLIGHT_ON_COMMAND) {
        driver.setBacklight(true);
    }
    else if (command == TURN_BACKLIGHT_OFF_COMMAND) {
        driver.setBacklight(false);
    }
}

void CharacterLcdDisplay::commitBuffersToPhysicalScreen() const {
    for (uint8_t rowIndex = 0; rowIndex < maximumRowCount; ++rowIndex) {
        // Set cursor to column 0 of the current row using HD44780 DDRAM addresses.
        const uint8_t ddramAddress = (rowIndex == 0) ? HD44780_DDRAM_ADDRESS_ROW_0 :
                                     (rowIndex == 1) ? HD44780_DDRAM_ADDRESS_ROW_1 :
                                     (rowIndex == 2) ? HD44780_DDRAM_ADDRESS_ROW_2 :
                                                        HD44780_DDRAM_ADDRESS_ROW_3;
        driver.sendCommand(ddramAddress);

        const char* rowHeadPointer = characterScreenMatrixBuffer + (rowIndex * (maximumColumnCount + 1));
        const uint8_t stringLength = strlen(rowHeadPointer);
        for (uint8_t i = 0; i < stringLength; i++) {
            driver.sendData(static_cast<uint8_t>(rowHeadPointer[i]));
        }
    }
}

// Method performs an active query function over the state
bool CharacterLcdDisplay::isBacklightOn() const {
    return driver.isBacklightOn();
}

uint8_t CharacterLcdDisplay::getMaximumRowCount() const { return maximumRowCount; }
uint8_t CharacterLcdDisplay::getMaximumColumnCount() const { return maximumColumnCount; }