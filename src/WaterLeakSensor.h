#ifndef WATER_LEAK_SENSOR_H
#define WATER_LEAK_SENSOR_H

/**
 * @file WaterLeakSensor.h
 * @brief Declares a water leak detection sensor.
 * @ingroup sensors_module
 */

#include "Sensor.h"

/**
 * @brief Water leak sensor that detects water presence via digital input.
 *
 * Compatible with common water leak sensor modules such as the YL-83 or
 * FR-101, which use a conductivity probe to detect water between exposed
 * metal contacts. These modules output a digital signal: HIGH when dry
 * (pull-up), LOW when water bridges the contacts.
 *
 * Uses interrupt-driven operation (CHANGE edge) for zero-latency leak detection.
 * The sensor connects between GPIO and GND; when water bridges the contacts,
 * the pin reads LOW. The GPIO interrupt fires on state transitions, and
 * processEvent() distinguishes between leak detected and leak cleared.
 *
 * Register via `appendSensorToTriggeringScheduler()` for interrupt-driven operation.
 * @see Sensor
 * @ingroup sensors_module
 */
class WaterLeakSensor : public Sensor {
protected:
    bool lastState; ///< The last known state of the sensor (true = leak detected).

protected:
    /**
     * @brief Detects water leak transitions from the GPIO interrupt trigger.
     *
     * Called from the sampler task via processTriggeredSensors() after the GPIO
     * interrupt fires. Compares the current pin state with the last known state
     * and emits either WATER_LEAK_DETECTED or WATER_LEAK_CLEARED events.
     * @param event Mutable event request/response payload.
     * @runs_on sampler
     */
    void processEvent(Event& event) override;

    [[nodiscard]] uint32_t getDefaultSamplingIntervalInMilliseconds() const override;
    [[nodiscard]] uint32_t getMinimalSamplingIntervalInMilliseconds() const override;

public:
    /** @brief Identifier emitted when a water leak is detected. */
    static constexpr int WATER_LEAK_DETECTED_EVENT_IDENTIFIER = 70;
    /** @brief Identifier emitted when a water leak is cleared. */
    static constexpr int WATER_LEAK_CLEARED_EVENT_IDENTIFIER = 71;
    /** @brief Prebuilt event instance for leak-detected notifications. */
    static const Event WATER_LEAK_DETECTED_EVENT;
    /** @brief Prebuilt event instance for leak-cleared notifications. */
    static const Event WATER_LEAK_CLEARED_EVENT;
    static constexpr int PROPAGATION_SUPPRESSION_IDENTIFIER = -1; ///< Identifier for propagation suppression.

    /// Default sampling interval in milliseconds (used only for hybrid registration).
    static constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS = 100;
    /// Minimum sampling interval in milliseconds (used only for hybrid registration).
    static constexpr uint32_t MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 20;

    /**
     * @brief Constructs a water leak sensor with an optional propagation target.
     * @param signalGpioPin Input GPIO pin connected to the water leak sensor.
     * @param parentHandler Optional upstream handler for propagated events.
     * @note Configures the pin as INPUT_PULLUP. Register via
     * appendSensorToTriggeringScheduler() for interrupt-driven operation.
     */
    explicit WaterLeakSensor(int signalGpioPin, EventHandler* parentHandler = nullptr);
    ~WaterLeakSensor() override = default;

    /**
     * @brief Reports whether a water leak is currently detected.
     * @return @c true if the last reading indicated a leak.
     */
    [[nodiscard]] bool isWaterLeakDetected() const;
};

#endif // WATER_LEAK_SENSOR_H
