#ifndef GAS_SENSOR_H
#define GAS_SENSOR_H

/**
 * @file GasSensor.h
 * @brief Declares a Gas Sensor adapter (e.g., MQ-series) for detecting gas levels.
 * @ingroup sensors_module
 * @date 2026-06-06
 * @version 1.0
 */

#include "Sensor.h"

/**
 * @name Sensors: Environmental & Distance
 * @ingroup sensors_module
 * @{
 */

/**
 * @brief Adapter for analog gas sensors (e.g., MQ-series) to detect gas presence and concentration.
 *
 * This sensor reads an analog voltage proportional to the gas concentration. It provides
 * the raw analog reading, the calculated sensor resistance (Rs), and the Rs/R0 ratio.
 * The Rs/R0 ratio can then be used with gas-specific calibration curves to estimate
 * gas concentration in Parts Per Million (PPM).
 *
 * The sensor supports two main operating modes:
 * - RawMode: Primarily provides digital gas detection (if digital pin configured) or raw analog/Rs values.
 * - AccurateMode: Provides Rs/R0 ratio and PPM estimations after a calibration phase
 *   where the R0 (resistance in clean air) is determined.
 */
class GasSensor : public Sensor {
public:
    /**
     * @brief Defines the operating mode of the GasSensor.
     */
    enum class OperatingMode {
        RawMode,          ///< Primarily digital gas detection (if digital pin configured) or raw analog/Rs values.
        CalibratingMode,  ///< Actively collecting data to determine R0; suppresses events.
        AccurateMode      ///< R0 is set, provides Rs/R0 ratio and PPM estimations.
    };

    /**
     * @brief Defines the types of gases that can be targeted for PPM estimation.
     * These correspond to specific calibration curves for MQ-series sensors.
     * NOTE: These are generic types; actual MQ sensor models (e.g., MQ-2, MQ-7)
     * have different sensitivities and curves for these gases.
     */
    enum class GasType {
        None,       ///< No specific gas type selected for PPM estimation.
        LPG,        ///< Liquefied Petroleum Gas (Propane, Butane)
        Methane,    ///< CH4, Natural Gas
        CarbonMonoxide, ///< CO
        Alcohol,    ///< Ethanol, C2H5OH
        Propane,    ///< C3H8
        Hydrogen,   ///< H2
        Smoke,      ///< General smoke detection
        Ammonia,    ///< NH3
        Benzene,    ///< C6H6
        Toluene,    ///< C7H8
        Acetone,    ///< C3H6O
        _MAX_GAS_TYPE_COUNT ///< Internal use: Represents the count of defined gas types.
    };

    // Calibration parameters struct (moved here for public visibility for static methods)
    struct GasCalibrationParameters {
        float slope;
        float intercept;
    };

private:
    // Analog-related members
    int cachedRawAnalogValue;
    float cachedSensorResistanceInKiloOhm;
    float cachedRsR0Ratio; // This will now store Rs/R0
    float r0InKiloOhm; // Resistance to clean air (calibrated baseline)

    // Digital-related members
    int digitalInputPin; ///< Digital input pin for threshold detection (e.g., D0 on breakout board).
    bool cachedDigitalGasDetected;

    OperatingMode currentMode;
    GasType targetGasType; ///< The specific gas type this sensor is currently configured to detect for PPM.

    // Storage for user-provided custom calibration parameters
    GasCalibrationParameters customGasCalibrations[static_cast<int>(GasType::_MAX_GAS_TYPE_COUNT)];
    bool customCalibrationSet[static_cast<int>(GasType::_MAX_GAS_TYPE_COUNT)];

    // Calibration state variables
    unsigned long calibrationStartTime;
    float calibrationSumRs;
    int calibrationReadingsCount;

    // A simple load resistor value for calculating Rs.
    // This would typically be part of the sensor's external circuit.
    static constexpr float DEFAULT_LOAD_RESISTOR_IN_KILOOHM = 10.0f; // Example: 10 KiloOhm

    // Threshold for detecting a significant change in Rs/R0 ratio to trigger an event.
    static constexpr float RSR0_CHANGE_THRESHOLD = 0.05f; // 5% change in Rs/R0 ratio

    // Calibration constants
    static constexpr unsigned long CALIBRATION_DURATION_IN_MILLISECONDS = 30000; // 30 seconds for calibration
    static const int CALIBRATION_MIN_READINGS = 10; // Minimum readings needed for R0 calculation

    // Threshold for accurate mode gas detection (deviation from clean air Rs/R0 = 1.0)
    static constexpr float ACCURATE_MODE_RSR0_DEVIATION_THRESHOLD = 0.15f; // 15% deviation from 1.0

    static const int PROPAGATION_SUPPRESSION_IDENTIFIER = -1;

    /**
     * @brief Loads the default calibration slope and intercept for a given gas type.
     * These are example values and should be overridden by developers for specific sensor models.
     * @param gasType The GasType to load default calibration parameters for.
     * @param calibrationParameters Reference to a struct to populate with the loaded parameters.
     * @return True if default calibration parameters were found and loaded, false otherwise.
     */
    static bool loadDefaultGasCalibrationParameters(const GasType gasType, GasCalibrationParameters& calibrationParameters); // Added const

    /**
     * @brief Calculates the sensor resistance (Rs) in KiloOhm from a raw analog reading.
     * @param rawAnalogValue The raw ADC reading from the analog input pin.
     * @return The calculated sensor resistance in KiloOhm.
     */
    static float calculateSensorResistanceInKiloOhm(const int rawAnalogValue); // Added const

    /**
     * @brief Reads the digital input pin (if configured) and updates the cached digital gas-detected state.
     */
    void updateDigitalGasDetectedState();

    /**
     * @brief Handles the logic for the CalibratingMode.
     * Collects Rs readings and transitions to AccurateMode after calibration.
     * @param event Mutable event request/response payload.
     * @param currentSensorResistanceInKiloOhm The most recent calculated sensor resistance.
     */
    void handleCalibratingModeLogic(Event& event, const float currentSensorResistanceInKiloOhm); // Added const

    /**
     * @brief Handles the logic for the AccurateMode.
     * Calculates Rs/R0 ratio and propagates events based on significant changes.
     * @param event Mutable event request/response payload.
     * @param currentSensorResistanceInKiloOhm The most recent calculated sensor resistance.
     */
    void handleAccurateModeLogic(Event& event, const float currentSensorResistanceInKiloOhm); // Added const

    /**
     * @brief Handles the logic for the RawMode.
     * Propagates events based on digital pin state or raw analog/Rs changes.
     * @param event Mutable event request/response payload.
     * @param currentSensorResistanceInKiloOhm The most recent calculated sensor resistance.
     */
    void handleRawModeLogic(Event& event, const float currentSensorResistanceInKiloOhm); // Added const

protected:
    /**
     * @brief Performs an analog read and updates cached gas level measurements.
     * Handles mode-specific logic for raw, calibrating, and accurate modes.
     * @param event Mutable event request/response payload.
     */
    void processEvent(Event& event) override;

public:
    /** @brief Identifier emitted when a digital gas detection threshold is crossed. */
    static const int DIGITAL_GAS_DETECTED_EVENT_IDENTIFIER = 40;
    /** @brief Prebuilt event instance for digital gas detection notifications. */
    static const Event DIGITAL_GAS_DETECTED_EVENT;

    /**
     * @brief Constructs a Gas Sensor adapter.
     * @param analogInputGpioPin Analog input pin connected to the gas sensor's output (e.g., A0 on breakout board).
     * @param digitalInputGpioPin Digital input pin for threshold detection (e.g., D0 on breakout board, default -1 if not used).
     * @param parentHandler Optional upstream handler for propagated events.
     * @param initialMode The initial operating mode of the sensor (default: RawMode).
     */
    GasSensor(const int analogInputGpioPin, const int digitalInputGpioPin = -1, EventHandler* const parentHandler = nullptr, const OperatingMode initialMode = OperatingMode::RawMode); // Added const

    /**
     * @brief Initiates the calibration process for the sensor.
     * The sensor will enter CalibratingMode, collect readings, and determine R0.
     * During calibration, no DATA_READ_EVENTs are propagated.
     * The sensor automatically transitions to AccurateMode upon successful calibration.
     */
    void startCalibration();

    /**
     * @brief Sets the R0 (resistance in clean air) value for calibration manually.
     * This can be used if R0 is known beforehand or determined externally.
     * Setting R0 manually will switch the sensor to AccurateMode.
     * @param cleanAirResistanceInKiloOhm The R0 value in KiloOhm.
     */
    void setR0(const float cleanAirResistanceInKiloOhm); // Added const

    /**
     * @brief Configures the sensor to target a specific gas type for PPM estimation.
     * This method only sets the target gas type identifier. The actual calibration
     * parameters (custom or default) are loaded dynamically when `getGasConcentrationInPpm()` is called.
     * @param gasType The GasType to configure the sensor for.
     * @return True if the gas type is valid, false otherwise.
     */
    bool setTargetGas(const GasType gasType); // Added const

    /**
     * @brief Allows the developer to provide custom calibration parameters (slope and intercept)
     * for a specific gas type. This data will override any default values.
     * @param gasType The GasType for which to set custom calibration parameters.
     * @param slope The slope of the gas's calibration curve on a log-log plot.
     * @param intercept The Y-intercept of the gas's calibration curve on a log-log plot.
     * @return True if the custom calibration parameters were successfully set, false otherwise.
     */
    bool setGasCalibrationParameters(const GasType gasType, const float slope, const float intercept); // Added const

    /**
     * @brief Returns the current operating mode of the sensor.
     * @return The current OperatingMode.
     */
    OperatingMode getOperatingMode() const;

    /**
     * @brief Returns the currently configured target gas type for PPM estimation.
     * @return The current GasType.
     */
    GasType getTargetGasType() const;

    /**
     * @brief Returns the most recent raw analog reading from the sensor.
     * @return Raw analog value (e.g., 0-4095 for ESP32 ADC).
     */
    int getRawAnalogValue() const;

    /**
     * @brief Returns the most recent calculated sensor resistance (Rs) in KiloOhm.
     * @return Sensor resistance in KiloOhm.
     */
    float getSensorResistanceInKiloOhm() const;

    /**
     * @brief Returns the most recent calculated Rs/R0 ratio.
     * This ratio is used with calibration curves to determine gas concentration.
     * Returns -1.0f if not in AccurateMode or R0 is not set.
     * @return Rs/R0 ratio.
     */
    float getRsR0Ratio() const;

    /**
     * @brief Estimates the gas concentration in PPM for the currently targeted gas type.
     *
     * This method uses the internally stored calibration parameters (slope and intercept)
     * for the gas type set via `setTargetGas()`.
     * The formula used is PPM = 10^((log10(Rs/R0) - intercept) / slope)).
     * Returns -1.0f if not in AccurateMode, Rs/R0 is invalid, R0 is not set,
     * or no target gas type has been configured.
     *
     * @return Estimated gas concentration in PPM.
     */
    float getGasConcentrationInPpm() const;

    /**
     * @brief Determines if gas is currently detected based on the sensor's operating mode.
     * - In RawMode: Returns true if the digital pin indicates gas, or if no digital pin,
     *   it returns false (as analog raw data alone isn't sufficient for 'detected').
     * - In CalibratingMode: Always returns false.
     * - In AccurateMode: Returns true if the Rs/R0 ratio deviates significantly from 1.0.
     * @return `true` if gas is detected, `false` otherwise.
     */
    bool isGasDetected() const;
};

/** @} */ // End of Sensors: Environmental & Distance

#endif // GAS_SENSOR_H
