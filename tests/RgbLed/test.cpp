#include <cassert>
#include <Arduino.h>
#include "RgbLed.h"

namespace {
struct Recorder : CommandHandler {
    int count = 0;
    void handle(const Command) override { count++; }
};
} // namespace

[[nodiscard]] int main() {
    Recorder parent;
    RgbLed led(1, 2, 3, false, &parent);
    assert(!led.isActive() && fakePwm[1] == 0 && fakePwm[2] == 0 && fakePwm[3] == 0);

    led.handle(RgbLed::TURN_ON_COMMAND);
    assert(led.isActive() && fakePwm[1] == 255 && fakePwm[2] == 255 && fakePwm[3] == 255);

    led.setColor(10, 20, 30);
    assert(fakePwm[1] == 255);
    led.handle(RgbLed::SET_COLOR_COMMAND);
    assert(fakePwm[1] == 10 && fakePwm[2] == 20 && fakePwm[3] == 30);
    assert(led.getRedIntensity() == 10 && led.getGreenIntensity() == 20 && led.getBlueIntensity() == 30);

    led.handle(RgbLed::TURN_OFF_COMMAND);
    assert(!led.isActive() && fakePwm[1] == 0 && led.getRedIntensity() == 10);
    led.handle(RgbLed::TOGGLE_LED_COMMAND);
    assert(led.isActive() && fakePwm[3] == 30);
    led.handle(RgbLed::TOGGLE_LED_COMMAND);
    assert(!led.isActive());

    assert(parent.count == 5);

    RgbLed anode(4, 5, 6, true);
    assert(fakePwm[4] == 255 && fakePwm[5] == 255 && fakePwm[6] == 255);
    anode.setColor(255, 0, 100);
    anode.handle(RgbLed::SET_COLOR_COMMAND);
    assert(fakePwm[4] == 0 && fakePwm[5] == 255 && fakePwm[6] == 155);

    led.setColor(RgbLed::ORANGE);
    led.handle(RgbLed::SET_COLOR_COMMAND);
    assert(led.getRedIntensity() == 255 && led.getGreenIntensity() == 165 && led.getBlueIntensity() == 0);
    assert(fakePwm[1] == 255 && fakePwm[2] == 165 && fakePwm[3] == 0);

    anode.setColor(RgbLed::CYAN);
    anode.handle(RgbLed::SET_COLOR_COMMAND);
    assert(fakePwm[4] == 255 && fakePwm[5] == 0 && fakePwm[6] == 0);

    // Test: ADJUST_BRIGHTNESS_COMMAND scales active color by payload/255
    led.setColor(200, 100, 50);
    led.handle(RgbLed::SET_COLOR_COMMAND);
    assert(led.getRedIntensity() == 200 && led.getGreenIntensity() == 100 && led.getBlueIntensity() == 50);
    led.handle(Command{RgbLed::ADJUST_BRIGHTNESS_COMMAND_IDENTIFIER, 127}); // ~50% brightness
    assert(led.getRedIntensity() == 99 && led.getGreenIntensity() == 49 && led.getBlueIntensity() == 24);

    // Test: ADJUST_BRIGHTNESS_COMMAND with payload 255 leaves color unchanged
    led.setColor(255, 255, 255);
    led.handle(RgbLed::SET_COLOR_COMMAND);
    led.handle(Command{RgbLed::ADJUST_BRIGHTNESS_COMMAND_IDENTIFIER, 255});
    assert(led.getRedIntensity() == 255 && led.getGreenIntensity() == 255 && led.getBlueIntensity() == 255);

    // Test: ADJUST_BRIGHTNESS_COMMAND with payload 0 turns off all channels
    led.setColor(128, 64, 32);
    led.handle(RgbLed::SET_COLOR_COMMAND);
    led.handle(Command{RgbLed::ADJUST_BRIGHTNESS_COMMAND_IDENTIFIER, 0});
    assert(led.getRedIntensity() == 0 && led.getGreenIntensity() == 0 && led.getBlueIntensity() == 0);

    // Test: adjustColorSetIntensity() scales all channels directly
    led.setColor(200, 100, 50);
    led.handle(RgbLed::SET_COLOR_COMMAND);
    assert(led.getRedIntensity() == 200 && led.getGreenIntensity() == 100 && led.getBlueIntensity() == 50);
    led.adjustColorSetIntensity(64); // ~25% brightness
    assert(led.getRedIntensity() == 50 && led.getGreenIntensity() == 25 && led.getBlueIntensity() == 12);

    // Test: adjustColorSetIntensity(255) leaves intensities unchanged
    led.setColor(128, 64, 32);
    led.handle(RgbLed::SET_COLOR_COMMAND);
    led.adjustColorSetIntensity(255);
    assert(led.getRedIntensity() == 128 && led.getGreenIntensity() == 64 && led.getBlueIntensity() == 32);

    return 0;
}
