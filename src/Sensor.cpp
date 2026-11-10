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
    : pin(signalGpioPin), eventHandler(parentHandler), consecutiveFailures(0), faultThreshold(0), isFaulted(false) {}

/**
 * @brief Processes the event locally and then forwards it to the configured handler.
 * @param event Event to handle.
 */
void Sensor::on(Event event) {
    // Execute sensor-specific reading and validation logic
    processEvent(event);

    // Suppress propagation if the event was marked for suppression
    if (event.identifier == -1)
        return;

    // Push event upstream to the registered mediator device
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

/**
 * @brief Sets the number of consecutive failures required before emitting a fault event.
 * @param count Consecutive failure threshold (default: 0 = disabled for backward compatibility).
 */
void Sensor::setFaultThreshold(uint8_t count) {
    faultThreshold = count;
}

/**
 * @brief Reports a successful read, potentially triggering a recovery event if previously faulted.
 * If the sensor was in a faulted state, emits SENSOR_RECOVERED_EVENT_IDENTIFIER (23) once
 * and clears the fault state. Resets the consecutive failure counter.
 * @param event The event to potentially modify.
 */
void Sensor::reportReadSuccess(Event& event) {
    if (isFaulted) {
        // Emit recovery event once, then clear fault state
        event.identifier = SENSOR_RECOVERED_EVENT_IDENTIFIER;
        event.sourceId = pin;
        isFaulted = false;
    }
    consecutiveFailures = 0;
}

/**
 * @brief Reports a failed read, potentially triggering a fault event after threshold consecutive failures.
 * If fault detection is enabled (threshold > 0) and consecutive failures reach the threshold,
 * emits SENSOR_FAULT_EVENT_IDENTIFIER (22) exactly once (latching state).
 * @param event The event to potentially modify.
 */
void Sensor::reportReadFailure(Event& event) {
    consecutiveFailures++;
    
    // Only emit fault event if threshold is configured and not already faulted
    if (faultThreshold > 0 && !isFaulted && consecutiveFailures >= faultThreshold) {
        event.identifier = SENSOR_FAULT_EVENT_IDENTIFIER;
        event.sourceId = pin;
        isFaulted = true;
    }
}