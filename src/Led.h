#ifndef LED_H
#define LED_H

/**
 * @file Led.h
 * @brief Declares an LED actuator with on/off/toggle command support.
 */

#include "Actuator.h"

/**
 * @brief Digital output actuator that controls an LED state.
 */
class Led : public Actuator {
private:
    bool active;

protected:
    /**
     * @brief Executes LED state commands and optionally propagates them upstream.
     * @param command Command to execute.
     */
    void executeCommand(Command command) override;

public:
    /** @brief Command identifier for turning the LED on. */
    static const int TURN_ON_COMMAND_IDENTIFIER = 100;
    /** @brief Command identifier for turning the LED off. */
    static const int TURN_OFF_COMMAND_IDENTIFIER = 101;
    /** @brief Command identifier for toggling the LED state. */
    static const int TOGGLE_LED_COMMAND_IDENTIFIER = 102;

    /** @brief Prebuilt command instance for turning the LED on. */
    static const Command TURN_ON_COMMAND;
    /** @brief Prebuilt command instance for turning the LED off. */
    static const Command TURN_OFF_COMMAND;
    /** @brief Prebuilt command instance for toggling LED state. */
    static const Command TOGGLE_LED_COMMAND;

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
    bool isActive() const;
};

#endif // LED_H