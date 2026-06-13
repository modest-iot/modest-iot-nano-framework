/**
 * @file Led.cpp
 * @brief Implements command-driven LED state transitions.
 * @date 2026-06-06
 * @version 1.0
 */

#include "Led.h"
#include <Arduino.h>


Led::Led(int signalGpioPin, bool initialState, CommandHandler* parentHandler)
    : Actuator(signalGpioPin, parentHandler), active(initialState) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, active ? HIGH : LOW);
}

void Led::executeCommand(Command command) {
    if (command == TURN_ON_COMMAND) {
        active = true;
    } 
    else if (command == TURN_OFF_COMMAND) {
        active = false;
    } 
    else if (command == TOGGLE_LED_COMMAND) {
        active = !active;
    }
    digitalWrite(pin, active ? HIGH : LOW);
}

bool Led::isActive() const { return active; }