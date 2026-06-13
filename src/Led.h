#ifndef LED_H
#define LED_H

/**
 * @file Led.h
 * @brief Declares an LED actuator with on/off/toggle command support.
 * @ingroup actuators_module
 * @date 2026-06-06
 * @version 1.0
 */

#include "Actuator.h"

/**
 * @name Actuators: Simple Digital Output
 * @ingroup actuators_module
 * @{
 */

/**
 * @brief Digital output actuator that controls an LED state.
 * @ingroup actuators_module
 */
class Led : public Actuator {
private:
    bool active; ///< Current state of the LED.

protected:
    /**
     * @brief Executes LED state commands and optionally propagates them upstream.
     * @param command Command to execute.
     */
    void executeCommand(Command command) override;

public:
    /** @brief Command identifier for turning the LED on. */
    static constexpr int TURN_ON_COMMAND_IDENTIFIER = 100;
    /** @brief Command identifier for turning the LED off. */
    static constexpr int TURN_OFF_COMMAND_IDENTIFIER = 101;
    /** @brief Command identifier for toggling the LED state. */
    static constexpr int TOGGLE_LED_COMMAND_IDENTIFIER = 102;

    /** @brief Prebuilt command instance for turning the LED on. */
    static constexpr Command TURN_ON_COMMAND{TURN_ON_COMMAND_IDENTIFIER};
    /** @brief Prebuilt command instance for turning the LED off. */
    static constexpr Command TURN_OFF_COMMAND{TURN_OFF_COMMAND_IDENTIFIER};
    /** @brief Prebuilt command instance for toggling LED state. */
    static constexpr Command TOGGLE_LED_COMMAND{TOGGLE_LED_COMMAND_IDENTIFIER};

    /**
     * @brief Constructs a LED actuator.
     * @param signalGpioPin Output GPIO pin connected to the LED.
     * @param initialState Initial power state applied to the output pin.
     * @param parentHandler Optional upstream handler for propagated commands.
     */
    Led(int signalGpioPin, bool initialState = false, CommandHandler* parentHandler = nullptr);

    /**
     * @brief Reports the current logical LED state.
     * @return `true` if the LED is on, otherwise `false`.
     */
    [[nodiscard]] bool isActive() const;
};

/** @} */ // End of Actuators: Simple Digital Output

#endif // LED_H
