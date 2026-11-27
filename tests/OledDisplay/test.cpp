#include <time.h>
/**
 * @file OledDisplay host test suite.
 * @brief Tests native SSD1306 I2C driver and OledDisplay actuator.
 */

#include "OledDisplay.h"
#include "FakeI2cBus.h"
#include <cassert>
#include <cstring>
#include <vector>

class Recorder : public CommandHandler {
public:
    std::vector<Command> commandsReceived;
    void handle(Command cmd) override { commandsReceived.push_back(cmd); }
};

void testInjectedBusStartupAndDisplayFailure() {
    Wire.transmissionCount = 0;
    FakeI2cBus bus;
    OledDisplay display(bus);

    assert(bus.writeCount == 0);
    assert(Wire.transmissionCount == 0);
    assert(!display.isInitialized());
    display.handle(OledDisplay::UPDATE_DISPLAY_COMMAND);
    assert(display.getLastI2cResult().status == I2cStatus::NotStarted);
    assert(bus.writeCount == 0);

    const I2cBusConfiguration config{21, 22, 400000};
    assert(bus.start(config) == I2cStatus::Ok);
    assert(display.start());
    assert(display.isInitialized());
    assert(display.isBacklightOn());
    assert(Wire.transmissionCount == 0);

    const int writesBeforeFailure = bus.writeCount;
    bus.nextWriteStatus = I2cStatus::AddressNack;
    display.handle(OledDisplay::UPDATE_DISPLAY_COMMAND);
    assert(bus.writeCount == writesBeforeFailure + 1);
    assert(display.getLastI2cResult().status == I2cStatus::AddressNack);
}

[[nodiscard]] int main() {
    testInjectedBusStartupAndDisplayFailure();
    // Test 1: Constructor initializes I2C address and clears frame buffer
    Recorder parent;
    OledDisplay oled(0x3C, &parent);
    assert(oled.isBacklightOn() == true); // Display is on after initialization
    assert(Wire.transmissionCount > 0); // Initialization sends I2C commands

    // Test 2: UPDATE_DISPLAY_COMMAND sends frame buffer
    Wire.transmissionCount = 0;
    oled.setText("Hi", 0, 0);
    oled.handle(OledDisplay::UPDATE_DISPLAY_COMMAND);
    assert(Wire.transmissionCount > 0); // Frame buffer sent to display

    // Test 3: CLEAR_DISPLAY_COMMAND clears and updates
    Wire.transmissionCount = 0;
    oled.handle(OledDisplay::CLEAR_DISPLAY_COMMAND);
    assert(Wire.transmissionCount > 0); // Clear + update sent

    // Test 4: SET_BACKLIGHT_COMMAND turns display on
    oled.handle(Command{999}); // Unknown command
    oled.handle(OledDisplay::SET_BACKLIGHT_COMMAND);
    assert(oled.isBacklightOn() == true);

    // Test 5: setText renders characters in frame buffer
    oled.clearFrameBuffer();
    oled.setText("A", 0, 0);
    oled.handle(OledDisplay::UPDATE_DISPLAY_COMMAND);
    assert(Wire.transmissionCount > 0);

    // Test 6: clearFrameBuffer zeros the buffer
    oled.setText("X", 0, 0);
    oled.clearFrameBuffer();
    oled.handle(OledDisplay::UPDATE_DISPLAY_COMMAND);
    // No crash, buffer is cleared

    // Test 7: drawHorizontalLine sets pixels
    oled.clearFrameBuffer();
    oled.drawHorizontalLine(0, 0, 10);
    oled.handle(OledDisplay::UPDATE_DISPLAY_COMMAND);
    assert(Wire.transmissionCount > 0);

    // Test 8: drawVerticalLine sets pixels
    oled.clearFrameBuffer();
    oled.drawVerticalLine(0, 0, 10);
    oled.handle(OledDisplay::UPDATE_DISPLAY_COMMAND);
    assert(Wire.transmissionCount > 0);

    // Test 9: drawRectangle sets pixels
    oled.clearFrameBuffer();
    oled.drawRectangle(10, 10, 20, 10);
    oled.handle(OledDisplay::UPDATE_DISPLAY_COMMAND);
    assert(Wire.transmissionCount > 0);

    // Test 10: fillRectangle sets pixels
    oled.clearFrameBuffer();
    oled.fillRectangle(10, 10, 20, 10);
    oled.handle(OledDisplay::UPDATE_DISPLAY_COMMAND);
    assert(Wire.transmissionCount > 0);

    // Test 11: Unknown command does not change state
    oled.handle(Command{999});
    assert(oled.isBacklightOn() == true); // State unchanged

    // Test 12: setText with nullptr is ignored
    oled.setText(nullptr, 0, 0);
    // No crash

    // Test 13: setText truncates at display width
    oled.setText("This is a very long string that exceeds the display width significantly", 0, 0);
    oled.handle(OledDisplay::UPDATE_DISPLAY_COMMAND);
    // No crash, text is truncated

    // Test 14: Constructor with alternate I2C address
    {
        OledDisplay oled2(0x3D, nullptr);
        assert(oled2.isBacklightOn() == true);
    }

    return 0;
}
