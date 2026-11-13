#ifndef ACTUATOR_H
#define ACTUATOR_H

/**
 * @file Actuator.h
 * @brief Declares the Actuator base class.
 * @ingroup actuators_module
 * @copyright (c) 2025-2026 Angel Velasquez
 *
 * This abstract base class represents output devices in the Modest IoT Nano-framework, providing
 * a foundation for actuators that respond to commands. It includes command propagation to an
 * optional handler, supporting the framework’s CQRS-inspired design.
 *
 * @author Angel Velasquez
 * @date 2026-06-06
 * @version 1.0
 */

#include "CommandHandler.h"

class Actuator : public CommandHandler {
protected:
    int pin; ///< GPIO pin assigned to the actuator.
    CommandHandler* commandHandler; ///< Optional handler to receive propagated commands.

    /**
     * @brief Template-method hook that lets subclasses execute or suppress a command before propagation.
     * @param command Command being processed.
     * @runs_on{sampler}
     */
    virtual void executeCommand(Command command) = 0;
public:
    /**
     * @brief Constructs an Actuator with a pin and optional command handler.
     * @param signalGpioPin The GPIO pin for the actuator.
     * @param parentHandler Pointer to a CommandHandler to receive commands (default: nullptr).
     */
    explicit Actuator(int signalGpioPin, CommandHandler* parentHandler = nullptr);

    /**
     * @brief Destructs the Actuator.
     */
    ~Actuator() override = default;

    /**
     * @brief Handles a command by propagating it to the assigned handler.
     * @param command The command to handle.
     * @runs_on{sampler}
     */
    void handle(Command command) final;

    /**
     * @brief Sets or updates the command handler for this actuator.
     * @param parentHandler Pointer to the new CommandHandler.
     */
    void setHandler(CommandHandler* parentHandler);
};

#endif // ACTUATOR_H