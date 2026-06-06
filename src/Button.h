#ifndef BUTTON_H
#define BUTTON_H

/**
 * @file Button.h
 * @brief Declares a debounced digital button sensor.
 */

#include "Sensor.h"

/**
 * @brief Debounced button sensor that emits a press event on stable transitions.
 */
class Button : public Sensor {
private:
    unsigned long lastStableTimestamp;
    const unsigned long debounceDelayInMilliseconds;
    bool previousState;

protected:
    /**
     * @brief Samples and debounces input transitions for press detection.
     * @param event Mutable event request/response payload.
     */
    void processEvent(Event& event) override;

public:
    /** @brief Identifier emitted when a valid button press is detected. */
    static const int BUTTON_PRESSED_EVENT_IDENTIFIER = 10;
    /** @brief Prebuilt event instance for button press notifications. */
    static const Event BUTTON_PRESSED_EVENT;

    /**
     * @brief Constructs a button sensor with an optional propagation target.
     * @param signalGpioPin Input GPIO pin connected to the button.
     * @param debounceDelayInMilliseconds Minimum stable interval before accepting a transition.
     * @param parentHandler Optional upstream handler for propagated events.
     */
    Button(int signalGpioPin, unsigned long debounceDelayInMilliseconds = 50, EventHandler* parentHandler = nullptr);
    virtual ~Button() override = default;
};

#endif // BUTTON_H