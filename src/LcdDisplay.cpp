#include "LcdDisplay.h"

/**
 * @brief Predefined command that copies the cached text buffer to the display.
 */
const Command LcdDisplay::UPDATE_TEXT_COMMAND = Command(LcdDisplay::UPDATE_TEXT_COMMAND_IDENTIFIER);
/**
 * @brief Predefined command that enables the LCD backlight.
 */
const Command LcdDisplay::TURN_BACKLIGHT_ON_COMMAND = Command(LcdDisplay::TURN_BACKLIGHT_ON_COMMAND_IDENTIFIER);
/**
 * @brief Predefined command that disables the LCD backlight.
 */
const Command LcdDisplay::TURN_BACKLIGHT_OFF_COMMAND = Command(LcdDisplay::TURN_BACKLIGHT_OFF_COMMAND_IDENTIFIER);

/**
 * @brief Constructs the LCD display wrapper and initializes the hardware buffer and controller.
 * @param i2cAddress I2C address of the LCD module.
 * @param enableBacklightAtBoot Whether the backlight should be on after initialization.
 * @param parentHandler Optional parent handler used for command propagation.
 */
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

/**
 * @brief Initializes the physical LCD controller and applies the configured backlight state.
 */
void LcdDisplay::initializeHardwareRegisters() {
    physicalLcdScreen.init();
    if (isBacklightEnabled)
        physicalLcdScreen.backlight();
    else
        physicalLcdScreen.noBacklight();
    physicalLcdScreen.clear();
}

/**
 * @brief Replaces a single row in the cached screen buffer.
 * @param rowIndex Zero-based row index.
 * @param nullTerminatedString Text to copy into the row buffer.
 */
void LcdDisplay::setScreenBuffer(uint8_t rowIndex, const char* nullTerminatedString) {
    if (rowIndex >= MAXIMUM_ROW_COUNT || nullTerminatedString == nullptr)
        return;

    // Safely copy string into buffer matrix, padded to exactly 16 characters
    uint8_t stringLength = strlen(nullTerminatedString);
    for (uint8_t columnIndex = 0; columnIndex < MAXIMUM_COLUMN_COUNT; ++columnIndex)
        screenBuffer[rowIndex][columnIndex] = (columnIndex < stringLength) ? nullTerminatedString[columnIndex] : ' '; // Pad out trailing empty spaces
    screenBuffer[rowIndex][MAXIMUM_COLUMN_COUNT] = '\0';
}

/**
 * @brief Replaces the complete cached screen buffer from a string array.
 * @param multiLineArrayPointer Array containing one string per row.
 */
void LcdDisplay::setScreenBuffer(const char* multiLineArrayPointer[]) {
    if (multiLineArrayPointer == nullptr) return;
    for (uint8_t i = 0; i < MAXIMUM_ROW_COUNT; ++i)
        setScreenBuffer(i, multiLineArrayPointer[i]);
}

/**
 * @brief Applies the requested command to the LCD hardware.
 * @param command Command to execute.
 */
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

/**
 * @brief Writes each cached row to the physical LCD controller.
 */
void LcdDisplay::commitBufferToPhysicalScreen() {
    for (uint8_t rowIndex = 0; rowIndex < MAXIMUM_ROW_COUNT; ++rowIndex) {
        physicalLcdScreen.setCursor(0, rowIndex);
        physicalLcdScreen.print(screenBuffer[rowIndex]);
    }
}

/**
 * @brief Indicates whether the LCD backlight is currently enabled.
 * @return True when the backlight is on.
 */
bool LcdDisplay::isBacklightOn() const { return isBacklightEnabled; }