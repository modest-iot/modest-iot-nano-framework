#ifndef BUTTON_H
#define BUTTON_H

/**
 * @file Button.h
 * @brief Declares the Button class.
 * 
 * A concrete sensor class in the Modest IoT Nano-framework for detecting button presses and
 * generating events. It serves as an example of extending the `Sensor` base class for input devices.
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

#include "Sensor.h"

class Button : public Sensor {
private:
    /**
     * @brief Timestamp of the last accepted interrupt, used for debouncing.
     */
    unsigned long lastInterruptTimestamp;
    /**
     * @brief Minimum time between accepted interrupts.
     */
    const unsigned long debounceDelayInMilliseconds;
protected:
    /**
     * @brief Button-specific event processing hook; the pressed event is already validated.
     * @param event Event being processed.
     */
    void processEvent(Event& event) override;
public:
    /**
     * @brief Unique identifier for the button pressed event.
     */
    static const int BUTTON_PRESSED_EVENT_IDENTIFIER = 10;

    /**
     * @brief Predefined event emitted when the button is accepted after debouncing.
     */
    static const Event BUTTON_PRESSED_EVENT;

    /**
     * @brief Constructs a Button sensor.
     * @param signalGpioPin The GPIO pin for the button (configured as INPUT_PULLUP).
     * @param debounceDelay Debounce delay in milliseconds to prevent multiple triggers from a single press (default: 200ms).
     * @param parentHandler Optional handler to receive button events (default: nullptr).
     */
    Button(int signalGpioPin, unsigned long debounceDelay = 200, EventHandler* parentHandler = nullptr);

    /**
     * @brief ISR-safe hook to convert a physical button interrupt into a framework event.
     */
    void handleHardwareInterrupt();
};

#endif // BUTTON_H