#ifndef SENSOR_H
#define SENSOR_H

/**
 * @file Sensor.h
 * @brief Declares the Sensor base class.
 * @ingroup sensors_module
 *
 * This abstract base class represents input devices in the Modest IoT Nano-framework, providing
 * a foundation for sensors that generate events. It includes event propagation to an optional
 * handler, supporting the framework’s event-driven design.
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

#include "EventHandler.h"
#include "Event.h"

/**
 * @name Sensors: Base Abstraction
 * @ingroup sensors_module
 * @{
 */

class Sensor : public EventHandler {
protected:
    int pin; ///< GPIO pin assigned to the sensor.
    EventHandler* eventHandler; ///< Optional handler to receive propagated events.

    /**
     * @brief Template-method hook that lets subclasses transform or suppress an event before propagation.
     * @param event Event being processed.
     */
    virtual void processEvent(Event& event) = 0;

public:
    /// --- FRAMEWORK-WIDE UNIFIED SENSOR SPECIFICATION EVENTS ---
    static constexpr int DATA_READ_EVENT_IDENTIFIER = 20;
    static constexpr int MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER = 21;

    static constexpr Event DATA_READ_EVENT = Event(DATA_READ_EVENT_IDENTIFIER);
    static constexpr Event MEASURE_DATA_REQUESTED_EVENT = Event(MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    /**
     * @brief Constructs a Sensor with a pin and optional event handler.
     * @param signalGpioPin The GPIO pin for the sensor.
     * @param parentHandler Pointer to an EventHandler to receive events (default: nullptr).
     */
    Sensor(int signalGpioPin, EventHandler* parentHandler = nullptr);
    ~Sensor() override = default;

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

/** @} */ // End of Sensors: Base Abstraction

#endif // SENSOR_H