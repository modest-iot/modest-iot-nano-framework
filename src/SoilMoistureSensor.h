#ifndef SOIL_MOISTURE_SENSOR_H
#define SOIL_MOISTURE_SENSOR_H

/**
 * @file SoilMoistureSensor.h
 * @brief Declares a soil moisture sensor using analog input.
 * @ingroup sensors_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @license Apache-2.0
 * @date 2026-11-09
 * @version 1.0
 */

#include "Sensor.h"

/**
 * @brief Analog input sensor for soil moisture monitoring.
 * @ingroup sensors_module
 * 
 * Reads soil moisture via an analog pin using `analogRead()` (0–1023 ADC range).
 * Uses epsilon-based change detection to suppress spurious events from ADC noise.
 * Common use cases include smart greenhouse irrigation control and agriculture
 * monitoring. No external libraries required.
 * 
 * @runs_on{sampler}_task
 */
class SoilMoistureSensor : public Sensor {
private:
    static constexpr int    INITIAL_CACHED_MOISTURE_LEVEL         = 0; ///< Initial value for cached moisture level.
    static constexpr float  MOISTURE_CHANGE_EPSILON               = 5.0f; ///< Epsilon for moisture change detection (ADC units).

    /// Default sampling interval in milliseconds.
    /// @note Soil moisture changes slowly; 1 second is sufficient for most applications.
    static constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS = 1000;
    /// Minimum sampling interval in milliseconds.
    /// @note Faster sampling provides diminishing returns for soil moisture.
    static constexpr uint32_t MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 100;

protected:
    int cachedMoistureLevel; ///< Latest moisture reading (raw ADC value).
    int lastNotifiedMoistureLevel; ///< Last notified moisture reading for change detection.

    static constexpr int    PROPAGATION_SUPPRESSION_IDENTIFIER    = -1; ///< Identifier used to suppress error events.
    static constexpr int    INITIAL_LAST_NOTIFIED_MOISTURE_LEVEL  = -1; ///< Initial value for the last notified moisture level.
    /**
     * @brief Performs an analog read and updates cached moisture level.
     * @param event Mutable event request/response payload.
     */
    void processEvent(Event& event) override;

    [[nodiscard]] uint32_t getDefaultSamplingIntervalInMilliseconds() const override;
    [[nodiscard]] uint32_t getMinimalSamplingIntervalInMilliseconds() const override;

public:
    /**
     * @brief Constructs a soil moisture sensor.
     * @param analogInputGpioPin Analog GPIO pin connected to the moisture sensor.
     * @param parentHandler Pointer to the parent handler that manages device events.
     */
    explicit SoilMoistureSensor(int analogInputGpioPin, EventHandler* parentHandler = nullptr);

    /**
     * @brief Returns the latest moisture reading.
     * @return Moisture level as a raw ADC value (0–1023).
     */
    [[nodiscard]] int getMoistureLevel() const;
};

#endif // SOIL_MOISTURE_SENSOR_H
