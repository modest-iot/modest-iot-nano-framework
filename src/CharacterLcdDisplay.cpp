/**
 * @file CharacterLcdDisplay.cpp
 * @brief Implements buffered text rendering and backlight control for I2C Liquid Crystal displays.
 * @date 2026-06-06
 * @version 1.0
 */

#include "CharacterLcdDisplay.h"

const Command CharacterLcdDisplay::UPDATE_TEXT_COMMAND = Command(CharacterLcdDisplay::UPDATE_TEXT_COMMAND_IDENTIFIER);
const Command CharacterLcdDisplay::TURN_BACKLIGHT_ON_COMMAND = Command(CharacterLcdDisplay::TURN_BACKLIGHT_ON_COMMAND_IDENTIFIER);
const Command CharacterLcdDisplay::TURN_BACKLIGHT_OFF_COMMAND = Command(CharacterLcdDisplay::TURN_BACKLIGHT_OFF_COMMAND_IDENTIFIER);

CharacterLcdDisplay::CharacterLcdDisplay(uint8_t i2cAddress, uint8_t columnCount, uint8_t rowCount,
                                         bool backlightInitialState, CommandHandler* parentHandler)
    : Actuator(static_cast<int>(i2cAddress), parentHandler),
      physicalLcdScreen(i2cAddress, columnCount, rowCount),
      maximumRowCount(rowCount),
      maximumColumnCount(columnCount),
      backlightEnabled(backlightInitialState) // Clean initialization mapping properties directly
{
    size_t absoluteAllocationSize = static_cast<size_t>(maximumRowCount) * (maximumColumnCount + 1);
    characterScreenMatrixBuffer = new char[absoluteAllocationSize];

    for (uint8_t rowIndex = 0; rowIndex < maximumRowCount; ++rowIndex) {
        char* rowHeadPointer = characterScreenMatrixBuffer + (rowIndex * (maximumColumnCount + 1));
        memset(rowHeadPointer, ' ', maximumColumnCount);
        rowHeadPointer[maximumColumnCount] = '\0';
    }

    initializeHardwareRegisters();
}

CharacterLcdDisplay::~CharacterLcdDisplay() {
    if (characterScreenMatrixBuffer != nullptr) {
        delete[] characterScreenMatrixBuffer;
        characterScreenMatrixBuffer = nullptr;
    }
}

void CharacterLcdDisplay::initializeHardwareRegisters() {
    physicalLcdScreen.init();

    // Evaluates state properties purely
    if (backlightEnabled) {
        physicalLcdScreen.backlight();
    } else {
        physicalLcdScreen.noBacklight();
    }
    physicalLcdScreen.clear();
}

void CharacterLcdDisplay::setLineBuffer(uint8_t rowIndex, const char* nullTerminatedString) {
    if (rowIndex >= maximumRowCount || nullTerminatedString == nullptr) {
        return;
    }

    char* rowHeadPointer = characterScreenMatrixBuffer + (rowIndex * (maximumColumnCount + 1));
    uint8_t stringLength = strlen(nullTerminatedString);

    for (uint8_t columnIndex = 0; columnIndex < maximumColumnCount; ++columnIndex) {
        if (columnIndex < stringLength) {
            rowHeadPointer[columnIndex] = nullTerminatedString[columnIndex];
        } else {
            rowHeadPointer[columnIndex] = ' ';
        }
    }
    rowHeadPointer[maximumColumnCount] = '\0';
}

void CharacterLcdDisplay::setLineBuffer(const char* multiLineArrayPointer[]) {
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
        backlightEnabled = true; // Variable mutations manipulate pure state
        physicalLcdScreen.backlight();
    }
    else if (command == TURN_BACKLIGHT_OFF_COMMAND) {
        backlightEnabled = false;
        physicalLcdScreen.noBacklight();
    }
}

void CharacterLcdDisplay::commitBuffersToPhysicalScreen() {
    for (uint8_t rowIndex = 0; rowIndex < maximumRowCount; ++rowIndex) {
        physicalLcdScreen.setCursor(0, rowIndex);
        char* rowHeadPointer = characterScreenMatrixBuffer + (rowIndex * (maximumColumnCount + 1));
        physicalLcdScreen.print(rowHeadPointer);
    }
}

// Method performs an active query function over the state
bool CharacterLcdDisplay::isBacklightOn() const {
    return backlightEnabled;
}

uint8_t CharacterLcdDisplay::getMaximumRowCount() const { return maximumRowCount; }
uint8_t CharacterLcdDisplay::getMaximumColumnCount() const { return maximumColumnCount; }