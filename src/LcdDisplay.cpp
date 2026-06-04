#include "LcdDisplay.h"

const Command LcdDisplay::UPDATE_TEXT_COMMAND = Command(LcdDisplay::UPDATE_TEXT_COMMAND_IDENTIFIER);
const Command LcdDisplay::TURN_BACKLIGHT_ON_COMMAND = Command(LcdDisplay::TURN_BACKLIGHT_ON_COMMAND_IDENTIFIER);
const Command LcdDisplay::TURN_BACKLIGHT_OFF_COMMAND = Command(LcdDisplay::TURN_BACKLIGHT_OFF_COMMAND_IDENTIFIER);

LcdDisplay::LcdDisplay(uint8_t i2cAddress, bool enableBacklightAtBoot, CommandHandler* parentHandler)
    : Actuator(static_cast<int>(i2cAddress), parentHandler), 
      physicalLcdScreen(i2cAddress, MAXIMUM_COLUMN_COUNT, MAXIMUM_ROW_COUNT),
      isBacklightEnabled(enableBacklightAtBoot) 
{
    // Clear display buffer matrices
    for (uint8_t i = 0; i < MAXIMUM_ROW_COUNT; ++i) {
        memset(screenBuffer[i], ' ', MAXIMUM_COLUMN_COUNT);
        screenBuffer[i][MAXIMUM_COLUMN_COUNT] = '\0';
    }
    
    initializeHardwareRegisters();
}

void LcdDisplay::initializeHardwareRegisters() {
    physicalLcdScreen.init();
    if (isBacklightEnabled)
        physicalLcdScreen.backlight();
    else
        physicalLcdScreen.noBacklight();
    physicalLcdScreen.clear();
}

void LcdDisplay::setScreenBuffer(uint8_t rowIndex, const char* nullTerminatedString) {
    if (rowIndex >= MAXIMUM_ROW_COUNT || nullTerminatedString == nullptr)
        return;

    // Safely copy string into buffer matrix, padded to exactly 16 characters
    uint8_t stringLength = strlen(nullTerminatedString);
    for (uint8_t columnIndex = 0; columnIndex < MAXIMUM_COLUMN_COUNT; ++columnIndex)
        screenBuffer[rowIndex][columnIndex] = (columnIndex < stringLength) ? nullTerminatedString[columnIndex] : ' '; // Pad out trailing empty spaces
    screenBuffer[rowIndex][MAXIMUM_COLUMN_COUNT] = '\0';
}

void LcdDisplay::setScreenBuffer(const char* multiLineArrayPointer[]) {
    if (multiLineArrayPointer == nullptr) return;
    for (uint8_t i = 0; i < MAXIMUM_ROW_COUNT; ++i)
        setScreenBuffer(i, multiLineArrayPointer[i]);
}

void LcdDisplay::executeCommand(Command command) {
    if (command == UPDATE_TEXT_COMMAND) {
        commitBufferToPhysicalScreen();
    }
    else if (command == TURN_BACKLIGHT_ON_COMMAND) {
        isBacklightEnabled = true;
        physicalLcdScreen.backlight();
    } 
    else if (command == TURN_BACKLIGHT_OFF_COMMAND) {
        isBacklightEnabled = false;
        physicalLcdScreen.noBacklight();
    }
}

void LcdDisplay::commitBufferToPhysicalScreen() {
    for (uint8_t rowIndex = 0; rowIndex < MAXIMUM_ROW_COUNT; ++rowIndex) {
        physicalLcdScreen.setCursor(0, rowIndex);
        physicalLcdScreen.print(screenBuffer[rowIndex]);
    }
}

bool LcdDisplay::isBacklightOn() const { return isBacklightEnabled; }