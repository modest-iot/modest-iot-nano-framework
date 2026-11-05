#ifndef BUTTON_H
#define BUTTON_H

/**
 * @file Button.h
 * @brief Declares a debounced digital button sensor.
 * @ingroup sensors_module
 * @date 2026-06-06
 * @version 1.0
 */

#include "Sensor.h"

/**
 * @name Sensors: Digital & Debounced Input
 * @ingroup sensors_module
 * @{
 */

/**
 * @brief Debounced button sensor that emits a press event on stable transitions.
 * @ingroup sensors_module
 */
class Button : public Sensor {
private:
    unsigned long lastStableTimestamp; ///< Timestamp of the last stable input transition.
    const unsigned long debounceDelayInMilliseconds; ///< Configured debounce delay in milliseconds.
    bool previousState; ///< The previous state of the input pin for debouncing logic.

protected:
    /**
     * @brief Samples and debounce input transitions for press detection.
     * @param event Mutable event request/response payload.
     */
    void processEvent(Event& event) override;

    [[nodiscard]] uint32_t getDefaultSamplingIntervalInMilliseconds() const override;
    [[nodiscard]] uint32_t getMinimalSamplingIntervalInMilliseconds() const override;

public:
    /** @brief Identifier emitted when a valid button press is detected. */
    static const int BUTTON_PRESSED_EVENT_IDENTIFIER = 10;
    /** @brief Prebuilt event instance for button press notifications. */
    static const Event BUTTON_PRESSED_EVENT;

    /// Default sampling interval in milliseconds.
    /// @note Button debounce typically needs 20-50 ms polling for responsive feel.
    static constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS = 50;
    /// Minimum sampling interval in milliseconds.
    /// @note Below 10 ms the debounce logic cannot settle reliably.
    static constexpr uint32_t MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 10;

    /**
     * @brief Constructs a button sensor with an optional propagation target.
     * @param signalGpioPin Input GPIO pin connected to the button.
     * @param debounceDelayInMilliseconds Minimum stable interval before accepting a transition.
     * @param parentHandler Optional upstream handler for propagated events.
     */
    Button(int signalGpioPin, unsigned long debounceDelayInMilliseconds = 50, EventHandler* parentHandler = nullptr);
    ~Button() override = default;
};

/** @} */ // End of Sensors: Digital & Debounced Input

#endif // BUTTON_H