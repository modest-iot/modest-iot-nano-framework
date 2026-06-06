#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

/**
 * @file CommandHandler.h
 * @brief Defines the CommandHandler interface.
 * @ingroup core_module
 *
 * This file establishes the foundation for command-driven behavior in the Modest IoT Nano-framework.
 * `CommandHandler` is an abstract interface for classes that process `Command` objects,
 * supporting a CQRS-inspired design.
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

#include "Command.h"
/**
 * @name Core: Base Contracts & Interfaces
 * @ingroup core_module
 * @{
 */

/**
 * @brief Abstract interface for handling commands.
 * @ingroup core_module
 *
 * Implement this interface in classes that need to execute commands. The `handle` method is called
 * when a command is issued, allowing for custom execution logic.
 */
class CommandHandler {
public:
    /**
     * @brief Processes an incoming command.
     * @param command Command instance to execute.
     */
    virtual void handle(Command command) = 0;

    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     */
    virtual ~CommandHandler() = default;
};

        /** @} */ // End of Core: Base Contracts & Interfaces

#endif // COMMAND_HANDLER_H