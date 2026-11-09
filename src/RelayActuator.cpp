/**
 * @file RelayActuator.cpp
 * @brief Implements command-driven relay state transitions.
 * @date 2026-11-09
 * @version 1.0
 */

#include "RelayActuator.h"
#include <Arduino.h>


RelayActuator::RelayActuator(const int signalGpioPin, const bool initialState, CommandHandler *parentHandler)
    : Actuator(signalGpioPin, parentHandler), active(initialState) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, active ? HIGH : LOW);
}

void RelayActuator::executeCommand(Command command) {
    if (command == RELAY_ON_COMMAND) {
        active = true;
    } else if (command == RELAY_OFF_COMMAND) {
        active = false;
    } else if (command == TOGGLE_RELAY_COMMAND) {
        active = !active;
    }
    digitalWrite(pin, active ? HIGH : LOW);
}

bool RelayActuator::isActive() const { return active; }
