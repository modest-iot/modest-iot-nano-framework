/**
 * @file GasSensor.cpp
 * @brief Implements gas level detection and concentration ratio calculation for analog gas sensors.
 * @date 2026-06-06
 * @version 1.0
 */

#include "GasSensor.h"
#include <Arduino.h>
#include <cmath> // For log10 and pow

// Assuming a 3.3V reference for ESP32 ADC and 12-bit resolution.
// These values are typical for ESP32, adjust if using a different board or ADC configuration.
static constexpr float ADC_MAX_VOLTAGE = 3.3f;
static constexpr float ADC_MAX_VALUE = 4095.0f;

// Define the custom digital gas-detected event
const Event GasSensor::DIGITAL_GAS_DETECTED_EVENT = Event(GasSensor::DIGITAL_GAS_DETECTED_EVENT_IDENTIFIER);

/**
 * @brief Constructs a GasSensor.
 * @param analogInputGpioPin Analog input GPIO pin.
 * @param digitalInputGpioPin Digital input GPIO pin.
 * @param parentHandler Upstream handler for events.
 * @param initialMode Initial operating mode for the sensor.
 */
GasSensor::GasSensor(const int analogInputGpioPin, const int digitalInputGpioPin, EventHandler* const parentHandler, const OperatingMode initialMode)
    : Sensor(analogInputGpioPin, parentHandler), // 'pin' from Sensor base class is now analogInputGpioPin
      cachedRawAnalogValue(-1),
      cachedSensorResistanceInKiloOhm(-1.0f),
      cachedRsR0Ratio(-1.0f),
      r0InKiloOhm(0.0f), // Initialize R0 to 0, indicating it's not yet set
      digitalInputPin(digitalInputGpioPin),
      cachedDigitalGasDetected(false),
      currentMode(initialMode),
      targetGasType(GasType::None), // No target gas initially
      calibrationStartTime(0),
      calibrationSumRs(0.0f),
      calibrationReadingsCount(0)
{
    pinMode(pin, INPUT); // Configure analog input pin
    if (digitalInputPin != -1) {
        pinMode(digitalInputPin, INPUT); // Configure digital input pin
    }
    // currentGasCalibration is no longer directly set in constructor, it's loaded on demand

    // Initialize custom calibration arrays
    for (int i = 0; i < static_cast<int>(GasType::MAX_GAS_TYPE_COUNT); ++i) {
        customCalibrationSet[i] = false;
        customGasCalibrations[i] = {0.0f, 0.0f};
    }
}

/**
 * @brief Starts the gas sensor calibration process.
 */
void GasSensor::startCalibration() {
    currentMode = OperatingMode::CalibratingMode;
    calibrationStartTime = millis();
    calibrationSumRs = 0.0f;
    calibrationReadingsCount = 0;
    r0InKiloOhm = 0.0f; // Reset R0 during calibration
    // Serial.println("[GasSensor] Starting calibration. Keep sensor in clean air."); // Removed Serial.println
}

/**
 * @brief Sets the R0 (resistance in clean air) value.
 * @param cleanAirResistanceInKiloOhm The R0 value in kOhm.
 */
void GasSensor::setR0(const float cleanAirResistanceInKiloOhm) {
    if (cleanAirResistanceInKiloOhm > 0.0f) {
        r0InKiloOhm = cleanAirResistanceInKiloOhm;
        currentMode = OperatingMode::AccurateMode;

        // Serial.print("[GasSensor] R0 manually set to: "); // Removed Serial.print
        // Serial.print(r0InKiloOhm); // Removed Serial.print
        // Serial.println(" KiloOhm. Entering AccurateMode."); // Removed Serial.println
    } else {
        // Serial.println("[GasSensor] Invalid R0 value provided. R0 must be positive."); // Removed Serial.println
    }
}

/**
 * @brief Sets the target gas type.
 * @param gasType The GasType to target.
 * @return true if successful, false otherwise.
 */
bool GasSensor::setTargetGas(const GasType gasType) {
    if (gasType == GasType::None || gasType >= GasType::MAX_GAS_TYPE_COUNT) {
        targetGasType = GasType::None;
        // Serial.println("[GasSensor] Invalid gas type selected."); // Removed Serial.println
        return false;
    }
    targetGasType = gasType; // Simply set the target gas type
    return true;
}

/**
 * @brief Sets custom calibration parameters for a gas type.
 * @param gasType The GasType.
 * @param slope The slope of the curve.
 * @param intercept The intercept of the curve.
 * @return true if successful, false otherwise.
 */
bool GasSensor::setGasCalibrationParameters(const GasType gasType, const float slope, const float intercept) {
    if (gasType == GasType::None || gasType >= GasType::MAX_GAS_TYPE_COUNT) {
        return false;
    }
    customGasCalibrations[static_cast<int>(gasType)] = {slope, intercept};
    customCalibrationSet[static_cast<int>(gasType)] = true;
    return true;
}

/**
 * @brief Gets the current operating mode.
 * @return The OperatingMode.
 */
GasSensor::OperatingMode GasSensor::getOperatingMode() const {
    return currentMode;
}

/**
 * @brief Gets the target gas type.
 * @return The GasType.
 */
GasSensor::GasType GasSensor::getTargetGasType() const {
    return targetGasType;
}

/**
 * @brief Loads default gas calibration parameters.
 * @param gasType The GasType.
 * @param calibrationParameters Reference to store parameters.
 * @return true if successful, false otherwise.
 */
bool GasSensor::loadDefaultGasCalibrationParameters(const GasType gasType, GasSensor::GasCalibrationParameters& calibrationParameters) { // Removed redundant static, fully qualified GasCalibrationParameters
    // NOTE: These are EXAMPLE calibration values.
    // Developers MUST replace these with actual datasheet values for their specific MQ sensor model.
    // The values below are loosely based on MQ-2 and MQ-7 datasheets for illustration.
    switch (gasType) {
        case GasType::LPG:
            calibrationParameters = {-0.45f, 2.9f}; // Example for MQ-2
            break;
        case GasType::Methane:
            calibrationParameters = {-0.38f, 3.0f}; // Example for MQ-2
            break;
        case GasType::CarbonMonoxide:
            calibrationParameters = {-0.77f, 3.3f}; // Example for MQ-7 (100ppm)
            break;
        case GasType::Alcohol:
            calibrationParameters = {-0.60f, 2.5f}; // Example for MQ-3
            break;
        case GasType::Hydrogen:
            calibrationParameters = {-0.48f, 2.8f}; // Example for MQ-8
            break;
        case GasType::Smoke:
            calibrationParameters = {-0.42f, 2.7f}; // Example for MQ-2
            break;
        case GasType::Ammonia:
            calibrationParameters = {-0.40f, 2.5f}; // Example for MQ-137
            break;
        case GasType::Benzene:
            calibrationParameters = {-0.50f, 2.8f}; // Example for MQ-135
            break;
        case GasType::Toluene:
            calibrationParameters = {-0.52f, 2.9f}; // Example for MQ-135
            break;
        case GasType::Acetone:
            calibrationParameters = {-0.48f, 2.6f}; // Example for MQ-138
            break;
        case GasType::None:
        default:
            calibrationParameters = {0.0f, 0.0f};
            return false; // No valid gas type selected
    }
    return true;
}

/**
 * @brief Calculates sensor resistance in kOhm.
 * @param rawAnalogValue The raw analog reading.
 * @return The resistance in kOhm.
 */
float GasSensor::calculateSensorResistanceInKiloOhm(const int rawAnalogValue) { // Removed redundant static, added const
    if (const float vOut = static_cast<float>(rawAnalogValue) * (ADC_MAX_VOLTAGE / ADC_MAX_VALUE); vOut > 0.001f) { // Avoid division by zero or near-zero V_out
        return (ADC_MAX_VOLTAGE / vOut - 1.0f) * DEFAULT_LOAD_RESISTOR_IN_KILOOHM;
    }
    return -1.0f;
}

/**
 * @brief Updates digital gas detection state.
 */
void GasSensor::updateDigitalGasDetectedState()  {
    if (digitalInputPin != -1) {
        cachedDigitalGasDetected = digitalRead(digitalInputPin) == HIGH; // Assuming HIGH means gas detected
    }
}

/**
 * @brief Handles calibrating mode logic.
 * @param event The event.
 * @param currentSensorResistanceInKiloOhm Current resistance in kOhm.
 */
void GasSensor::handleCalibratingModeLogic(Event& event, float currentSensorResistanceInKiloOhm) {
    if (currentSensorResistanceInKiloOhm > 0.0f) {
        calibrationSumRs += currentSensorResistanceInKiloOhm;
        calibrationReadingsCount++;
    }

    if (unsigned long currentTime = millis(); (currentTime - calibrationStartTime >= CALIBRATION_DURATION_IN_MILLISECONDS) &&
                                              (calibrationReadingsCount >= CALIBRATION_MIN_READINGS)) {

        r0InKiloOhm = calibrationSumRs / static_cast<float>(calibrationReadingsCount);
        currentMode = OperatingMode::AccurateMode;
        // Update ratio for first accurate reading, ensuring R0 is set
        if (r0InKiloOhm > 0.0f) {
            cachedRsR0Ratio = currentSensorResistanceInKiloOhm / r0InKiloOhm;
        }

        event = DATA_READ_EVENT; // Signal calibration completion and first accurate reading
    } else {
        // Suppress events during calibration
        event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
    }
}

/**
 * @brief Handles accurate mode logic.
 * @param event The event.
 * @param currentSensorResistanceInKiloOhm Current resistance in kOhm.
 */
void GasSensor::handleAccurateModeLogic(Event& event, const float currentSensorResistanceInKiloOhm) {
    float currentRsR0Ratio = -1.0f;
    if (r0InKiloOhm > 0.0f && currentSensorResistanceInKiloOhm > 0.0f) {
        currentRsR0Ratio = currentSensorResistanceInKiloOhm / r0InKiloOhm;
    }

    bool changed = false;
    if (currentRsR0Ratio > 0.0f && cachedRsR0Ratio > 0.0f) { // Only compare if both are valid
        if (fabs(currentRsR0Ratio - cachedRsR0Ratio) > RSR0_CHANGE_THRESHOLD) {
            changed = true;
        }
    } else if (currentRsR0Ratio > 0.0f && cachedRsR0Ratio < 0.0f) { // First valid reading after calibration
        changed = true;
    }

    if (changed) {
        cachedRsR0Ratio = currentRsR0Ratio;
        event = DATA_READ_EVENT; // Mutate to the standard data read event
    } else {
        event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER; // Suppress event if no significant change
    }
}

/**
 * @brief Handles raw mode logic.
 * @param event The event.
 * @param currentSensorResistanceInKiloOhm Current resistance in kOhm.
 */
void GasSensor::handleRawModeLogic(Event& event, const float currentSensorResistanceInKiloOhm) {
    if (digitalInputPin != -1) {
        // In RawMode, if digital pin is configured, propagate DIGITAL_GAS_DETECTED_EVENT on change
        const bool previousDigitalGasDetected = cachedDigitalGasDetected; // Use the value before updateDigitalGasDetectedState()
        updateDigitalGasDetectedState(); // Update cachedDigitalGasDetected

        if (cachedDigitalGasDetected != previousDigitalGasDetected) {
            // Event is only sent when gas is detected (HIGH)
            if (cachedDigitalGasDetected) {
                event = DIGITAL_GAS_DETECTED_EVENT;
            } else {
                // If gas is no longer detected digitally, suppress event
                event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
            }
        } else {
            // No change in digital state, suppress event
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
        }
    } else {
        // Fallback for RawMode if no digital pin: propagate on Rs change
        bool changed = false;
        if (fabs(currentSensorResistanceInKiloOhm - cachedSensorResistanceInKiloOhm) > 0.01f) { // Simple change detection for Rs
             changed = true;
        } else if (currentSensorResistanceInKiloOhm < 0.0f && currentSensorResistanceInKiloOhm > 0.0f) // First valid reading
            changed = true;

        if (changed) {
            event = DATA_READ_EVENT;
        } else {
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
        }
    }
}

/**
 * @brief Processes incoming events.
 * @param event The event to process.
 */
void GasSensor::processEvent(Event& event) {
    if (event.identifier != MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER; // Only process measure requests
        return;
    }

    // --- Read Analog Pin ---
    int currentRawAnalogValue = analogRead(pin);
    float currentSensorResistanceInKiloOhm = calculateSensorResistanceInKiloOhm(currentRawAnalogValue);

    // Always update analog caches
    cachedRawAnalogValue = currentRawAnalogValue;
    cachedSensorResistanceInKiloOhm = currentSensorResistanceInKiloOhm;

    // --- Read Digital Pin (if configured) and update cache ---
    updateDigitalGasDetectedState();

    // --- Mode-specific logic ---
    if (currentMode == OperatingMode::CalibratingMode) {
        handleCalibratingModeLogic(event, currentSensorResistanceInKiloOhm);
    } else if (currentMode == OperatingMode::AccurateMode) {
        handleAccurateModeLogic(event, currentSensorResistanceInKiloOhm);
    } else { // RawMode
        handleRawModeLogic(event, currentSensorResistanceInKiloOhm);
    }
}

/**
 * @brief Gets the cached raw analog value.
 * @return The raw analog value.
 */
int GasSensor::getRawAnalogValue() const {
    return cachedRawAnalogValue;
}

/**
 * @brief Gets the cached sensor resistance in kOhm.
 * @return The resistance in kOhm.
 */
float GasSensor::getSensorResistanceInKiloOhm() const {
    return cachedSensorResistanceInKiloOhm;
}

/**
 * @brief Gets the cached Rs/R0 ratio.
 * @return The Rs/R0 ratio.
 */
float GasSensor::getRsR0Ratio() const {
    if (currentMode != OperatingMode::AccurateMode || r0InKiloOhm <= 0.0f) {
        return -1.0f; // Not in accurate mode or R0 not set
    }
    return cachedRsR0Ratio;
}

/**
 * @brief Calculates and gets the gas concentration in PPM.
 * @return The gas concentration in PPM.
 */
float GasSensor::getGasConcentrationInPpm() const {
    if (currentMode != OperatingMode::AccurateMode || r0InKiloOhm <= 0.0f || targetGasType == GasType::None) {
        return -1.0f; // Not in accurate mode, R0 not set, or no target gas
    }
    if (cachedRsR0Ratio <= 0.0f) {
        return -1.0f; // Invalid ratio
    }

    // Retrieve calibration parameters dynamically
    GasCalibrationParameters parameters{}; // Fully qualified
    if (customCalibrationSet[static_cast<int>(targetGasType)]) {
        parameters = customGasCalibrations[static_cast<int>(targetGasType)];
    } else {
        if (!loadDefaultGasCalibrationParameters(targetGasType, parameters)) {
            return -1.0f; // Failed to load default parameters
        }
    }

    // Formula: PPM = 10^((log10(Rs/R0) - intercept) / slope)
    if (cachedRsR0Ratio > 0.0f && parameters.slope != 0.0f) {
        float logRsR0 = log10(cachedRsR0Ratio);
        float logPpm = (logRsR0 - parameters.intercept) / parameters.slope;
        return pow(10.0f, logPpm);
    }
    return -1.0f;
}

/**
 * @brief Checks if gas is detected.
 * @return true if detected, false otherwise.
 */
bool GasSensor::isGasDetected() const {
    if (currentMode == OperatingMode::RawMode) {
        // In RawMode, if digital pin is configured, use its state
        if (digitalInputPin != -1) {
            return cachedDigitalGasDetected;
        }
        // If no digital pin, RawMode cannot reliably determine 'gas detected'
        // based on raw analog alone without a threshold, so return false.
        return false;
    } else if (currentMode == OperatingMode::AccurateMode) {
        // In AccurateMode, determine gas detected based on Rs/R0 ratio deviation from clean air (1.0)
        if (r0InKiloOhm > 0.0f && cachedRsR0Ratio > 0.0f) {
            // If Rs/R0 is significantly different from 1.0 (clean air), then gas is detected
            return fabs(cachedRsR0Ratio - 1.0f) > ACCURATE_MODE_RSR0_DEVIATION_THRESHOLD;
        }
        return false; // R0 not set or invalid ratio
    }
    // In CalibratingMode, gas is never "detected" for operational purposes
    return false;
}