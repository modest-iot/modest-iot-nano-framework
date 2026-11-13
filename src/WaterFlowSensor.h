#ifndef WATER_FLOW_SENSOR_H
#define WATER_FLOW_SENSOR_H

/**
 * @file WaterFlowSensor.h
 * @brief Declares a water flow rate sensor (YF-S201) using hybrid scheduling.
 * @ingroup sensors_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-09
 * @version 1.0
 */

#include "Sensor.h"

/**
 * @brief Water flow sensor that measures liters per minute from pulse frequency.
 *
 * Uses hybrid scheduling: interrupt-driven pulse counting via CHANGE edge captures
 * every Hall effect pulse, while periodic sampling computes the flow rate from the
 * accumulated pulse count. Register via appendToBothSchedulers() for this behavior.
 * @ingroup sensors_module
 */
class WaterFlowSensor : public Sensor {
protected:
    volatile uint32_t pulseCount; ///< Interrupt-safe pulse counter (incremented in ISR, protected for testing).
    float cachedFlowRateInLitersPerMinute; ///< Last computed flow rate (protected for testing).
    float lastNotifiedFlowRateInLitersPerMinute; ///< Last flow rate that triggered a DATA_READ_EVENT (protected for testing).

    /// Event identifier used internally to prevent propagation when flow rate hasn't changed.
    static constexpr int PROPAGATION_SUPPRESSION_IDENTIFIER = -1;

    /// Initial value for cachedFlowRateInLitersPerMinute.
    static constexpr float INITIAL_CACHED_FLOW_RATE_IN_LITERS_PER_MINUTE = 0.0f;

    /// Initial value for lastNotifiedFlowRateInLitersPerMinute, ensuring the first reading always triggers an event.
    static constexpr float INITIAL_LAST_NOTIFIED_FLOW_RATE_IN_LITERS_PER_MINUTE = -1.0f;

private:
    uint32_t lastPulseCount; ///< Pulse count at the last sampling period.

    /// Pulses per milliliter for the YF-S201 sensor (7 pulses/ml).
    static constexpr float PULSES_PER_MILLILITER = 7.0f;

    /// Minimum change in liters per minute to trigger a DATA_READ_EVENT.
    static constexpr float FLOW_RATE_CHANGE_EPSILON = 0.01f;

    /// Conversion factor: seconds per minute.
    static constexpr float SECONDS_PER_MINUTE = 60.0f;

    /// Conversion factor: milliliters per liter.
    static constexpr float MILLILITERS_PER_LITER = 1000.0f;

    /// Conversion factor: milliseconds per second.
    static constexpr float MILLISECONDS_PER_SECOND = 1000.0f;

    /// Default sampling interval in milliseconds for flow rate computation.
    /// @note 1 second provides reasonable resolution for flow rate changes.
    static constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS = 1000;

    /// Minimum sampling interval in milliseconds.
    /// @note Below 200 ms the flow rate calculation becomes noisy.
    static constexpr uint32_t MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 200;

    /**
     * @brief Resets the pulse counter and related state.
     * Resets internal counters to zero for a fresh measurement session.
     */
    // ReSharper disable once CppHiddenFunction
    void resetPulseCount();

protected:
    /**
     * @brief Computes the flow rate from the accumulated pulse count.
     *
     * Called from the sampler task during periodic sampling. Reads the pulse count,
     * calculates the delta since the last sample, and converts to liters per minute.
     * @param event Mutable event request/response payload.
     * @runs_on{sampler}
     */
    void processEvent(Event& event) override;

    [[nodiscard]] uint32_t getDefaultSamplingIntervalInMilliseconds() const override;
    [[nodiscard]] uint32_t getMinimalSamplingIntervalInMilliseconds() const override;

public:
    /**
     * @brief Constructs a water flow sensor with an optional propagation target.
     * @param signalGpioPin Digital input GPIO pin connected to the YF-S201 signal output.
     * @param parentHandler Optional upstream handler for propagated events.
     * @note Configures the pin as INPUT_PULLUP. Register via
     * appendToBothSchedulers() for hybrid interrupt + periodic operation.
     * @note Emits Sensor::DATA_READ_EVENT when a new flow rate reading is available,
     * consistent with other data-reading sensors in the framework.
     */
    explicit WaterFlowSensor(int signalGpioPin, EventHandler* parentHandler = nullptr);
    ~WaterFlowSensor() override = default;

    /**
     * @brief Returns the latest flow rate reading.
     * @return Flow rate in liters per minute.
     */
    [[nodiscard]] float getFlowRateInLitersPerMinute() const;

};

#endif // WATER_FLOW_SENSOR_H
