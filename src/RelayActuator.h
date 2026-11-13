#ifndef RELAY_ACTUATOR_H
#define RELAY_ACTUATOR_H

/**
 * @file RelayActuator.h
 * @brief Declares a relay actuator with on/off/toggle command support.
 * @ingroup actuators_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-09
 * @version 1.0
 */

#include "Actuator.h"

/**
 * @brief Digital output actuator that controls a relay for AC/DC load switching.
 * @ingroup actuators_module
 * 
 * Controls a relay module via a single GPIO pin. Common use cases include
 * smart home load control, greenhouse pump automation, and industrial relay
 * switching. Uses only `digitalWrite()` — no external libraries required.
 * 
 * @runs_on{sampler}_task
 */
class RelayActuator : public Actuator {
private:
    bool active; ///< Current state of the relay.

protected:
    /**
     * @brief Executes relay state commands and optionally propagates them upstream.
     * @param command Command to execute.
     */
    void executeCommand(Command command) override;

public:
    /** @brief Command identifier for turning the relay on. */
    static constexpr int RELAY_ON_COMMAND_IDENTIFIER = 300;
    /** @brief Command identifier for turning the relay off. */
    static constexpr int RELAY_OFF_COMMAND_IDENTIFIER = 301;
    /** @brief Command identifier for toggling the relay state. */
    static constexpr int TOGGLE_RELAY_COMMAND_IDENTIFIER = 302;

    /** @brief Prebuilt command instance for turning the relay on. */
    static constexpr Command RELAY_ON_COMMAND{RELAY_ON_COMMAND_IDENTIFIER};
    /** @brief Prebuilt command instance for turning the relay off. */
    static constexpr Command RELAY_OFF_COMMAND{RELAY_OFF_COMMAND_IDENTIFIER};
    /** @brief Prebuilt command instance for toggling relay state. */
    static constexpr Command TOGGLE_RELAY_COMMAND{TOGGLE_RELAY_COMMAND_IDENTIFIER};

    /**
     * @brief Constructs a relay actuator.
     * @param signalGpioPin Output GPIO pin connected to the relay module.
     * @param initialState Initial power state applied to the output pin.
     * @param parentHandler Optional upstream handler for propagated commands.
     */
    explicit RelayActuator(int signalGpioPin, bool initialState = false, CommandHandler* parentHandler = nullptr);

    /**
     * @brief Reports the current logical relay state.
     * @return `true` if the relay is on, otherwise `false`.
     */
    [[nodiscard]] bool isActive() const;
};

#endif // RELAY_ACTUATOR_H
