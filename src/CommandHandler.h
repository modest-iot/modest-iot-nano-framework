#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

/**
 * @file CommandHandler.h
 * @brief Defines the CommandHandler interface.
 * @ingroup core_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @license Apache-2.0
 *
 * This file establishes the foundation for command-driven behavior in the Modest IoT Nano-framework.
 * `CommandHandler` is an abstract interface for classes that process `Command` objects,
 * supporting a CQRS-inspired design.
 *
 * @author Angel Velasquez
 * @date 2026-06-06
 * @version 1.0
 */

#include "Command.h"

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

#endif // COMMAND_HANDLER_H