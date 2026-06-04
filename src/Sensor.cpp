/**
* @file Sensor.cpp
 * @brief Implements the Sensor base class.
 *
 * Provides core functionality for sensors in the Modest IoT Nano-framework, including event
 * propagation to an assigned handler. Subclasses should configure hardware and define specific
 * event generation logic.
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

// Define the core global standard events once at the framework boundary layer
const Event Sensor::DATA_READ_EVENT = Event(Sensor::DATA_READ_EVENT_IDENTIFIER);
const Event Sensor::MEASURE_DATA_REQUESTED_EVENT = Event(Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);

/**
 * @brief Stores the target GPIO pin and optional event handler for this sensor.
 * @param signalGpioPin GPIO pin associated with the sensor.
 * @param parentHandler Optional parent handler used for event propagation.
 */
Sensor::Sensor(int signalGpioPin, EventHandler* parentHandler)
    : pin(signalGpioPin), eventHandler(parentHandler) {}

/**
 * @brief Processes the event locally and then forwards it to the configured handler.
 * @param event Event to handle.
 */
void Sensor::on(Event event) {
    processEvent(event);
    if (event.identifier == -1)
        return;
    if (eventHandler != nullptr) {
        eventHandler->on(event);
    }
}

/**
 * @brief Replaces the current propagation target for this sensor.
 * @param parentHandler Pointer to the new event handler.
 */
void Sensor::setHandler(EventHandler* parentHandler) {
    eventHandler = parentHandler;
}
