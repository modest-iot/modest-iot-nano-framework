#include <cassert>
#include <Arduino.h>
#include "RgbLed.h"

struct Recorder : CommandHandler {
    int count = 0;
    void handle(Command) override { count++; }
};

int main() {
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
    return 0;
}
