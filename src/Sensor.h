#ifndef SENSOR_H
#define SENSOR_H

/**
 * @file Sensor.h
 * @brief Declares the Sensor base class.
 * 
 * This abstract base class represents input devices in the Modest IoT Nano-framework, providing
 * a foundation for sensors that generate events. It includes event propagation to an optional
 * handler, supporting the framework’s event-driven design.
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

#include "EventHandler.h"

class Sensor : public EventHandler {
protected:
    int pin; ///< GPIO pin assigned to the sensor.
    EventHandler* eventHandler; ///< Optional handler to receive propagated events.
    virtual void processEvent(Event& event) = 0;

public:
    /**
     * @brief Constructs a Sensor with a pin and optional event handler.
     * @param signalGpioPin The GPIO pin for the sensor.
     * @param parentHandler Pointer to an EventHandler to receive events (default: nullptr).
     */
    Sensor(int signalGpioPin, EventHandler* parentHandler = nullptr);
    virtual ~Sensor() = default;

    /**
     * @brief Handles an event by propagating it to the assigned handler.
     * @param event The event to handle.
     */
    void on(Event event) override;

    /**
     * @brief Sets or updates the event handler for this sensor.
     * @param parentHandler Pointer to the new EventHandler.
     */
    void setHandler(EventHandler* parentHandler);
};

#endif // SENSOR_H