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
    long    tareOffsetRaw; ///< Offset for taring the sensor.
    int     gainPulses; ///< Number of pulses for gain/channel selection.

    static constexpr int    PROPAGATION_SUPPRESSION_IDENTIFIER  = -1; ///< Identifier used to suppress error events.
    static constexpr float  WEIGHT_CHANGE_EPSILON               = 0.1f; ///< Epsilon for weight change detection.
    static constexpr int    CALIBRATION_STABILITY_THRESHOLD     = 20; ///< Threshold for calibration stability.
    static constexpr int    CALIBRATION_SAMPLES                 = 10; ///< Number of samples for calibration.
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
     * @return The 24-bit raw value read from the HX711, or -1 if a timeout occurs.
     */
    [[nodiscard]] long readRawValue() const;

    /**
     * @brief Calculates the weight from the raw ADC value using linear interpolation.
     * @param rawValue Raw ADC value from HX711.
     * @return Weight in grams.
     */
    [[nodiscard]] float calculateWeight(long rawValue) const;

    /**
     * @brief Detects if the weight change between the current and last notified reading exceeds the epsilon threshold.
     * @return True if weight changed significantly, false otherwise.
     */
    bool changeDetected();

    /**
     * @brief Applies a moving average filter to the raw ADC value to reduce noise.
     * @param rawValue Raw ADC value from HX711.
     * @return Filtered raw ADC value.
     */
    long applyMovingAverageFilter(long rawValue);

    /**
     * @brief Checks if the weight value is valid for calibration (must be positive).
     * @param weightInGrams Weight in grams.
     * @return True if valid, false otherwise.
     */
    [[nodiscard]] static bool isValidWeight(float weightInGrams);

    /**
     * @brief Computes the average raw ADC value from multiple calibration samples.
     * @param successfulSamples Reference to store the number of successful samples collected.
     * @return The average raw ADC value.
     */
    long calculateAverageRawValue(int& successfulSamples) const;

    /**
     * @brief Checks if the calibration raw value is stable enough to be accepted.
     * @param averageRawValue The average raw ADC value.
     * @return True if stable, false otherwise.
     */
    [[nodiscard]] bool isCalibrationStable(long averageRawValue) const;

    /**
     * @brief Applies the calibration parameters based on a known weight and raw ADC value.
     * @param knownWeightInGrams The known calibration weight.
     * @param rawValue The corresponding raw ADC value.
     */
    void applyCalibration(float knownWeightInGrams, long rawValue);

protected:
    /**
     * @brief Performs an HX711 read cycle and updates cached measurement.
     * @param event Mutable event request/response payload.
     */
    void processEvent(Event& event) override;

public:
    /**
     * @brief Constructs an HX711 load cell sensor wrapper.
     * @param dataGpioPin GPIO pin connected to the HX711 Data (DOUT) pin.
     * @param clockGpioPin GPIO pin connected to the HX711 Clock (PD_SCK) pin.
     * @param minWeightInGrams The lower bound for weight measurement in grams.
     * @param maxWeightInGrams The upper bound for weight measurement in grams.
     * @param minRawValue The raw ADC reading corresponding to minWeightInGrams.
     * @param maxRawValue The raw ADC reading corresponding to maxWeightInGrams.
     * @param filterDepth Depth of the moving average filter (default: 5).
     * @param parentHandler Pointer to the parent handler that manages device events.
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

    /**
     * @brief Sets the gain and channel for the HX711.
     * @param gain Gain value (128 or 32).
     */
    void setGain(int gain);

    /**
     * @brief Powers down the HX711 sensor.
     */
    void powerDown();

    /**
     * @brief Powers up the HX711 sensor.
     */
    void powerUp();

    /**
     * @brief Tares the sensor, setting the current weight to zero.
     */
    void tare();

    /**
     * @brief Calibrates the sensor with a known weight.
     * @param knownWeightInGrams Weight currently on the scale in grams.
     * @return True if calibration successful, false otherwise.
     */
    bool calibrate(float knownWeightInGrams);
};

/** @} */ // End of Sensors: Environmental & Mass

#endif // LOAD_CELL_AMPLIFIER_H
