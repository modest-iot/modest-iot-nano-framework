#ifndef BUTTON_H
#define BUTTON_H

/**
 * @file Button.h
 * @brief Declares a debounced digital button sensor.
 * @ingroup sensors_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-06-06
 * @version 1.0
 */

#include "Sensor.h"

/**
 * @brief Debounced button sensor that emits a press event on stable transitions.
 *
 * Uses interrupt-driven operation (FALLING edge) for zero-latency press detection.
 * Debouncing is performed in the sampler task context via processEvent(), which is
 * called from processTriggeredSensors() when the GPIO interrupt fires.
 * @ingroup sensors_module
 */
class Button : public Sensor {
private:
    unsigned long lastStableTimestamp; ///< Timestamp of the last stable input transition.
    const unsigned long debounceDelayInMilliseconds; ///< Configured debounce delay in milliseconds.
    bool previousState; ///< The previous state of the input pin for debouncing logic.

protected:
    /**
     * @brief Debounces and emits a press event when a stable FALLING edge is detected.
     *
     * Called from the sampler task via processTriggeredSensors() after the GPIO
     * interrupt fires. Reads the current pin state and applies the debounce window.
     * @param event Mutable event request/response payload.
     * @runs_on{sampler}
     */
    void processEvent(Event& event) override;

    [[nodiscard]] uint32_t getDefaultSamplingIntervalInMilliseconds() const override;
    [[nodiscard]] uint32_t getMinimalSamplingIntervalInMilliseconds() const override;

public:
    /** @brief Identifier emitted when a valid button press is detected. */
    static constexpr int BUTTON_PRESSED_EVENT_IDENTIFIER = 10;
    /** @brief Prebuilt event instance for button press notifications. */
    static const Event BUTTON_PRESSED_EVENT;

    /// Default sampling interval in milliseconds (used only for hybrid registration).
    /// @note In interrupt mode, this value is not used for polling.
    static constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS = 50;
    /// Minimum sampling interval in milliseconds (used only for hybrid registration).
    static constexpr uint32_t MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 10;

    /**
     * @brief Constructs a button sensor with an optional propagation target.
     * @param signalGpioPin Input GPIO pin connected to the button.
     * @param debounceDelayInMilliseconds Minimum stable interval before accepting a transition.
     * @param parentHandler Optional upstream handler for propagated events.
     * @note Configures the pin as INPUT_PULLUP. Register via
     * appendSensorToTriggeringScheduler() for interrupt-driven operation.
     */
    explicit Button(int signalGpioPin, unsigned long debounceDelayInMilliseconds = 50, EventHandler* parentHandler = nullptr);
    ~Button() override = default;
};

#endif // BUTTON_H