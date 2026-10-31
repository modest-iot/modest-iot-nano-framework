#ifndef RGB_LED_H
#define RGB_LED_H

/**
 * @file RgbLed.h
 * @brief Declares an RGB LED actuator with PWM color control.
 * @ingroup actuators_module
 * @date 2026-10-31
 * @version 1.0
 */

#include "Actuator.h"
#include <Arduino.h>

/**
 * @name Actuators: Color Output
 * @ingroup actuators_module
 * @{
 */

/**
 * @brief PWM actuator that drives a three-channel (red, green, blue) LED.
 *
 * The color is buffered through `setColor()` and applied when `SET_COLOR_COMMAND` is handled.
 * `TURN_ON_COMMAND` lights the LED with the last applied color (white by default),
 * `TURN_OFF_COMMAND` darkens it while remembering that color, and `TOGGLE_COMMAND` alternates both.
 * Common-anode LEDs are supported by inverting the PWM duty cycle.
 *
 * @note The red channel pin is the pin registered with the base `Actuator`.
 */
class RgbLed : public Actuator {
private:
    int greenPin; ///< PWM pin connected to the green channel.
    int bluePin;  ///< PWM pin connected to the blue channel.
    bool commonAnode; ///< `true` when the LED shares a common anode (inverted drive).

    uint8_t bufferedRed;   ///< Buffered red intensity (0-255).
    uint8_t bufferedGreen; ///< Buffered green intensity (0-255).
    uint8_t bufferedBlue;  ///< Buffered blue intensity (0-255).

    uint8_t activeRed;   ///< Last applied red intensity (0-255).
    uint8_t activeGreen; ///< Last applied green intensity (0-255).
    uint8_t activeBlue;  ///< Last applied blue intensity (0-255).

    bool active; ///< Logical state: `true` when lit.

    /** @brief Writes the active color (or darkness) to the three PWM channels. */
    void applyState() const;

    /** @brief Writes one channel, applying the common-anode inversion when needed. */
    void writeColorChannel(int gpioPin, uint8_t intensity) const;

protected:
    /**
     * @brief Executes RGB LED commands and optionally propagates them upstream.
     * @param command Command to execute.
     */
    void executeCommand(Command command) override;

public:
    /** @brief Command identifier for turning the LED on with the last applied color. */
    static constexpr int TURN_ON_COMMAND_IDENTIFIER = 300;
    /** @brief Command identifier for turning the LED off. */
    static constexpr int TURN_OFF_COMMAND_IDENTIFIER = 301;
    /** @brief Command identifier for toggling the LED state. */
    static constexpr int TOGGLE_COMMAND_IDENTIFIER = 302;
    /** @brief Command identifier for applying the buffered color. */
    static constexpr int SET_COLOR_COMMAND_IDENTIFIER = 303;

    /** @brief Prebuilt command instance for turning the LED on. */
    static constexpr Command TURN_ON_COMMAND{TURN_ON_COMMAND_IDENTIFIER};
    /** @brief Prebuilt command instance for turning the LED off. */
    static constexpr Command TURN_OFF_COMMAND{TURN_OFF_COMMAND_IDENTIFIER};
    /** @brief Prebuilt command instance for toggling the LED. */
    static constexpr Command TOGGLE_COMMAND{TOGGLE_COMMAND_IDENTIFIER};
    /** @brief Prebuilt command instance for applying the buffered color. */
    static constexpr Command SET_COLOR_COMMAND{SET_COLOR_COMMAND_IDENTIFIER};

    /**
     * @brief Constructs an RGB LED actuator. The LED starts off.
     * @param redGpioPin PWM-capable output pin for the red channel.
     * @param greenGpioPin PWM-capable output pin for the green channel.
     * @param blueGpioPin PWM-capable output pin for the blue channel.
     * @param isCommonAnode `true` for common-anode LEDs (inverted drive); default `false`.
     * @param parentHandler Optional upstream handler for propagated commands.
     */
    RgbLed(int redGpioPin, int greenGpioPin, int blueGpioPin,
           bool isCommonAnode = false, CommandHandler* parentHandler = nullptr);

    /**
     * @brief Buffers the color applied by the next `SET_COLOR_COMMAND`.
     * @param red Red intensity (0-255).
     * @param green Green intensity (0-255).
     * @param blue Blue intensity (0-255).
     */
    void setColor(uint8_t red, uint8_t green, uint8_t blue);

    /**
     * @brief Reports whether the LED is currently lit.
     * @return `true` if lit, otherwise `false`.
     */
    [[nodiscard]] bool isActive() const;

    /** @brief Returns the last applied red intensity. */
    [[nodiscard]] uint8_t getRedIntensity() const;
    /** @brief Returns the last applied green intensity. */
    [[nodiscard]] uint8_t getGreenIntensity() const;
    /** @brief Returns the last applied blue intensity. */
    [[nodiscard]] uint8_t getBlueIntensity() const;
};

/** @} */ // End of Actuators: Color Output

#endif // RGB_LED_H
