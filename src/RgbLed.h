#ifndef RGB_LED_H
#define RGB_LED_H

/**
 * @file RgbLed.h
 * @brief Declares an RGB LED actuator with PWM color control and predefined colors.
 * @ingroup actuators_module
 *
 * Provides `RgbColor`, a lightweight color value type, and `RgbLed`, a command-driven
 * actuator that drives the red, green and blue channels of a common-cathode or
 * common-anode LED through PWM.
 * @date 2026-10-31
 * @version 1.0
 */

#include "Actuator.h"
#include <Arduino.h>

/**
 * @brief Red/green/blue color value with per-channel intensities (0-255).
 * @ingroup actuators_module
 *
 * Colors are logical: common-anode inversion is applied by `RgbLed`, so the same value
 * works for both LED types. Aggregate-initialize it as `RgbColor{r, g, b}`.
 *
 * @see RgbLed for the predefined color constants.
 */
struct RgbColor {
    uint8_t redIntensity;   ///< Red intensity (0-255).
    uint8_t greenIntensity; ///< Green intensity (0-255).
    uint8_t blueIntensity;  ///< Blue intensity (0-255).
};

/**
 * @brief PWM actuator that drives a three-channel (red, green, blue) LED.
 * @ingroup actuators_module
 *
 * Follows the framework's buffered-command pattern: `setColor()` only stores a color, and the
 * hardware changes when a command is handled.
 *
 * | Command | Effect |
 * |---|---|
 * | `TURN_ON_COMMAND` | Lights the LED with the last applied color (white by default). |
 * | `TURN_OFF_COMMAND` | Darkens the LED but remembers the last applied color. |
 * | `TOGGLE_LED_COMMAND` | Alternates between lit and dark. |
 * | `SET_COLOR_COMMAND` | Applies the buffered color and lights the LED. |
 *
 * Unknown commands are ignored by this actuator, then propagated to the parent handler like any other.
 *
 * @par Example
 * @code
 * RgbLed statusLed(25, 26, 27);          // red, green, blue PWM pins
 * statusLed.setColor(RgbLed::ORANGE);    // buffer a predefined color
 * statusLed.handle(RgbLed::SET_COLOR_COMMAND);
 * statusLed.handle(RgbLed::TOGGLE_LED_COMMAND);
 * @endcode
 *
 * @note The red channel pin is the pin registered with the base `Actuator`.
 * @note PWM is written with `analogWrite()`, so all three pins must be PWM-capable.
 * @see RgbColor
 */
class RgbLed : public Actuator {
private:
    int greenPin; ///< PWM pin connected to the green channel.
    int bluePin;  ///< PWM pin connected to the blue channel.
    bool commonAnode; ///< `true` when the LED shares a common anode (inverted drive).

    uint8_t bufferedRedIntensity;   ///< Buffered red intensity (0-255).
    uint8_t bufferedGreenIntensity; ///< Buffered green intensity (0-255).
    uint8_t bufferedBlueIntensity;  ///< Buffered blue intensity (0-255).

    uint8_t activeRedIntensity;   ///< Last applied red intensity (0-255).
    uint8_t activeGreenIntensity; ///< Last applied green intensity (0-255).
    uint8_t activeBlueIntensity;  ///< Last applied blue intensity (0-255).

    bool active; ///< Logical state: `true` when lit.

    /**
     * @brief Writes the logical state to the hardware.
     * @details Writes the active color when lit, or zero intensity on every channel when off.
     */
    void applyState() const;

    /**
     * @brief Writes one color channel, applying the common-anode inversion when needed.
     * @param gpioPin PWM pin of the channel.
     * @param intensity Logical intensity (0-255).
     */
    void writeColorChannelIntensity(int gpioPin, uint8_t intensity) const;

protected:
    /**
     * @brief Executes RGB LED commands before they are propagated upstream.
     * @param command Command to execute; unknown commands leave the outputs untouched.
     */
    void executeCommand(Command command) override;

public:
    /** @name Command identifiers (300-303)
     *  Reserved range for this actuator; unique among the framework's actuators.
     *  @{ */
    /** @brief Command identifier for turning the LED on with the last applied color. */
    static constexpr int TURN_ON_COMMAND_IDENTIFIER = 300;
    /** @brief Command identifier for turning the LED off. */
    static constexpr int TURN_OFF_COMMAND_IDENTIFIER = 301;
    /** @brief Command identifier for toggling the LED state. */
    static constexpr int TOGGLE_LED_COMMAND_IDENTIFIER = 302;
    /** @brief Command identifier for applying the buffered color. */
    static constexpr int SET_COLOR_COMMAND_IDENTIFIER = 303;
    /** @} */

    /** @name Prebuilt commands
     *  @{ */
    /** @brief Prebuilt command instance for turning the LED on. */
    static constexpr Command TURN_ON_COMMAND{TURN_ON_COMMAND_IDENTIFIER};
    /** @brief Prebuilt command instance for turning the LED off. */
    static constexpr Command TURN_OFF_COMMAND{TURN_OFF_COMMAND_IDENTIFIER};
    /** @brief Prebuilt command instance for toggling the LED. */
    static constexpr Command TOGGLE_LED_COMMAND{TOGGLE_LED_COMMAND_IDENTIFIER};
    /** @brief Prebuilt command instance for applying the buffered color. */
    static constexpr Command SET_COLOR_COMMAND{SET_COLOR_COMMAND_IDENTIFIER};
    /** @} */

    /** @name Predefined colors
     *  Approximate values; the perceived tint depends on the LED model.
     *  @{ */
    static constexpr RgbColor RED{255, 0, 0};       ///< Pure red.
    static constexpr RgbColor GREEN{0, 255, 0};     ///< Pure green.
    static constexpr RgbColor BLUE{0, 0, 255};      ///< Pure blue.
    static constexpr RgbColor YELLOW{255, 255, 0};  ///< Red + green.
    static constexpr RgbColor CYAN{0, 255, 255};    ///< Green + blue.
    static constexpr RgbColor MAGENTA{255, 0, 255}; ///< Red + blue.
    static constexpr RgbColor WHITE{255, 255, 255}; ///< All channels at full intensity.
    static constexpr RgbColor ORANGE{255, 165, 0};  ///< Approximate orange.
    static constexpr RgbColor PURPLE{128, 0, 128};  ///< Approximate purple.
    /** @} */

    /**
     * @brief Constructs an RGB LED actuator. The LED starts off.
     * @details Configures the three pins as outputs and writes the dark state. The color
     *          applied by the first `TURN_ON_COMMAND` is white until `SET_COLOR_COMMAND` is handled.
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
     * @details Does not change the outputs; the LED is unaffected until the command is handled.
     * @param redIntensity Red intensity (0-255).
     * @param greenIntensity Green intensity (0-255).
     * @param blueIntensity Blue intensity (0-255).
     */
    void setColor(uint8_t redIntensity, uint8_t greenIntensity, uint8_t blueIntensity);

    /**
     * @brief Buffers a color applied by the next `SET_COLOR_COMMAND`.
     * @details Convenience overload for `RgbColor` values; does not change the outputs.
     * @param color Color to buffer, typically a predefined constant such as `RgbLed::ORANGE`.
     */
    void setColor(const RgbColor& color);

    /**
     * @brief Reports the current logical LED state.
     * @return `true` if lit, otherwise `false`.
     */
    [[nodiscard]] bool isActive() const;

    /**
     * @brief Returns the last applied red intensity.
     * @return Intensity (0-255) applied by the last `SET_COLOR_COMMAND`; unchanged by on/off/toggle.
     */
    [[nodiscard]] uint8_t getRedIntensity() const;
    /**
     * @brief Returns the last applied green intensity.
     * @return Intensity (0-255) applied by the last `SET_COLOR_COMMAND`; unchanged by on/off/toggle.
     */
    [[nodiscard]] uint8_t getGreenIntensity() const;
    /**
     * @brief Returns the last applied blue intensity.
     * @return Intensity (0-255) applied by the last `SET_COLOR_COMMAND`; unchanged by on/off/toggle.
     */
    [[nodiscard]] uint8_t getBlueIntensity() const;
};

#endif // RGB_LED_H
