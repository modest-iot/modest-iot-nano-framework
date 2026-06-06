#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

/**
 * @file EventHandler.h
 * @brief Defines the EventHandler interface.
 * @ingroup core_module
 *
 * This file provides the foundation for event-driven behavior in the Modest IoT Nano-framework.
 * `EventHandler` is an abstract interface for classes that respond to `Event` objects,
 * enabling asynchronous reactivity in IoT devices.
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

#include "Event.h"
/**
 * @name Core: Base Contracts & Interfaces
 * @ingroup core_module
 * @{
 */

/**
 * @brief Abstract interface for handling events.
 * @ingroup core_module
 *
 * Implement this interface in classes that need to react to events. The `on` method is called
 * when an event occurs, allowing for custom handling logic.
 */
class EventHandler {
public:
    /**
     * @brief Handles an incoming event.
     * @param event Event instance to process.
     */
    virtual void on(Event event) = 0;

    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     */
    virtual ~EventHandler() = default;
};

        /** @} */ // End of Core: Base Contracts & Interfaces

#endif // EVENT_HANDLER_H