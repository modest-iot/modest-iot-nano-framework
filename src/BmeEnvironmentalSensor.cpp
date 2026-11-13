/**
 * @file BmeEnvironmentalSensor.cpp
 * @brief Implements the abstract base class for Bosch BME280/BME680 environmental sensors.
 * @ingroup sensors_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @license Apache-2.0
 *
 * @author Angel Velasquez
 * @date 2026-11-10
 * @version 1.0
 */

#include "BmeEnvironmentalSensor.h"
#include <cmath>

BmeEnvironmentalSensor::~BmeEnvironmentalSensor() = default;

BmeEnvironmentalSensor::BmeEnvironmentalSensor(const int signalGpioPin, const uint8_t i2cAddress,
                                               EventHandler* parentHandler)
    : Sensor(signalGpioPin, parentHandler)
    , i2cAddress(i2cAddress)
    , digT1(0), digT2(0), digT3(0)
    , digP1(0), digP2(0), digP3(0), digP4(0)
    , digP5(0), digP6(0), digP7(0), digP8(0), digP9(0)
    , digH1(0), digH2(0), digH3(0), digH4(0), digH5(0), digH6(0)
    , tFine(0)
    , cachedTemperatureInCelsius(INITIAL_CACHED_TEMPERATURE_IN_CELSIUS)
    , cachedHumidityInPercentage(INITIAL_CACHED_HUMIDITY_IN_PERCENTAGE)
    , cachedPressureInHectopascals(INITIAL_CACHED_PRESSURE_IN_HECTOPASCALS)
    , lastNotifiedTemperatureInCelsius(INITIAL_LAST_NOTIFIED_TEMPERATURE_IN_CELSIUS)
    , lastNotifiedHumidityInPercentage(INITIAL_LAST_NOTIFIED_HUMIDITY_IN_PERCENTAGE)
    , lastNotifiedPressureInHectopascals(INITIAL_LAST_NOTIFIED_PRESSURE_IN_HECTOPASCALS)
    , initialized(false)
{
    Wire.begin();
}

bool BmeEnvironmentalSensor::isInitialized() const {
    return initialized;
}

float BmeEnvironmentalSensor::getTemperatureInCelsius() const {
    return cachedTemperatureInCelsius;
}

float BmeEnvironmentalSensor::getHumidityInPercentage() const {
    return cachedHumidityInPercentage;
}

float BmeEnvironmentalSensor::getPressureInHectopascals() const {
    return cachedPressureInHectopascals;
}

float BmeEnvironmentalSensor::getAltitudeInMeters(const float seaLevelPressureInHectopascals) const {
    if (seaLevelPressureInHectopascals <= 0.0f) {
        return 0.0f;
    }
    if (cachedPressureInHectopascals <= 0.0f) {
        return 0.0f;
    }
    const float pressureRatio = cachedPressureInHectopascals / seaLevelPressureInHectopascals;
    return ALTITUDE_SCALE_HEIGHT_IN_METERS * (1.0f - std::powf(pressureRatio, ALTITUDE_BAROMETRIC_EXPONENT));
}

int16_t BmeEnvironmentalSensor::readRegister16(const uint8_t registerAddress) const {
    Wire.beginTransmission(i2cAddress);
    Wire.write(registerAddress);
    (void)Wire.endTransmission(false);
    
    Wire.requestFrom(i2cAddress, static_cast<uint8_t>(2));
    if (Wire.available() >= 2) {
        uint8_t lsb = Wire.read();
        uint8_t msb = Wire.read();
        return static_cast<int16_t>(msb << 8 | lsb);
    }
    return 0;
}

bool BmeEnvironmentalSensor::readCalibrationData() {
    Wire.beginTransmission(i2cAddress);
    Wire.write(BME_REGISTER_CALIBRATION_START);
    (void)Wire.endTransmission(false);
    
    Wire.requestFrom(i2cAddress, BME_CALIBRATION_TEMP_PRESSURE_BYTE_COUNT);
    if (Wire.available() < BME_CALIBRATION_TEMP_PRESSURE_BYTE_COUNT) {
        return false;
    }
    
    digT1 = static_cast<uint16_t>(Wire.read() | (Wire.read() << 8));
    digT2 = static_cast<int16_t>(Wire.read() | (Wire.read() << 8));
    digT3 = static_cast<int16_t>(Wire.read() | (Wire.read() << 8));
    digP1 = static_cast<uint16_t>(Wire.read() | (Wire.read() << 8));
    digP2 = static_cast<int16_t>(Wire.read() | (Wire.read() << 8));
    digP3 = Wire.read();
    digP4 = Wire.read();
    digP5 = Wire.read();
    digP6 = Wire.read();
    digP7 = Wire.read();
    digP8 = Wire.read();
    digP9 = Wire.read();
    for (uint8_t i = 0; i < BME_RAW_DATA_SKIP_BYTE_COUNT; i++) {
        Wire.read();
    }
    
    Wire.beginTransmission(i2cAddress);
    Wire.write(BME_REGISTER_CALIBRATION_HUMIDITY);
    (void)Wire.endTransmission(false);
    
    Wire.requestFrom(i2cAddress, BME_CALIBRATION_HUMIDITY_BYTE_COUNT);
    if (Wire.available() < BME_CALIBRATION_HUMIDITY_BYTE_COUNT) {
        return false;
    }
    
    digH1 = Wire.read();
    digH2 = static_cast<int16_t>(Wire.read() | (Wire.read() << 8));
    digH3 = Wire.read();
    digH4 = (Wire.read() << 4) | (Wire.read() & 0x0F);
    digH5 = (Wire.read() << 4) | (Wire.read() >> 4);
    digH6 = Wire.read();
    
    return true;
}

bool BmeEnvironmentalSensor::readRawData(int32_t& rawTemperature, int32_t& rawPressure,
                                         uint16_t& rawHumidity) const {
    Wire.beginTransmission(i2cAddress);
    Wire.write(BME_REGISTER_DATA);
    (void)Wire.endTransmission(false);
    
    Wire.requestFrom(i2cAddress, BME_RAW_DATA_BYTE_COUNT);
    if (Wire.available() < BME_RAW_DATA_BYTE_COUNT) {
        return false;
    }
    
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();
    uint8_t xlsb = Wire.read();
    rawTemperature = (static_cast<int32_t>(msb) << BME_RAW_DATA_SHIFT_MSB) |
                     (static_cast<int32_t>(lsb) << BME_RAW_DATA_SHIFT_LSB) |
                     (xlsb >> BME_RAW_DATA_SHIFT_LSB);
    
    msb = Wire.read();
    lsb = Wire.read();
    xlsb = Wire.read();
    rawPressure = (static_cast<int32_t>(msb) << BME_RAW_DATA_SHIFT_MSB) |
                  (static_cast<int32_t>(lsb) << BME_RAW_DATA_SHIFT_LSB) |
                  (xlsb >> BME_RAW_DATA_SHIFT_LSB);
    
    msb = Wire.read();
    lsb = Wire.read();
    rawHumidity = static_cast<uint16_t>((msb << 8) | lsb);
    
    return true;
}

float BmeEnvironmentalSensor::compensateTemperature(const int32_t rawTemperature) {
    const float var1 = static_cast<float>(rawTemperature) / BME_TEMP_COMPENSATION_SCALE_14 -
                       static_cast<float>(digT1) / BME_TEMP_COMPENSATION_SCALE_10;
    const float var2 = var1 * static_cast<float>(digT2);
    float var3 = (var1 * var1) * static_cast<float>(digT3);
    var3 = var3 + (var2 * BME_TEMP_COMPENSATION_SCALE_14) / BME_TEMP_COMPENSATION_SCALE_10;
    var3 = (var3 + BME_TEMP_COMPENSATION_OFFSET) / BME_TEMP_COMPENSATION_SCALE_16;
    
    // Update tFine for pressure compensation (5 Q8 format)
    tFine = static_cast<int32_t>(var3 * BME_TEMP_TFINE_SCALE_8);
    
    return var3;
}

float BmeEnvironmentalSensor::compensatePressure(const int32_t rawPressure) const {
    float var1 = static_cast<float>(tFine) / BME_PRESS_COMPENSATION_DIVISOR_2 -
                 BME_PRESS_COMPENSATION_OFFSET;
    float var2 = var1 * var1 * static_cast<float>(digP6) / BME_PRESS_COMPENSATION_SCALE_15;
    var2 = var2 + var1 * static_cast<float>(digP5) * BME_PRESS_COMPENSATION_DIVISOR_2;
    var2 = var2 / BME_PRESS_COMPENSATION_DIVISOR_4 +
           static_cast<float>(digP4) * BME_TEMP_COMPENSATION_SCALE_16;
    
    var1 = (static_cast<float>(digP3) * var1 * var1 / BME_PRESS_COMPENSATION_SCALE_17 +
            static_cast<float>(digP2) * var1) / BME_PRESS_COMPENSATION_SCALE_19;
    var1 = (1.0f + var1 / BME_PRESS_COMPENSATION_SCALE_15) * static_cast<float>(digP1);
    
    if (var1 == 0.0f) {
        return 0.0f;
    }
    
    float pressure = BME_PRESS_RAW_OFFSET - static_cast<float>(rawPressure);
    pressure = (pressure - var2 / BME_PRESS_COMPENSATION_SCALE_12) *
               BME_PRESS_SCALING_FACTOR / var1;
    
    float temp1 = static_cast<float>(digP9) * pressure * pressure / BME_PRESS_OVERFLOW_PROTECTION;
    float temp2 = pressure * static_cast<float>(digP8) / BME_PRESS_COMPENSATION_SCALE_15;
    pressure = pressure + (temp1 + temp2 + static_cast<float>(digP7)) / BME_PRESS_DIVISOR_16;
    
    return pressure / BME_PRESS_TO_HECTOPASCALS;
}

float BmeEnvironmentalSensor::compensateHumidity(const uint16_t rawHumidity) const {
    float humidity = static_cast<float>(tFine) - BME_HUMID_COMPENSATION_OFFSET;
    humidity = humidity + (static_cast<float>(rawHumidity) -
                          (static_cast<float>(digH4) / BME_HUMID_COMPENSATION_SCALE_6 +
                           static_cast<float>(digH5) * humidity / BME_HUMID_COMPENSATION_SCALE_14));
    humidity = humidity * (static_cast<float>(digH2) / BME_HUMID_COMPENSATION_SCALE_16 *
                          (1.0f + static_cast<float>(digH6) * humidity / BME_HUMID_COMPENSATION_SCALE_26 *
                           (1.0f + static_cast<float>(digH3) * humidity / BME_HUMID_COMPENSATION_SCALE_26)));
    humidity = humidity * (1.0f - static_cast<float>(digH1) * humidity / BME_HUMID_COMPENSATION_SCALE_19);
    
    if (humidity > BME_HUMIDITY_MAX_PERCENTAGE) {
        humidity = BME_HUMIDITY_MAX_PERCENTAGE;
    } else if (humidity < BME_HUMIDITY_MIN_PERCENTAGE) {
        humidity = BME_HUMIDITY_MIN_PERCENTAGE;
    }
    
    return humidity;
}
