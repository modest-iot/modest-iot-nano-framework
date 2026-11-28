/**
 * @file PowerManager.cpp
 * @brief Implements battery voltage sensing and power monitoring for ESP32 IoT devices.
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-14
 * @version 1.0
 */

#include "PowerManager.h"
#include <Arduino.h>
#include <cmath>
#include <limits>

// Static member definitions
uint8_t     PowerManager::batteryPin                       = 0;
float       PowerManager::dividerRatio                     = 1.0f;
uint32_t    PowerManager::vccReferenceInMillivolts         = DEFAULT_VCC_REFERENCE_IN_MILLIVOLTS;
uint16_t    PowerManager::adcMaximumValue                  = ADC_MAX_VALUE;
uint32_t    PowerManager::lowBatteryThresholdInMillivolts  = 3300; ///< Default low-battery threshold: 3.3V
uint32_t    PowerManager::minVoltageInMillivolts           = 3000; ///< Default empty battery: 3.0V (Li-ion)
uint32_t    PowerManager::maxVoltageInMillivolts           = 4200; ///< Default full battery: 4.2V (Li-ion)
bool        PowerManager::initialized                      = false;

bool PowerManager::begin(const uint8_t pin,
                         const float dividerRatioValue,
                         const uint32_t vccReference,
                         const uint16_t adcMaximum) {
    initialized = false;
    if (pin == INVALID_BATTERY_ADC_PIN || !std::isfinite(dividerRatioValue) ||
        dividerRatioValue <= 0.0f || vccReference == 0 || adcMaximum == 0) {
        return false;
    }

    batteryPin = pin;
    dividerRatio = dividerRatioValue;
    vccReferenceInMillivolts = vccReference;
    adcMaximumValue = adcMaximum;
    initialized = true;
#ifdef ARDUINO_ARCH_ESP32
    pinMode(pin, INPUT);
#endif
    return true;
}

BatteryMeasurement PowerManager::getBatteryMeasurement() {
    if (!initialized) {
        return {};
    }

    const int rawADC = analogRead(batteryPin);
    if (rawADC < 0 || static_cast<uint32_t>(rawADC) > adcMaximumValue) {
        return {};
    }

    const double voltage = static_cast<double>(rawADC) *
                           static_cast<double>(vccReferenceInMillivolts) *
                           static_cast<double>(dividerRatio) /
                           static_cast<double>(adcMaximumValue);
    if (!std::isfinite(voltage) || voltage > static_cast<double>(std::numeric_limits<uint32_t>::max())) {
        return {};
    }

    return {true, static_cast<uint32_t>(voltage)};
}

uint32_t PowerManager::getBatteryVoltageInMillivolts() {
    const BatteryMeasurement measurement = getBatteryMeasurement();
    return measurement.valid ? measurement.voltageInMillivolts : 0;
}

float PowerManager::getBatteryLevelInPercentage() {
    return getBatteryLevelInPercentage(getBatteryMeasurement());
}

float PowerManager::getBatteryLevelInPercentage(const BatteryMeasurement& measurement) {
    if (!measurement.valid || maxVoltageInMillivolts <= minVoltageInMillivolts) {
        return 0.0f;
    }

    const uint32_t voltageInMillivolts = measurement.voltageInMillivolts;

    if (voltageInMillivolts <= minVoltageInMillivolts) {
        return 0.0f;
    }
    if (voltageInMillivolts >= maxVoltageInMillivolts) {
        return 100.0f;
    }

    // Linear interpolation between min and max voltage
    const float levelInPercentage = static_cast<float>(voltageInMillivolts - minVoltageInMillivolts) /
                  static_cast<float>(maxVoltageInMillivolts - minVoltageInMillivolts) * 100.0f;

    return levelInPercentage;
}

bool PowerManager::isLowBattery() {
    const BatteryMeasurement measurement = getBatteryMeasurement();
    return measurement.valid && lowBatteryThresholdInMillivolts > 0 &&
           measurement.voltageInMillivolts < lowBatteryThresholdInMillivolts;
}

bool PowerManager::setLowBatteryThresholdInMillivolts(const uint32_t thresholdInMillivolts) {
    if (thresholdInMillivolts == 0 || thresholdInMillivolts > maxVoltageInMillivolts) {
        return false;
    }
    lowBatteryThresholdInMillivolts = thresholdInMillivolts;
    return true;
}

uint32_t PowerManager::getLowBatteryThresholdInMillivolts() {
    return lowBatteryThresholdInMillivolts;
}

bool PowerManager::setBatteryVoltageRange(const uint32_t minimumVoltageInMillivolts,
                                          const uint32_t maximumVoltageInMillivolts) {
    if (minimumVoltageInMillivolts >= maximumVoltageInMillivolts ||
        lowBatteryThresholdInMillivolts > maximumVoltageInMillivolts) {
        return false;
    }
    minVoltageInMillivolts = minimumVoltageInMillivolts;
    maxVoltageInMillivolts = maximumVoltageInMillivolts;
    return true;
}

uint32_t PowerManager::getMinVoltageInMillivolts() {
    return minVoltageInMillivolts;
}

uint32_t PowerManager::getMaxVoltageInMillivolts() {
    return maxVoltageInMillivolts;
}

bool PowerManager::isInitialized() {
    return initialized;
}
