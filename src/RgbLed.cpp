/**
 * @file RgbLed.cpp
 * @brief Implements command-driven RGB LED color and state transitions.
 * @date 2026-10-31
 * @version 1.0
 */

#include "RgbLed.h"
#include <Arduino.h>

namespace {
    constexpr uint8_t MAXIMUM_INTENSITY = 255;
}


/**
 * @details Buffered and active colors default to white, so a `TURN_ON_COMMAND` issued before
 *          any color is set lights the LED white.
 */
RgbLed::RgbLed(const int redGpioPin, const int greenGpioPin, const int blueGpioPin,
               const bool isCommonAnode, CommandHandler *parentHandler)
    : Actuator(redGpioPin, parentHandler),
      greenPin(greenGpioPin), bluePin(blueGpioPin), commonAnode(isCommonAnode),
      bufferedRedIntensity(MAXIMUM_INTENSITY), bufferedGreenIntensity(MAXIMUM_INTENSITY), bufferedBlueIntensity(MAXIMUM_INTENSITY),
      activeRedIntensity(MAXIMUM_INTENSITY), activeGreenIntensity(MAXIMUM_INTENSITY), activeBlueIntensity(MAXIMUM_INTENSITY),
      active(false) {
    pinMode(pin, OUTPUT);
    pinMode(greenPin, OUTPUT);
    pinMode(bluePin, OUTPUT);
    applyState();
}

/**
 * @details For common-anode LEDs the duty cycle is inverted (`255 - intensity`), so that
 *          logical intensity 255 is always the brightest output.
 */
void RgbLed::writeColorChannelIntensity(int gpioPin, const uint8_t intensity) const {
    analogWrite(gpioPin, commonAnode ? MAXIMUM_INTENSITY - intensity : intensity);
}

void RgbLed::applyState() const {
    writeColorChannelIntensity(pin, active ? activeRedIntensity : 0);
    writeColorChannelIntensity(greenPin, active ? activeGreenIntensity : 0);
    writeColorChannelIntensity(bluePin, active ? activeBlueIntensity : 0);
}

/**
 * @details `SET_COLOR_COMMAND` copies the buffered color to the active color and lights the LED.
 *          Outputs are rewritten only for recognized commands.
 */
void RgbLed::executeCommand(Command command) {
    if (command == TURN_ON_COMMAND) {
        active = true;
    } else if (command == TURN_OFF_COMMAND) {
        active = false;
    } else if (command == TOGGLE_LED_COMMAND) {
        active = !active;
    } else if (command == SET_COLOR_COMMAND) {
        activeRedIntensity = bufferedRedIntensity;
        activeGreenIntensity = bufferedGreenIntensity;
        activeBlueIntensity = bufferedBlueIntensity;
        active = true;
    } else {
        return;
    }
    applyState();
}

void RgbLed::setColor(const uint8_t redIntensity, const uint8_t greenIntensity, const uint8_t blueIntensity) {
    bufferedRedIntensity = redIntensity;
    bufferedGreenIntensity = greenIntensity;
    bufferedBlueIntensity = blueIntensity;
}

void RgbLed::setColor(const RgbColor& color) {
    setColor(color.redIntensity, color.greenIntensity, color.blueIntensity);
}

bool RgbLed::isActive() const { return active; }
uint8_t RgbLed::getRedIntensity() const { return activeRedIntensity; }
uint8_t RgbLed::getGreenIntensity() const { return activeGreenIntensity; }
uint8_t RgbLed::getBlueIntensity() const { return activeBlueIntensity; }
