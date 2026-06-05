#ifndef CHARACTER_LCD_DISPLAY_H
#define CHARACTER_LCD_DISPLAY_H

#include "Actuator.h"
#include <LiquidCrystal_I2C.h>

class CharacterLcdDisplay : public Actuator {
private:
    LiquidCrystal_I2C physicalLcdScreen;

    // Instance-specific structural dimensions assigned at construction
    const uint8_t maximumRowCount;
    const uint8_t maximumColumnCount;

    // Contiguous memory allocation buffer tracking absolute dimensions safely
    char* characterScreenMatrixBuffer;

    // ALIGNED PARADIGM: Variables hold raw state configurations without 'is' or 'has' prefixes
    bool backlightEnabled;

    // Private encapsulated layout routines to elevate code clarity
    void initializeHardwareRegisters();
    void commitBuffersToPhysicalScreen();

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

    virtual ~CharacterLcdDisplay() override;

    /**
     * @brief Stages a clean string into a target row index, automatically zero-padded.
     */
    void setLineBuffer(uint8_t rowIndex, const char* nullTerminatedString);

    /**
     * @brief Composite helper allowing multi-row string pointer arrays to be loaded concurrently.
     */
    void setLineBuffer(const char* multiLineArrayPointer[]);

    // ALIGNED PARADIGM: Active interrogative queries are reserved exclusively for methods
    bool isBacklightOn() const;

    uint8_t getMaximumRowCount() const;
    uint8_t getMaximumColumnCount() const;
};

#endif // CHARACTER_LCD_DISPLAY_H