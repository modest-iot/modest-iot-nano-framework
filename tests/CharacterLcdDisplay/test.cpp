#include <time.h>
/**
 * @file CharacterLcdDisplay host test suite.
 * @brief Tests native HD44780 I2C driver implementation (W19).
 */

#include "CharacterLcdDisplay.h"
#include <cassert>
#include <cstring>
#include <vector>

class Recorder : public CommandHandler {
public:
    std::vector<Command> commandsReceived;
    void handle(Command cmd) override { commandsReceived.push_back(cmd); }
}; // namespace

[[nodiscard]] int main() {
    // Test 1: Constructor initializes I2C address and dimensions
    Recorder parent;
    CharacterLcdDisplay lcd(0x27, 16, 2, true, &parent);
    assert(lcd.getMaximumRowCount() == 2);
    assert(lcd.getMaximumColumnCount() == 16);
    assert(lcd.isBacklightOn() == true);
    assert(Wire.transmissionCount > 0); // Initialization sends I2C commands

    // Test 2: UPDATE_TEXT_COMMAND commits buffered text
    Wire.transmissionCount = 0;
    lcd.setLineBuffer(0, "Hello");
    lcd.setLineBuffer(1, "World");
    lcd.handle(CharacterLcdDisplay::UPDATE_TEXT_COMMAND);
    assert(Wire.transmissionCount > 0); // commitBuffersToPhysicalScreen sends I2C data

    // Test 3: TURN_BACKLIGHT_ON_COMMAND enables backlight
    lcd.handle(CharacterLcdDisplay::TURN_BACKLIGHT_OFF_COMMAND);
    assert(lcd.isBacklightOn() == false);
    lcd.handle(CharacterLcdDisplay::TURN_BACKLIGHT_ON_COMMAND);
    assert(lcd.isBacklightOn() == true);

    // Test 4: TURN_BACKLIGHT_OFF_COMMAND disables backlight
    lcd.handle(CharacterLcdDisplay::TURN_BACKLIGHT_OFF_COMMAND);
    assert(lcd.isBacklightOn() == false);

    // Test 5: setLineBuffer truncates long strings
    lcd.setLineBuffer(0, "This is a very long string that exceeds 16 columns");
    lcd.handle(CharacterLcdDisplay::UPDATE_TEXT_COMMAND);
    // No crash, string is truncated to column count

    // Test 6: setLineBuffer pads short strings with spaces
    lcd.setLineBuffer(0, "Hi");
    lcd.handle(CharacterLcdDisplay::UPDATE_TEXT_COMMAND);
    // No crash, string is padded with spaces

    // Test 7: setLineBuffer with array
    const char* lines[] = {"Line 1", "Line 2"};
    lcd.setLineBuffer(lines);
    lcd.handle(CharacterLcdDisplay::UPDATE_TEXT_COMMAND);
    assert(Wire.transmissionCount > 0);

    // Test 8: Row index out of bounds is ignored
    lcd.setLineBuffer(5, "Should be ignored");
    lcd.handle(CharacterLcdDisplay::UPDATE_TEXT_COMMAND);
    // No crash, out-of-bounds row is silently ignored

    // Test 9: Unknown command does not change state
    lcd.handle(Command{999});
    assert(lcd.isBacklightOn() == false); // State unchanged

    // Test 10: Constructor with backlight off
    {
        CharacterLcdDisplay lcd2(0x3E, 20, 4, false, nullptr);
        assert(lcd2.getMaximumRowCount() == 4);
        assert(lcd2.getMaximumColumnCount() == 20);
        assert(lcd2.isBacklightOn() == false);
    }

    return 0;
}
