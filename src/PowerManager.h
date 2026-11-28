/**
 * @file PowerManager.h
 * @brief Manages battery voltage sensing and power monitoring for ESP32 IoT devices.
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-14
 * @version 1.0
 * @ingroup core_module
 */

#ifndef MODEST_IOT_NANO_FRAMEWORK_POWER_MANAGER_H
#define MODEST_IOT_NANO_FRAMEWORK_POWER_MANAGER_H

#include <cstdint>

/**
 * @brief Default full-scale raw ADC value for a 12-bit reading.
 *
 * Applications should pass the actual configured ADC full-scale value to
 * begin() when using a different resolution.
 */
constexpr uint16_t ADC_MAX_VALUE = 4095;

/**
 * @brief Legacy nominal ADC conversion scale in millivolts.
 *
 * This is a compatibility default, not a calibrated or universal ESP32 value
 * or the chip's internal reference voltage. Applications should provide a
 * board- and attenuation-appropriate full-scale conversion, calibrated against
 * a meter where battery policy depends on it.
 */
constexpr uint32_t DEFAULT_VCC_REFERENCE_IN_MILLIVOLTS = 1100;

/** @brief Invalid ADC pin sentinel; valid GPIO selection is board-specific. */
constexpr uint8_t INVALID_BATTERY_ADC_PIN = 0xFF;

/**
 * @brief Result of one battery ADC conversion.
 *
 * A valid zero-millivolt reading is distinct from an unavailable or invalid
 * measurement.
 */
struct BatteryMeasurement {
    bool valid = false;
    uint32_t voltageInMillivolts = 0;
};

/**
 * @brief Manages battery voltage sensing and power monitoring.
 *
 * Reads battery voltage via an ADC pin with configurable voltage divider ratio,
 * calculates battery level percentage, and provides low-battery threshold alerts.
 *
 * All methods are static and share one configuration. Configure during startup
 * before concurrent tasks read measurements; this class does not synchronize
 * concurrent configuration and reads.
 *
 * @par Voltage Divider
 * Most battery circuits use a voltage divider to scale the battery voltage to
 * the ADC input range. For example, a 2:1 divider (two equal resistors) halves
 * the battery voltage before it reaches the ADC. The divider ratio is configured
 * via begin(). The reference/scale, raw full-scale value, ADC resolution, and
 * attenuation must match the target board's actual configuration.
 *
 * @par Example
 * @code
 * // Configure battery monitoring on ADC pin 34 with a 2:1 voltage divider
 * PowerManager::begin(34, 2.0f, 3300, ADC_MAX_VALUE);
 *
 * // Read battery voltage
 * uint32_t voltage = PowerManager::getBatteryVoltageInMillivolts(); // in millivolts
 * float level = PowerManager::getBatteryLevelInPercentage();        // 0.0 to 100.0
 *
 * // Check low battery status
 * if (PowerManager::isLowBattery()) {
 *     // Take action: reduce sampling, enter deep sleep, etc.
 * }
 * @endcode
 *
 * @par Threading
 * @runs_on{startup/readers} — configure before starting concurrent readers;
 * reconfiguration is not thread-safe.
 */
class PowerManager {
public:
    /**
     * @brief Initializes battery monitoring on the specified ADC pin.
     * @param batteryPin ADC pin connected to the battery voltage divider.
     * @param dividerRatio Voltage divider ratio (e.g., 2.0f for a 2:1 divider).
     * @param vccReferenceInMillivolts Effective ADC full-scale conversion in millivolts
     *        (legacy default 1100 mV; supply a board-specific value for new uses).
     * @param adcMaximumValue Maximum valid raw ADC value (default 4095 for 12-bit).
     * @return true when the configuration is valid; false disables monitoring.
     * @note Call during setup before reading battery values. Pin capability is
     *       board-specific and must be verified by the application.
     *       The divider ratio scales the ADC reading back to the actual battery voltage.
     *       For example, with a 2:1 divider and a 3.7V battery, the ADC sees ~1.85V.
     */
    static bool begin(uint8_t batteryPin,
                      float dividerRatio,
                      uint32_t vccReferenceInMillivolts = DEFAULT_VCC_REFERENCE_IN_MILLIVOLTS,
                      uint16_t adcMaximumValue = ADC_MAX_VALUE);

    /**
     * @brief Reads and validates one battery measurement.
     * @return A valid voltage, or `valid == false` when uninitialized or when
     *         the raw ADC result is outside the configured range/conversion.
     */
    [[nodiscard]] static BatteryMeasurement getBatteryMeasurement();

    /**
     * @brief Reads the current battery voltage from the ADC.
     * @return Battery voltage in millivolts, or 0 when unavailable. Use
     *         getBatteryMeasurement() to distinguish unavailable from zero volts.
     * @note Applies the configured voltage divider ratio and ADC reference voltage
     *       to convert the raw ADC reading to actual battery voltage.
     */
    [[nodiscard]] static uint32_t getBatteryVoltageInMillivolts();

    /**
     * @brief Returns the estimated battery level as a percentage.
     * @return Battery level from 0.0 (empty) to 100.0 (full).
     * @note Uses a linear scale between minVoltage and maxVoltage.
     *       Values below minVoltage return 0.0; values above maxVoltage return 100.0.
     *       Default range is 3000 mV (empty) to 4200 mV (full) for Li-ion batteries.
     */
    [[nodiscard]] static float getBatteryLevelInPercentage();

    /** @brief Calculates the percentage from an already-read measurement. */
    [[nodiscard]] static float getBatteryLevelInPercentage(const BatteryMeasurement& measurement);

    /**
     * @brief Checks whether the battery voltage is below the low-battery threshold.
     * @return true only for a valid measurement below the configured threshold;
     *         unavailable readings never indicate low battery.
     * @note Default threshold is 3300 mV. Applications can adjust via setLowBatteryThresholdInMillivolts().
     */
    [[nodiscard]] static bool isLowBattery();

    /**
     * @brief Sets the low-battery voltage threshold.
     * @param thresholdInMillivolts Low-battery threshold in millivolts (default 3300 mV).
     * @return true if the threshold is nonzero and within the configured maximum voltage.
     * @note When battery voltage drops below this threshold, isLowBattery() returns true.
     */
    static bool setLowBatteryThresholdInMillivolts(uint32_t thresholdInMillivolts);

    /**
     * @brief Returns the current low-battery threshold.
     * @return Low-battery threshold in millivolts.
     */
    [[nodiscard]] static uint32_t getLowBatteryThresholdInMillivolts();

    /**
     * @brief Sets the voltage range for battery level percentage calculation.
     * @param minimumVoltageInMillivolts Empty battery voltage in millivolts (default 3000 mV).
     * @param maximumVoltageInMillivolts Full battery voltage in millivolts (default 4200 mV).
     * @return true if the range is ordered and includes the configured low-battery threshold.
     * @note Used by getBatteryLevelInPercentage() to calculate the percentage.
     *       Adjust these values to match your specific battery chemistry.
     */
    static bool setBatteryVoltageRange(uint32_t minimumVoltageInMillivolts,
                                       uint32_t maximumVoltageInMillivolts);

    /**
     * @brief Returns the minimum voltage for battery level calculation.
     * @return Minimum voltage in millivolts.
     */
    [[nodiscard]] static uint32_t getMinVoltageInMillivolts();

    /**
     * @brief Returns the maximum voltage for battery level calculation.
     * @return Maximum voltage in millivolts.
     */
    [[nodiscard]] static uint32_t getMaxVoltageInMillivolts();

    /**
     * @brief Checks whether battery monitoring has been initialized.
     * @return true if begin() accepted the conversion configuration. The application
     *         remains responsible for ensuring the selected pin supports ADC input.
     */
    [[nodiscard]] static bool isInitialized();

private:
    static uint8_t batteryPin;          ///< ADC pin connected to the battery voltage divider.
    static float dividerRatio;          ///< Voltage divider ratio for scaling ADC readings.
    static uint32_t vccReferenceInMillivolts;     ///< ADC reference voltage in millivolts.
    static uint16_t adcMaximumValue;    ///< Maximum valid ADC reading for the configured resolution.
    static uint32_t lowBatteryThresholdInMillivolts; ///< Low-battery threshold in millivolts.
    static uint32_t minVoltageInMillivolts;       ///< Minimum voltage for battery level calculation (empty).
    static uint32_t maxVoltageInMillivolts;       ///< Maximum voltage for battery level calculation (full).
    static bool initialized;            ///< Tracks whether begin() was called.
};

#endif // MODEST_IOT_NANO_FRAMEWORK_POWER_MANAGER_H
