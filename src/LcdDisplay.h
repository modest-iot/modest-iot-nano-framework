#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include "Actuator.h"
#include <LiquidCrystal_I2C.h>

/**
 * @brief Concrete actuator for an I2C character LCD display.
 *
 * The display keeps a fixed two-row, sixteen-column buffer in memory and applies it to the
 * physical screen when the update command is handled.
 */
class LcdDisplay : public Actuator {
private:
    /**
     * @brief Wrapped LCD hardware driver instance.
     */
    LiquidCrystal_I2C physicalLcdScreen;
    
    /**
     * @brief Number of supported display rows.
     */
    static const uint8_t MAXIMUM_ROW_COUNT = 2;
    /**
     * @brief Number of supported characters per row.
     */
    static const uint8_t MAXIMUM_COLUMN_COUNT = 16;
    
    /**
     * @brief Fixed-size text buffer mirrored to the LCD hardware.
     */
    char screenBuffer[MAXIMUM_ROW_COUNT][MAXIMUM_COLUMN_COUNT + 1];
    /**
     * @brief Tracks whether the backlight is currently enabled.
     */
    bool isBacklightEnabled;

    // Private helper methods to elevate code clarity
    /**
     * @brief Initializes the physical LCD and applies the boot backlight state.
     */
    void initializeHardwareRegisters();
    /**
     * @brief Writes the cached text buffer to the physical screen.
     */
    void commitBufferToPhysicalScreen();

protected:
    /**
     * @brief Applies actuator commands to the LCD display.
     * @param command Command to execute.
     */
    void executeCommand(Command command) override;

public:
    /**
     * @brief Unique identifier for the text update command.
     */
    static const int UPDATE_TEXT_COMMAND_IDENTIFIER = 200;
    /**
     * @brief Unique identifier for the backlight-on command.
     */
    static const int TURN_BACKLIGHT_ON_COMMAND_IDENTIFIER = 201;
    /**
     * @brief Unique identifier for the backlight-off command.
     */
    static const int TURN_BACKLIGHT_OFF_COMMAND_IDENTIFIER = 202;

    /**
     * @brief Predefined command that copies the cached buffer to the physical display.
     */
    static const Command UPDATE_TEXT_COMMAND;
    /**
     * @brief Predefined command that turns the LCD backlight on.
     */
    static const Command TURN_BACKLIGHT_ON_COMMAND;
    /**
     * @brief Predefined command that turns the LCD backlight off.
     */
    static const Command TURN_BACKLIGHT_OFF_COMMAND;

    /**
     * @brief Creates an LCD actuator and initializes the hardware.
     * @param i2cAddress I2C address of the LCD module.
     * @param enableBacklightAtBoot Whether the backlight should be enabled during initialization.
     * @param parentHandler Optional handler to receive command notifications.
     */
    LcdDisplay(uint8_t i2cAddress, bool enableBacklightAtBoot = true, CommandHandler* parentHandler = nullptr);

    /**
     * @brief Updates a single row of the cached screen buffer.
     * @param rowIndex Zero-based row index.
     * @param nullTerminatedString Text to copy into the row buffer.
     */
    void setScreenBuffer(uint8_t rowIndex, const char* nullTerminatedString);

    /**
     * @brief Updates the entire cached screen buffer using a two-line string array.
     * @param multiLineArrayPointer Array containing one string per row.
     */
    void setScreenBuffer(const char* multiLineArrayPointer[]);
    
    /**
     * @brief Returns whether the LCD backlight is enabled.
     * @return True when the backlight is on.
     */
    bool isBacklightOn() const;
};

#endif // LCD_DISPLAY_H