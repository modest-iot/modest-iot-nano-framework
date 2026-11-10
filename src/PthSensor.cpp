/**
 * @file PthSensor.cpp
 * @brief Implements the BME280-compatible pressure, temperature, and humidity sensor adapter.
 * @ingroup sensors_module
 *
 * @author Angel Velasquez
 * @date 2026-11-10
 * @version 1.0
 */

#include "PthSensor.h"
#include <cmath>

PthSensor::PthSensor(const int signalGpioPin, const uint8_t i2cAddress, EventHandler* parentHandler)
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
    Wire.write(BME280_REGISTER_TEMP_PRESS_X4);
    Wire.write((BME280_OSRS_TEMP_16 << 5) | (BME280_OSRS_PRESS_16 << 2) | BME280_MODE_NORMAL);
    Wire.write((BME280_OSRS_HUMID_16 << 4) | (BME280_STANDBY_0_5_MS << 1) | BME280_FILTER_COEFF_16);
    (void)Wire.endTransmission();
    
    initialized = true;
    return true;
}

bool PthSensor::isInitialized() const {
    return initialized;
}

float PthSensor::getTemperatureInCelsius() const {
    return cachedTemperatureInCelsius;
}

float PthSensor::getHumidityInPercentage() const {
    return cachedHumidityInPercentage;
}

float PthSensor::getPressureInHectopascals() const {
    return cachedPressureInHectopascals;
}

float PthSensor::getAltitudeInMeters(const float seaLevelPressureInHectopascals) const {
    if (seaLevelPressureInHectopascals <= 0.0f) {
        return 0.0f;
    }
    if (cachedPressureInHectopascals <= 0.0f) {
        return 0.0f;
    }
    const float pressureRatio = cachedPressureInHectopascals / seaLevelPressureInHectopascals;
    return ALTITUDE_SCALE_HEIGHT_IN_METERS * (1.0f - std::powf(pressureRatio, ALTITUDE_BAROMETRIC_EXPONENT));
}

int16_t PthSensor::readRegister16(uint8_t registerAddress) const {
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

bool PthSensor::readCalibrationData() {
    Wire.beginTransmission(i2cAddress);
    Wire.write(BME280_REGISTER_CALIBRATION_START);
    (void)Wire.endTransmission(false);
    
    Wire.requestFrom(i2cAddress, BME280_CALIBRATION_TEMP_PRESSURE_BYTE_COUNT);
    if (Wire.available() < BME280_CALIBRATION_TEMP_PRESSURE_BYTE_COUNT) {
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
    for (uint8_t i = 0; i < BME280_RAW_DATA_SKIP_BYTE_COUNT; i++) {
        Wire.read();
    }
    
    Wire.beginTransmission(i2cAddress);
    Wire.write(BME280_REGISTER_CALIBRATION_HUMIDITY);
    (void)Wire.endTransmission(false);
    
    Wire.requestFrom(i2cAddress, BME280_CALIBRATION_HUMIDITY_BYTE_COUNT);
    if (Wire.available() < BME280_CALIBRATION_HUMIDITY_BYTE_COUNT) {
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

bool PthSensor::readRawData(int32_t& rawTemperature, int32_t& rawPressure, uint16_t& rawHumidity) const {
    Wire.beginTransmission(i2cAddress);
    Wire.write(BME280_REGISTER_DATA);
    (void)Wire.endTransmission(false);
    
    Wire.requestFrom(i2cAddress, BME280_RAW_DATA_BYTE_COUNT);
    if (Wire.available() < BME280_RAW_DATA_BYTE_COUNT) {
        return false;
    }
    
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();
    uint8_t xlsb = Wire.read();
    rawTemperature = (static_cast<int32_t>(msb) << BME280_RAW_DATA_SHIFT_MSB) | (static_cast<int32_t>(lsb) << BME280_RAW_DATA_SHIFT_LSB) | (xlsb >> BME280_RAW_DATA_SHIFT_LSB);
    
    msb = Wire.read();
    lsb = Wire.read();
    xlsb = Wire.read();
    rawPressure = (static_cast<int32_t>(msb) << BME280_RAW_DATA_SHIFT_MSB) | (static_cast<int32_t>(lsb) << BME280_RAW_DATA_SHIFT_LSB) | (xlsb >> BME280_RAW_DATA_SHIFT_LSB);
    
    msb = Wire.read();
    lsb = Wire.read();
    rawHumidity = static_cast<uint16_t>((msb << 8) | lsb);
    
    return true;
}

float PthSensor::compensateTemperature(const int32_t rawTemperature) {
    const float var1 = static_cast<float>(rawTemperature) / BME280_TEMP_COMPENSATION_SCALE_14 - static_cast<float>(digT1) / BME280_TEMP_COMPENSATION_SCALE_10;
    const float var2 = var1 * static_cast<float>(digT2);
    float var3 = (var1 * var1) * static_cast<float>(digT3);
    var3 = var3 + (var2 * BME280_TEMP_COMPENSATION_SCALE_14) / BME280_TEMP_COMPENSATION_SCALE_10;
    var3 = (var3 + BME280_TEMP_COMPENSATION_OFFSET) / BME280_TEMP_COMPENSATION_SCALE_16;
    
    // Update tFine for pressure compensation (5 Q8 format)
    tFine = static_cast<int32_t>(var3 * BME280_TEMP_TFINE_SCALE_8);
    
    return var3;
}

float PthSensor::compensatePressure(const int32_t rawPressure) const {
    float var1 = static_cast<float>(tFine) / BME280_PRESS_COMPENSATION_DIVISOR_2 - BME280_PRESS_COMPENSATION_OFFSET;
    float var2 = var1 * var1 * static_cast<float>(digP6) / BME280_PRESS_COMPENSATION_SCALE_15;
    var2 = var2 + var1 * static_cast<float>(digP5) * BME280_PRESS_COMPENSATION_DIVISOR_2;
    var2 = var2 / BME280_PRESS_COMPENSATION_DIVISOR_4 + static_cast<float>(digP4) * BME280_TEMP_COMPENSATION_SCALE_16;
    
    var1 = (static_cast<float>(digP3) * var1 * var1 / BME280_PRESS_COMPENSATION_SCALE_17 + static_cast<float>(digP2) * var1) / BME280_PRESS_COMPENSATION_SCALE_19;
    var1 = (1.0f + var1 / BME280_PRESS_COMPENSATION_SCALE_15) * static_cast<float>(digP1);
    
    if (var1 == 0.0f) {
        return 0.0f;
    }
    
    float pressure = BME280_PRESS_RAW_OFFSET - static_cast<float>(rawPressure);
    pressure = (pressure - var2 / BME280_PRESS_COMPENSATION_SCALE_12) * BME280_PRESS_SCALING_FACTOR / var1;
    
    float temp1 = static_cast<float>(digP9) * pressure * pressure / BME280_PRESS_OVERFLOW_PROTECTION;
    float temp2 = pressure * static_cast<float>(digP8) / BME280_PRESS_COMPENSATION_SCALE_15;
    pressure = pressure + (temp1 + temp2 + static_cast<float>(digP7)) / BME280_PRESS_DIVISOR_16;
    
    return pressure / BME280_PRESS_TO_HECTOPASCALS;
}

float PthSensor::compensateHumidity(const uint16_t rawHumidity) const {
    float humidity = static_cast<float>(tFine) - BME280_HUMID_COMPENSATION_OFFSET;
    humidity = humidity + (static_cast<float>(rawHumidity) - (static_cast<float>(digH4) / BME280_HUMID_COMPENSATION_SCALE_6 + static_cast<float>(digH5) * humidity / BME280_HUMID_COMPENSATION_SCALE_14));
    humidity = humidity * (static_cast<float>(digH2) / BME280_HUMID_COMPENSATION_SCALE_16 * (1.0f + static_cast<float>(digH6) * humidity / BME280_HUMID_COMPENSATION_SCALE_26 * (1.0f + static_cast<float>(digH3) * humidity / BME280_HUMID_COMPENSATION_SCALE_26)));
    humidity = humidity * (1.0f - static_cast<float>(digH1) * humidity / BME280_HUMID_COMPENSATION_SCALE_19);
    
    if (humidity > BME280_HUMIDITY_MAX_PERCENTAGE) {
        humidity = BME280_HUMIDITY_MAX_PERCENTAGE;
    } else if (humidity < BME280_HUMIDITY_MIN_PERCENTAGE) {
        humidity = BME280_HUMIDITY_MIN_PERCENTAGE;
    }
    
    return humidity;
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
