/**
 * @file EnvironmentalGasSensor.cpp
 * @brief Implements the BME680-compatible environmental gas sensor adapter.
 * @ingroup sensors_module
 * @copyright (c) 2025-2026 Angel Velasquez
 *
 * @author Angel Velasquez
 * @date 2026-11-10
 * @version 1.0
 */

#include "EnvironmentalGasSensor.h"
#include <cmath>

EnvironmentalGasSensor::EnvironmentalGasSensor(const int signalGpioPin, const uint8_t i2cAddress,
                                               EventHandler* parentHandler)
    : BmeEnvironmentalSensor(signalGpioPin, i2cAddress, parentHandler)
    , digG1(0), digG2(0), digG3(0)
    , cachedGasResistanceInKOhms(INITIAL_CACHED_GAS_RESISTANCE_IN_KOHMS)
    , lastNotifiedGasResistanceInKOhms(INITIAL_LAST_NOTIFIED_GAS_RESISTANCE_IN_KOHMS)
{
    initialize();
}

bool EnvironmentalGasSensor::initialize() {
    Wire.beginTransmission(i2cAddress);
    uint8_t chipId = Wire.endTransmission() == 0 ? 0 : 0;
    if (chipId == 0) {
        Wire.requestFrom(i2cAddress, static_cast<uint8_t>(1));
        if (Wire.available()) {
            chipId = Wire.read();
        }
    }
    if (chipId != BME680_CHIP_ID_VALUE) {
        initialized = false;
        return false;
    }
    
    if (!readCalibrationData()) {
        initialized = false;
        return false;
    }
    
    if (!readGasCalibrationData()) {
        initialized = false;
        return false;
    }
    
    // Configure sensor for forced mode with high resolution
    Wire.beginTransmission(i2cAddress);
    Wire.write(BME_REGISTER_TEMP_PRESS_X4);
    Wire.write((BME_OSRS_TEMP_16 << 5) | (BME_OSRS_PRESS_16 << 2) | BME680_MODE_FORCED);
    Wire.write((BME_OSRS_HUMID_16 << 4) | (BME_STANDBY_0_5_MS << 1) | BME_FILTER_COEFF_16);
    (void)Wire.endTransmission();
    
    initialized = true;
    return true;
}

bool EnvironmentalGasSensor::readGasCalibrationData() {
    Wire.beginTransmission(i2cAddress);
    Wire.write(BME680_REGISTER_CALIBRATION_GAS);
    (void)Wire.endTransmission(false);
    
    Wire.requestFrom(i2cAddress, BME680_CALIBRATION_GAS_BYTE_COUNT);
    if (Wire.available() < BME680_CALIBRATION_GAS_BYTE_COUNT) {
        return false;
    }
    
    digG1 = Wire.read();
    digG2 = Wire.read();
    digG3 = Wire.read();
    
    return true;
}

bool EnvironmentalGasSensor::readRawGasData(uint16_t& rawGasResistance, uint8_t& gasRange) const {
    Wire.beginTransmission(i2cAddress);
    Wire.write(BME680_REGISTER_GAS_DATA);
    (void)Wire.endTransmission(false);
    
    Wire.requestFrom(i2cAddress, BME680_RAW_GAS_DATA_BYTE_COUNT);
    if (Wire.available() < BME680_RAW_GAS_DATA_BYTE_COUNT) {
        return false;
    }
    
    uint8_t gasMsb = Wire.read();
    uint8_t gasLsb = Wire.read();
    uint8_t gasRangeByte = Wire.read();
    
    rawGasResistance = static_cast<uint16_t>((gasMsb << 4) | (gasLsb >> 4));
    gasRange = gasRangeByte & 0x0F;
    
    return true;
}

float EnvironmentalGasSensor::getGasResistanceInKOhms() const {
    return cachedGasResistanceInKOhms;
}

float EnvironmentalGasSensor::compensateGasResistance(const uint16_t rawGasResistance, 
                                                       const uint8_t gasRange) const {
    // Gas resistance lookup table from Bosch datasheet
    static constexpr int32_t gasConstArray[] = {
        BME680_GAS_CONST_ARRAY_0, BME680_GAS_CONST_ARRAY_1, BME680_GAS_CONST_ARRAY_2,
        BME680_GAS_CONST_ARRAY_3, BME680_GAS_CONST_ARRAY_4, BME680_GAS_CONST_ARRAY_5,
        BME680_GAS_CONST_ARRAY_6, BME680_GAS_CONST_ARRAY_7, BME680_GAS_CONST_ARRAY_8,
        BME680_GAS_CONST_ARRAY_9, BME680_GAS_CONST_ARRAY_10, BME680_GAS_CONST_ARRAY_11,
        BME680_GAS_CONST_ARRAY_12, BME680_GAS_CONST_ARRAY_13, BME680_GAS_CONST_ARRAY_14,
        BME680_GAS_CONST_ARRAY_15
    };
    
    // Clamp gasRange to valid range
    const uint8_t clampedRange = (gasRange > 15) ? 15 : gasRange;
    const int32_t constArrayValue = gasConstArray[clampedRange];
    
    // Calculate gas resistance using Bosch formula
    const int32_t var1 = (static_cast<int32_t>(digG2) << BME680_GAS_SHIFT_15) + 
                         (static_cast<int32_t>(digG3) * (1 << BME680_GAS_SHIFT_1));
    const int64_t var2 = ((static_cast<int64_t>(rawGasResistance) << BME680_GAS_SHIFT_15) - 
                         (static_cast<int64_t>(1) << BME680_GAS_SHIFT_24));
    const int64_t var3 = (static_cast<int64_t>(constArrayValue) * 
                         static_cast<int64_t>(var1 + static_cast<int32_t>(digG1) * 128)) >> BME680_GAS_SHIFT_9;
    
    if (var2 == 0) {
        return 0.0f;
    }
    
    const int32_t gasResistance = static_cast<int32_t>((var3 + (var2 >> BME680_GAS_SHIFT_1)) / var2);
    
    // Gas resistance must be non-negative; clamp invalid results
    if (gasResistance < 0) {
        return 0.0f;
    }
    
    // Convert from Ohms to kilo-Ohms
    return static_cast<float>(gasResistance) / BME680_OHMS_TO_KOHMS;
}

void EnvironmentalGasSensor::processEvent(Event& event) {
    if (event.identifier == MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        if (!initialized) {
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
            return;
        }
        
        int32_t rawTemperature, rawPressure;
        uint16_t rawHumidity, rawGasResistance;
        uint8_t gasRange;
        
        if (!readRawData(rawTemperature, rawPressure, rawHumidity)) {
            reportReadFailure(event);
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
            return;
        }
        
        if (!readRawGasData(rawGasResistance, gasRange)) {
            reportReadFailure(event);
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
            return;
        }

        const float newTemperature = compensateTemperature(rawTemperature);
        const float newPressure = compensatePressure(rawPressure);
        const float newHumidity = compensateHumidity(rawHumidity);
        const float newGasResistance = compensateGasResistance(rawGasResistance, gasRange);
        
        cachedTemperatureInCelsius = newTemperature;
        cachedPressureInHectopascals = newPressure;
        cachedHumidityInPercentage = newHumidity;
        cachedGasResistanceInKOhms = newGasResistance;

        const bool temperatureChanged = std::fabs(newTemperature - lastNotifiedTemperatureInCelsius) > 
                                        TEMPERATURE_CHANGE_EPSILON;
        const bool pressureChanged = std::fabs(newPressure - lastNotifiedPressureInHectopascals) > 
                                     PRESSURE_CHANGE_EPSILON;
        const bool humidityChanged = std::fabs(newHumidity - lastNotifiedHumidityInPercentage) > 
                                     HUMIDITY_CHANGE_EPSILON;
        const bool gasResistanceChanged = std::fabs(newGasResistance - lastNotifiedGasResistanceInKOhms) > 
                                          GAS_RESISTANCE_CHANGE_EPSILON;
        
        if (temperatureChanged || pressureChanged || humidityChanged || gasResistanceChanged) {
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
            if (gasResistanceChanged) {
                lastNotifiedGasResistanceInKOhms = newGasResistance;
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

uint32_t EnvironmentalGasSensor::getDefaultSamplingIntervalInMilliseconds() const {
    return DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

uint32_t EnvironmentalGasSensor::getMinimalSamplingIntervalInMilliseconds() const {
    return MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS;
}
