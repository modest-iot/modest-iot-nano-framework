#ifndef CHARACTER_LCD_DISPLAY_H
#define CHARACTER_LCD_DISPLAY_H

/**
 * @file CharacterLcdDisplay.h
 * @brief Declares an I2C character LCD actuator with buffered text updates.
 * @ingroup actuators_module
 * @date 2026-06-06
 * @version 1.0
 */

#include "Actuator.h"
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

/**
 * @name Actuators: Display & Character Output
 * @ingroup actuators_module
 * @{
 */

/**
 * @brief Actuator adapter for HD44780-compatible character Liquid Crystal displays over I2C.
 * @ingroup actuators_module
 */
class CharacterLcdDisplay : public Actuator {
private:
    LiquidCrystal_I2C physicalLcdScreen; ///< Underlying LiquidCrystal_I2C driver instance.

    // Instance-specific structural dimensions assigned at construction
    const uint8_t maximumRowCount; ///< Maximum number of rows supported.
    const uint8_t maximumColumnCount; ///< Maximum number of columns supported.

    // Contiguous memory allocation buffer tracking absolute dimensions safely
    char* characterScreenMatrixBuffer; ///< Buffer for storing characters to be displayed.

    // ALIGNED PARADIGM: Variables hold raw state configurations without 'is' or 'has' prefixes
    bool backlightEnabled; ///< Backlight state (true = on, false = off).

    // Private encapsulated layout routines to elevate code clarity
    void initializeHardwareRegisters(); ///< Initializes hardware registers for the LCD.
    void commitBuffersToPhysicalScreen(); ///< Writes buffered data to the physical screen.

protected:
    /**
     * @brief Intercepts target display commands to commit textual changes or modify backlight states.
     */
    void executeCommand(Command command) override;

public:
    static const int UPDATE_TEXT_COMMAND_IDENTIFIER = 200;
    static const int TURN_BACKLIGHT_ON_COMMAND_IDENTIFIER = 201;
    static const int TURN_BACKLIGHT_OFF_COMMAND_IDENTIFIER = 202;

    static const Command UPDATE_TEXT_COMMAND;
    static const Command TURN_BACKLIGHT_ON_COMMAND;
    static const Command TURN_BACKLIGHT_OFF_COMMAND;

    /**
     * @param i2cAddress Mapped to the base framework 'pin' attribute via casting.
     * @param columnCount Total number of columns (e.g., 16, 20, 40).
     * @param rowCount Total number of rows (e.g., 2, 4).
     * @param backlightInitialState Backlight state configuration upon physical initialization.
     * @param parentHandler Base framework coordinator pointer connection.
     */
    CharacterLcdDisplay(uint8_t i2cAddress, uint8_t columnCount, uint8_t rowCount,
                        bool backlightInitialState = true, CommandHandler* parentHandler = nullptr);

    ~CharacterLcdDisplay() override;

    /**
     * @brief Stages a clean string into a target row index, automatically zero-padded.
     * @param rowIndex Target row to update.
     * @param nullTerminatedString Source string to copy into the row buffer.
     */
    void setLineBuffer(uint8_t rowIndex, const char* nullTerminatedString);

    /**
     * @brief Composite helper allowing multi-row string pointer arrays to be loaded concurrently.
     * @param multiLineArrayPointer Null-terminated array of C-string row values.
     */
    void setLineBuffer(const char* multiLineArrayPointer[]);

    // ALIGNED PARADIGM: Active interrogative queries are reserved exclusively for methods
    /**
     * @brief Reports whether the backlight is currently enabled.
     * @return `true` when backlight is on, otherwise `false`.
     */
    bool isBacklightOn() const;

    /** @brief Returns the number of addressable rows on this display. */
    uint8_t getMaximumRowCount() const;
    /** @brief Returns the number of addressable columns on this display. */
    uint8_t getMaximumColumnCount() const;
};

/** @} */ // End of Actuators: Display & Character Output

#endif // CHARACTER_LCD_DISPLAY_H