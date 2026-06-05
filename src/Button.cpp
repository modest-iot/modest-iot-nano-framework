#include "Button.h"
#include <Arduino.h>

const Event Button::BUTTON_PRESSED_EVENT = Event(Button::BUTTON_PRESSED_EVENT_IDENTIFIER);

Button::Button(int signalGpioPin, unsigned long debounceDelay, EventHandler *parentHandler)
    : Sensor(signalGpioPin, parentHandler),
      lastInterruptTimestamp(0),
      debounceDelayInMilliseconds(debounceDelay) {
    pinMode(pin, INPUT_PULLUP);
}

void Button::handleHardwareInterrupt() {
    unsigned long currentTimestamp = millis();
    if (currentTimestamp - lastInterruptTimestamp > debounceDelayInMilliseconds) {
        lastInterruptTimestamp = currentTimestamp;
        this->on(BUTTON_PRESSED_EVENT);
    }
}

void Button::processEvent(Event &event) {
}
