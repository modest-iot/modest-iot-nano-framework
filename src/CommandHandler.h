#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

/**
 * @file CommandHandler.h
 * @brief Defines the CommandHandler interface.
 * @ingroup core_module
 * @copyright (c) 2025-2026 Angel Velasquez
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
#include <cstdint>

/** @brief Driver-level outcome of executing a command. */
enum class CommandExecutionStatus : uint8_t {
    Succeeded, ///< The backend confirmed accepting the command output.
    Failed, ///< The backend reported that the command output was not accepted.
    Unknown ///< The handler cannot confirm whether the backend accepted the output.
};

/** @brief Result returned by the additive checked command API. */
struct CommandExecutionResult {
    CommandExecutionStatus status = CommandExecutionStatus::Unknown;

    /** @brief Creates a confirmed driver-level success result. */
    [[nodiscard]] static constexpr CommandExecutionResult succeeded() {
        return {CommandExecutionStatus::Succeeded};
    }

    /** @brief Creates a confirmed driver-level failure result. */
    [[nodiscard]] static constexpr CommandExecutionResult failed() {
        return {CommandExecutionStatus::Failed};
    }

    /** @brief Creates an unreported/unknown result. */
    [[nodiscard]] static constexpr CommandExecutionResult unknown() {
        return {CommandExecutionStatus::Unknown};
    }
};

/**
 * @brief Abstract interface for handling commands.
 * @ingroup core_module
 *
 * Implement this interface in classes that need to execute commands. The `handle` method is called
 * when a command is issued, allowing for custom execution logic. Existing implementations remain
 * compatible with `handleChecked()` and report `Unknown` unless they provide driver-level evidence.
 */
class CommandHandler {
public:
    /**
     * @brief Processes an incoming command.
     * @param command Command instance to execute.
     */
    virtual void handle(Command command) = 0;

    /**
     * @brief Executes a command through the legacy handler and reports its outcome when available.
     * @param command Command instance to execute.
     * @return `Unknown` unless the implementation overrides this method with driver evidence.
     */
    virtual CommandExecutionResult handleChecked(const Command command) {
        handle(command);
        return CommandExecutionResult::unknown();
    }

    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     */
    virtual ~CommandHandler() = default;
};

#endif // COMMAND_HANDLER_H