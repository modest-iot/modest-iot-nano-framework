#ifndef DOOR_WINDOW_SENSOR_H
#define DOOR_WINDOW_SENSOR_H

/**
 * @file DoorWindowSensor.h
 * @brief Declares a door/window open/close detection sensor.
 * @ingroup sensors_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @license Apache-2.0
 */

#include "Sensor.h"

/**
 * @brief Door/window sensor that detects open/close transitions using a
 *        magnetic reed switch.
 *
 * Compatible with common magnetic contact sensors such as the A313 or
 * MY-103LY reed switch modules, which use a normally-closed reed switch
 * that opens when a magnet moves away (door/window opens). These sensors
 * output a digital signal: LOW when closed (magnet present), HIGH when
 * open (magnet absent).
 *
 * Uses interrupt-driven operation (CHANGE edge) for zero-latency detection.
 * The sensor connects between GPIO and VCC (normally closed reed switch).
 * When the door/window opens, the pin reads HIGH. The GPIO interrupt fires
 * on state transitions, and processEvent() distinguishes between door opened
 * and door closed.
 *
 * Register via `appendSensorToTriggeringScheduler()` for interrupt-driven operation.
 * @see Sensor
 * @ingroup sensors_module
 */
class DoorWindowSensor : public Sensor {
protected:
    bool lastState; ///< The last known state of the sensor (true = door open).

protected:
    /**
     * @brief Detects door/window transitions from the GPIO interrupt trigger.
     *
     * Called from the sampler task via processTriggeredSensors() after the GPIO
     * interrupt fires. Compares the current pin state with the last known state
     * and emits either DOOR_OPENED or DOOR_CLOSED events.
     * @param event Mutable event request/response payload.
     * @runs_on{sampler}
     */
    void processEvent(Event& event) override;

    [[nodiscard]] uint32_t getDefaultSamplingIntervalInMilliseconds() const override;
    [[nodiscard]] uint32_t getMinimalSamplingIntervalInMilliseconds() const override;

public:
    /** @brief Identifier emitted when the door/window is opened. */
    static constexpr int DOOR_OPENED_EVENT_IDENTIFIER = 72;
    /** @brief Identifier emitted when the door/window is closed. */
    static constexpr int DOOR_CLOSED_EVENT_IDENTIFIER = 73;
    /** @brief Prebuilt event instance for door-opened notifications. */
    static const Event DOOR_OPENED_EVENT;
    /** @brief Prebuilt event instance for door-closed notifications. */
    static const Event DOOR_CLOSED_EVENT;
    static constexpr int PROPAGATION_SUPPRESSION_IDENTIFIER = -1; ///< Identifier for propagation suppression.

    /// Default sampling interval in milliseconds (used only for hybrid registration).
    static constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS = 100;
    /// Minimum sampling interval in milliseconds (used only for hybrid registration).
    static constexpr uint32_t MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 20;

    /**
     * @brief Constructs a door/window sensor with an optional propagation target.
     * @param signalGpioPin Input GPIO pin connected to the reed switch.
     * @param parentHandler Optional upstream handler for propagated events.
     * @note Configures the pin as INPUT. Register via
     * appendSensorToTriggeringScheduler() for interrupt-driven operation.
     */
    explicit DoorWindowSensor(int signalGpioPin, EventHandler* parentHandler = nullptr);
    ~DoorWindowSensor() override = default;

    /**
     * @brief Reports whether the door/window is currently open.
     * @return @c true if the last reading indicated the door is open.
     */
    [[nodiscard]] bool isDoorOpen() const;
};

#endif // DOOR_WINDOW_SENSOR_H
