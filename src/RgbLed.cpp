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


RgbLed::RgbLed(const int redGpioPin, const int greenGpioPin, const int blueGpioPin,
               const bool isCommonAnode, CommandHandler *parentHandler)
    : Actuator(redGpioPin, parentHandler),
      greenPin(greenGpioPin), bluePin(blueGpioPin), commonAnode(isCommonAnode),
      bufferedRed(MAXIMUM_INTENSITY), bufferedGreen(MAXIMUM_INTENSITY), bufferedBlue(MAXIMUM_INTENSITY),
      activeRed(MAXIMUM_INTENSITY), activeGreen(MAXIMUM_INTENSITY), activeBlue(MAXIMUM_INTENSITY),
      active(false) {
    pinMode(pin, OUTPUT);
    pinMode(greenPin, OUTPUT);
    pinMode(bluePin, OUTPUT);
    writeOutputs();
}

void RgbLed::writeColorChannel(int gpioPin, const uint8_t intensity) const {
    analogWrite(gpioPin, commonAnode ? MAXIMUM_INTENSITY - intensity : intensity);
}

void RgbLed::writeOutputs() const {
    writeColorChannel(pin, active ? activeRed : 0);
    writeColorChannel(greenPin, active ? activeGreen : 0);
    writeColorChannel(bluePin, active ? activeBlue : 0);
}

void RgbLed::executeCommand(Command command) {
    if (command == TURN_ON_COMMAND) {
        active = true;
    } else if (command == TURN_OFF_COMMAND) {
        active = false;
    } else if (command == TOGGLE_COMMAND) {
        active = !active;
    } else if (command == SET_COLOR_COMMAND) {
        activeRed = bufferedRed;
        activeGreen = bufferedGreen;
        activeBlue = bufferedBlue;
        active = true;
    } else {
        return;
    }
    writeOutputs();
}

void RgbLed::setColor(uint8_t red, uint8_t green, uint8_t blue) {
    bufferedRed = red;
    bufferedGreen = green;
    bufferedBlue = blue;
}

bool RgbLed::isActive() const { return active; }
uint8_t RgbLed::getRed() const { return activeRed; }
uint8_t RgbLed::getGreen() const { return activeGreen; }
uint8_t RgbLed::getBlue() const { return activeBlue; }
