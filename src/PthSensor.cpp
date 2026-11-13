/**
 * @file PthSensor.cpp
 * @brief Implements the BME280-compatible pressure, temperature, and humidity sensor adapter.
 * @ingroup sensors_module
 * @copyright (c) 2025-2026 Angel Velasquez
 *
 * @author Angel Velasquez
 * @date 2026-11-10
 * @version 1.0
 */

#include "PthSensor.h"
#include <cmath>

PthSensor::PthSensor(const int signalGpioPin, const uint8_t i2cAddress, EventHandler* parentHandler)
    : BmeEnvironmentalSensor(signalGpioPin, i2cAddress, parentHandler)
{
    initialize();
}

bool PthSensor::initialize() {
    Wire.beginTransmission(i2cAddress);
    uint8_t chipId = Wire.endTransmission() == 0 ? 0 : 0;
    if (chipId == 0) {
        Wire.requestFrom(i2cAddress, static_cast<uint8_t>(1));
        if (Wire.available()) {
            chipId = Wire.read();
        }
    }
    if (chipId != BME280_CHIP_ID_VALUE) {
        initialized = false;
        return false;
    }
    
    if (!readCalibrationData()) {
        initialized = false;
        return false;
    }
    
    // Configure sensor for normal mode with high resolution
    Wire.beginTransmission(i2cAddress);
    Wire.write(BME_REGISTER_TEMP_PRESS_X4);
    Wire.write((BME_OSRS_TEMP_16 << 5) | (BME_OSRS_PRESS_16 << 2) | BME280_MODE_NORMAL);
    Wire.write((BME_OSRS_HUMID_16 << 4) | (BME_STANDBY_0_5_MS << 1) | BME_FILTER_COEFF_16);
    (void)Wire.endTransmission();
    
    initialized = true;
    return true;
}

void PthSensor::processEvent(Event& event) {
    if (event.identifier == MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        if (!initialized) {
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
            return;
        }
        
        int32_t rawTemperature, rawPressure;
        uint16_t rawHumidity;
        
        if (!readRawData(rawTemperature, rawPressure, rawHumidity)) {
            reportReadFailure(event);
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
            return;
        }

        const float newTemperature = compensateTemperature(rawTemperature);
        const float newPressure = compensatePressure(rawPressure);
        const float newHumidity = compensateHumidity(rawHumidity);
        
        cachedTemperatureInCelsius = newTemperature;
        cachedPressureInHectopascals = newPressure;
        cachedHumidityInPercentage = newHumidity;

        const bool temperatureChanged = std::fabs(newTemperature - lastNotifiedTemperatureInCelsius) > TEMPERATURE_CHANGE_EPSILON;
        const bool pressureChanged = std::fabs(newPressure - lastNotifiedPressureInHectopascals) > PRESSURE_CHANGE_EPSILON;
        // ReSharper disable once CppTooWideScopeInitStatement
        const bool humidityChanged = std::fabs(newHumidity - lastNotifiedHumidityInPercentage) > HUMIDITY_CHANGE_EPSILON;
        
        if (temperatureChanged || pressureChanged || humidityChanged) {
            reportReadSuccess(event);
            if (temperatureChanged) {
                lastNotifiedTemperatureInCelsius = newTemperature;
            }
            if (pressureChanged) {
                lastNotifiedPressureInHectopascals = newPressure;
            }
            if (humidityChanged) {
                lastNotifiedHumidityInPercentage = newHumidity;
            }
            event.identifier = DATA_READ_EVENT_IDENTIFIER;
            event.sourceId = pin;
        } else {
            reportReadSuccess(event);
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
        }
        return;
    }
    // Non-measurement events pass through unchanged
}

uint32_t PthSensor::getDefaultSamplingIntervalInMilliseconds() const {
    return DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

uint32_t PthSensor::getMinimalSamplingIntervalInMilliseconds() const {
    return MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS;
}
