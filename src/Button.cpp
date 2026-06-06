/**
 * @file Button.cpp
 * @brief Implements debounced button sampling and press-event emission.
 */

#include "Button.h"
#include <Arduino.h>

const Event Button::BUTTON_PRESSED_EVENT = Event(Button::BUTTON_PRESSED_EVENT_IDENTIFIER);

Button::Button(int signalGpioPin, unsigned long debounceDelayInMilliseconds, EventHandler* parentHandler)
    : Sensor(signalGpioPin, parentHandler),
      lastStableTimestamp(0),
      debounceDelayInMilliseconds(debounceDelayInMilliseconds),
      previousState(HIGH)
{
    pinMode(pin, INPUT_PULLUP);
}

void Button::processEvent(Event& event) {
    if (event.identifier == MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        unsigned long currentTimestamp = millis();
        bool currentState = digitalRead(pin);

        if (currentState != previousState) {
            if (currentTimestamp - lastStableTimestamp > debounceDelayInMilliseconds) {
                lastStableTimestamp = currentTimestamp;
                previousState = currentState;

                if (currentState == LOW) {
                    event = BUTTON_PRESSED_EVENT;
                    return;
                }
            }
        }
        event.identifier = -1;
    }
}