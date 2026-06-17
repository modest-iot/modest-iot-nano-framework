#ifndef LOAD_CELL_AMPLIFIER_H
#define LOAD_CELL_AMPLIFIER_H

/**
 * @file LoadCellAmplifier.h
 * @brief Declares a LoadCellAmplifier sensor adapter.
 * @ingroup sensors_module
 * @date 2026-06-16
 * @version 1.0
 */

#include "Sensor.h"

/**
 * @name Sensors: Environmental & Mass
 * @ingroup sensors_module
 * @{
 */

/**
 * @brief Sensor adapter for HX711-based load cell amplifiers.
 * @ingroup sensors_module
 */
class LoadCellAmplifier : public Sensor {
private:
    int     clockGpioPin; ///< GPIO pin used for the clock signal.
    float   cachedWeightInGrams; ///< Last successful weight reading in grams.
    float   lastNotifiedWeightInGrams; ///< Last notified weight reading in grams.
    float   minWeightInGrams; ///< Minimum weight value for the LoadCell.
    float   maxWeightInGrams; ///< Maximum weight value for the LoadCell.
    long    minRawValue; ///< Minimum raw ADC value for calibration.
    long    maxRawValue; ///< Maximum raw ADC value for calibration.

    static constexpr int    PROPAGATION_SUPPRESSION_IDENTIFIER  = -1; ///< Identifier used to suppress error events.
    static constexpr float  WEIGHT_CHANGE_EPSILON               = 0.1f; ///< Epsilon for weight change detection.
    static constexpr int    HX711_BIT_COUNT                     = 24; ///< Number of bits to read from HX711.
    static constexpr int    HX711_CLOCK_PULSE_DELAY_US          = 1; ///< Delay in microseconds for clock pulses.
    static constexpr long   HX711_SIGN_BIT_MASK                 = 0x800000; ///< Mask for 24-bit sign bit.
    static constexpr long   HX711_SIGN_EXTENSION_MASK           = 0xFF000000; ///< Mask for sign extension to 32-bit.
    static constexpr float  INITIAL_CACHED_WEIGHT               = 0.0f; ///< Initial value for cached weight.
    static constexpr float  INITIAL_LAST_NOTIFIED_WEIGHT        = -1.0f; ///< Initial value for the last notified weight.
    static constexpr int    MAX_FILTER_SIZE                     = 10; ///< Maximum size of the moving average filter.
    static constexpr unsigned long READ_TIMEOUT_MS              = 100; ///< Timeout for sensor readiness.

    long    rawValueBuffer[MAX_FILTER_SIZE]{}; ///< Buffer for moving average filter.
    int     bufferIndex{}; ///< Current index for the filter buffer.
    int     filterDepth; ///< Current depth of the moving average filter.

    /**
     * @brief Performs the HX711 bit-banging protocol to read the 24-bit raw value.
     * @return The 24-bit raw value read from the HX711.
     */
    [[nodiscard]] long readRawValue() const;

    /**
     * @brief Calculates the weight from the raw ADC value.
     * @param rawValue Raw ADC value from HX711.
     * @return Weight in grams.
     */
    [[nodiscard]] float calculateWeight(long rawValue) const;

    /**
     * @brief Detects if the weight has changed significantly.
     * @return True if weight changed significantly, false otherwise.
     */
    bool changeDetected();

    /**
     * @brief Applies a moving average filter to the raw ADC value.
     * @param rawValue Raw ADC value from HX711.
     * @return Filtered raw ADC value.
     */
    long applyMovingAverageFilter(long rawValue);

protected:
    /**
     * @brief Performs an HX711 read cycle and updates cached measurement.
     * @param event Mutable event request/response payload.
     */
    void processEvent(Event& event) override;

public:
    /**
     * @brief Constructs an HX711 load cell sensor wrapper.
     * @param dataGpioPin Data pin connected to the HX711 Digital OUT.
     * @param clockGpioPin Clock pin connected to the HX711 PD_SCK.
     * @param minWeightInGrams Minimum weight value in grams.
     * @param maxWeightInGrams Maximum weight value in grams.
     * @param minRawValue Minimum raw ADC value for calibration.
     * @param maxRawValue Maximum raw ADC value for calibration.
     * @param filterDepth Depth of the moving average filter (default: 5).
     * @param parentHandler Optional upstream handler for propagated events.
     */
    LoadCellAmplifier(int dataGpioPin, int clockGpioPin, float minWeightInGrams, float maxWeightInGrams, long minRawValue, long maxRawValue, int filterDepth = 5, EventHandler* parentHandler = nullptr);

    /**
     * @brief Returns the most recent successful weight reading.
     * @return Weight in grams.
     */
    [[nodiscard]] float getWeightInGrams() const;

    /**
     * @brief Returns the minimum weight value.
     * @return Minimum weight in grams.
     */
    [[nodiscard]] float getMinWeightInGrams() const;

    /**
     * @brief Returns the maximum weight value.
     * @return Maximum weight in grams.
     */
    [[nodiscard]] float getMaxWeightInGrams() const;

    /**
     * @brief Resets the sensor, clearing cached measures, and re-calibrating.
     */
    void reset();
};

/** @} */ // End of Sensors: Environmental & Mass

#endif // LOAD_CELL_AMPLIFIER_H
