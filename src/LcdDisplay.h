#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include "Actuator.h"
#include <LiquidCrystal_I2C.h>

class LcdDisplay : public Actuator {
private:
    LiquidCrystal_I2C physicalLcdScreen;
    
    static const uint8_t MAXIMUM_ROW_COUNT = 2;
    static const uint8_t MAXIMUM_COLUMN_COUNT = 16;
    
    char screenBuffer[MAXIMUM_ROW_COUNT][MAXIMUM_COLUMN_COUNT + 1];
    bool isBacklightEnabled;

    // Private helper methods to elevate code clarity
    void initializeHardwareRegisters();
    void commitBufferToPhysicalScreen();

protected:
    void executeCommand(Command command) override;

public:
    static const int UPDATE_TEXT_COMMAND_IDENTIFIER = 200;
    static const int TURN_BACKLIGHT_ON_COMMAND_IDENTIFIER = 201;
    static const int TURN_BACKLIGHT_OFF_COMMAND_IDENTIFIER = 202;

    static const Command UPDATE_TEXT_COMMAND;
    static const Command TURN_BACKLIGHT_ON_COMMAND;
    static const Command TURN_BACKLIGHT_OFF_COMMAND;

    LcdDisplay(uint8_t i2cAddress, bool enableBacklightAtBoot = true, CommandHandler* parentHandler = nullptr);

    void setScreenBuffer(uint8_t rowIndex, const char* nullTerminatedString);
    void setScreenBuffer(const char* multiLineArrayPointer[]);
    
    bool isBacklightOn() const;
};

#endif // LCD_DISPLAY_H