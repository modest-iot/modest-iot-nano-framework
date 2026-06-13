/**
* @file Sensor.cpp
 * @brief Implements the Sensor base class.
 *
 * Provides core functionality for sensors in the Modest IoT Nano-framework, including event
 * propagation to an assigned handler. Subclasses should configure hardware and define specific
 * event generation logic.
 *
 * @author Angel Velasquez
 * @date 2026-06-06
 * @version 1.0
 */

/*
 * This file is part of the Modest IoT Nano-framework (C++ Edition).
 * Copyright (c) 2025-2026 Angel Velasquez
 *
 * Licensed under the MIT License.
 * See LICENSE.md in the root of this repository for full terms.
 */

#include "Sensor.h"

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
    // 1. Execute concrete sensor reading and validation logic
    processEvent(event);

    // 2. Performance Guard: If event mutation sets ID to -1, propagation terminates early
    if (event.identifier == -1)
        return;

    // 3. Automatically push event upstream to the registered mediator device
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