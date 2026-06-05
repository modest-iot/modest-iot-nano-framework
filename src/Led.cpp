#include "Led.h"
#include <Arduino.h>

const Command Led::TURN_ON_COMMAND = Command(Led::TURN_ON_COMMAND_IDENTIFIER);
const Command Led::TURN_OFF_COMMAND = Command(Led::TURN_OFF_COMMAND_IDENTIFIER);
const Command Led::TOGGLE_LED_COMMAND = Command(Led::TOGGLE_LED_COMMAND_IDENTIFIER);

Led::Led(int signalGpioPin, bool initialState, CommandHandler* parentHandler)
    : Actuator(signalGpioPin, parentHandler), state(initialState) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, state ? HIGH : LOW);
}

void Led::executeCommand(Command command) {
    if (command == TURN_ON_COMMAND) {
        state = true;
    } 
    else if (command == TURN_OFF_COMMAND) {
        state = false;
    } 
    else if (command == TOGGLE_LED_COMMAND) {
        state = !state;
    }
    digitalWrite(pin, state ? HIGH : LOW);
}

bool Led::isActive() const { return state; }