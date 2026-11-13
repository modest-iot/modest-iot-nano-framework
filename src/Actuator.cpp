/**
 * @file Actuator.cpp
 * @brief Implements the Actuator base class.
 * @copyright (c) 2025-2026 Angel Velasquez
 *
 * Provides core functionality for actuators in the Modest IoT Nano-framework, including command
 * propagation to an assigned handler. Subclasses should configure hardware and define specific
 * command execution logic.
 *
 * @author Angel Velasquez
 * @date 2026-06-06
 * @version 1.0
 */

#include "Actuator.h"

/**
 * @brief Stores the target GPIO pin and optional command handler for this actuator.
 */
Actuator::Actuator(int signalGpioPin, CommandHandler* parentHandler)
    : pin(signalGpioPin), commandHandler(parentHandler) {}

/**
 * @brief Executes the command locally and then forwards it to the configured handler.
 */
void Actuator::handle(const Command command) {
    // Execute hardware-specific command handling
    executeCommand(command);

    // Propagate the command to the parent handler for monitoring
    if (commandHandler != nullptr) {
        commandHandler->handle(command);
    }}

/**
 * @brief Replaces the current propagation target for this actuator.
 */
void Actuator::setHandler(CommandHandler* parentHandler) {
    commandHandler = parentHandler;
}