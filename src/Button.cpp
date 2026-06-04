/**
* @file Button.cpp
 * @brief Implements the Button class.
 *
 * Configures a button as an input device in the Modest IoT Nano-framework, setting up the pin
 * with an internal pull-up resistor. Event generation (e.g., BUTTON_PRESSED_EVENT) is typically
 * triggered externally via interrupt or polling in user code.
 *
 * @author Angel Velasquez
 * @date March 22, 2025
 * @version 0.1
 */

/*
 * This file is part of the Modest IoT Nano-framework (C++ Edition).
 * Copyright (c) 2025 Angel Velasquez
 *
 * Licensed under the Creative Commons Attribution-NoDerivatives 4.0 International (CC BY-ND 4.0).
 * You may use, copy, and distribute this software in its original, unmodified form, provided
 * you give appropriate credit to the original author (Angel Velasquez) and include this notice.
 * Modifications, adaptations, or derivative works are not permitted.
 *
 * Full license text: https://creativecommons.org/licenses/by-nd/4.0/legalcode
 */

#include "Button.h"
#include <Arduino.h>

/**
 * @brief Predefined event emitted for a validated button press.
 */
const Event Button::BUTTON_PRESSED_EVENT = Event(BUTTON_PRESSED_EVENT_IDENTIFIER);

/**
 * @brief Initializes the button sensor and configures the input pin with an internal pull-up.
 * @param signalGpioPin GPIO pin connected to the button.
 * @param debounceDelay Debounce interval in milliseconds.
 * @param parentHandler Optional parent handler used for event propagation.
 */
Button::Button(int signalGpioPin, unsigned long debounceDelay, EventHandler* parentHandler)
    : Sensor(signalGpioPin, parentHandler),
lastInterruptTimestamp(0),
debounceDelayInMilliseconds(debounceDelay){
    pinMode(signalGpioPin, INPUT_PULLUP);
}

/**
 * @brief Converts a hardware interrupt into a debounced button-press event.
 */
void Button::handleHardwareInterrupt() {
    unsigned long currentTimestamp = millis();

    // Encapsulated temporal tracking logic handles debouncing internally
    if (currentTimestamp - lastInterruptTimestamp > debounceDelayInMilliseconds) {
        lastInterruptTimestamp = currentTimestamp;

        // Safely invoke the template method pipeline
        this->on(BUTTON_PRESSED_EVENT);
    }
}

/**
 * @brief Button events are terminal and require no local mutation before propagation.
 * @param event Event being processed.
 */
void Button::processEvent(Event& event) {
    // The button interrupt enforces its own valid event; no further mutation is required
}

