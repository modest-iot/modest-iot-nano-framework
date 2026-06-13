#ifndef PULSE_SENSOR_H
#define PULSE_SENSOR_H

/**
 * @file PulseSensor.h
 * @brief Declares a heartbeat sensor adapter with adaptive threshold detection.
 * @ingroup sensors_module
 * @date 2026-06-06
 * @version 1.0
 */

#include "Sensor.h"

/**
 * @name Sensors: Biometric & Analog
 * @ingroup sensors_module
 * @{
 */

/**
 * @brief Analog pulse sensor that estimates beats-per-minute from waveform peaks.
 * @ingroup sensors_module
 */
class PulseSensor : public Sensor {
private:
    int cachedBeatsPerMinute; ///< Last calculated heart rate in BPM.
    int sampleSignalValue; ///< Last raw signal sample.
    unsigned long lastBeatTimestamp; ///< Timestamp of the last detected beat.
    
    bool searchForHeartbeat; ///< Flag indicating if a heartbeat is being searched.
    int runningSignalPeak; ///< Peak value of the signal.
    int runningSignalTrough; ///< Trough value of the signal.
    int adaptiveThresholdValue; ///< Adaptive threshold value.

    /// --- SCOPED EXPLICIT CONSTANTS (ELIMINATING MAGIC NUMBERS) ---
    static constexpr int DEFAULT_STABILIZATION_PEAK_BASELINE = 512; ///< Default stabilization peak baseline.
    static constexpr int HEARTBEAT_IGNITION_MINIMUM_AMPLITUDE = 100; ///< Minimum amplitude for heartbeat ignition.
    static constexpr unsigned long DEBOUNCE_HEARTBEAT_WINDOW_IN_MILLISECONDS = 300; ///< Debounce a heartbeat window in milliseconds.
    static constexpr int PROPAGATION_SUPPRESSION_IDENTIFIER = -1; ///< Identifier for propagation suppression.
    
    // Mathematical normalization scaling figures
    static constexpr int MILLISECONDS_PER_MINUTE_COEFFICIENT = 60000; ///< Milliseconds per minute coefficient.
    static constexpr int MINIMUM_PHYSIOLOGICAL_HEART_RATE_BPM = 30; ///< Minimum physiological heart rate in BPM.
    static constexpr int MAXIMUM_PHYSIOLOGICAL_HEART_RATE_BPM = 200; ///< Maximum physiological heart rate in BPM.
    static constexpr int DOUBLE_DEBOUNCE_MULTIPLIER = 2; ///< Multiplier for double debouncing.

protected:
    /**
     * @brief Intercepts the unified base-class event identifier to perform ADC sampling.
     * Mutates the event to Sensor::DATA_READ_EVENT only when a valid new heartbeat cycle is captured.
     */
    void processEvent(Event& event) override;

public:
    /**
     * @brief Constructs a pulse sensor wrapper.
     * @param analogInputGpioPin Analog input pin connected to the pulse module.
     * @param parentHandler Optional upstream handler for propagated events.
     */
    PulseSensor(int analogInputGpioPin, EventHandler* parentHandler = nullptr);

    /**
     * @brief Returns the latest validated heart-rate estimate.
     * @return Beats per minute.
     */
    [[nodiscard]] int getBeatsPerMinute() const;

    /**
     * @brief Returns the latest raw ADC sample value.
     * @return Raw analog sample from the pulse input pin.
     */
    [[nodiscard]] int getRawSignalValue() const;
};

/** @} */ // End of Sensors: Biometric & Analog

#endif // PULSE_SENSOR_H