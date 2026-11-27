/**
 * @file BmeEnvironmentalSensor.cpp
 * @brief Implements the abstract base class for Bosch BME280/BME680 environmental sensors.
 * @ingroup sensors_module
 * @copyright (c) 2025-2026 Angel Velasquez
 *
 * @author Angel Velasquez
 * @date 2026-11-10
 * @version 1.0
 */

#include "BmeEnvironmentalSensor.h"
#include <cmath>

namespace {
I2cStatus mapWireStatus(const uint8_t status) {
    switch (status) {
        case 0: return I2cStatus::Ok;
        case 1: return I2cStatus::InvalidArgument;
        case 2: return I2cStatus::AddressNack;
        case 3: return I2cStatus::DataNack;
        case 5: return I2cStatus::Timeout;
        default: return I2cStatus::BusError;
    }
}
}

BmeEnvironmentalSensor::~BmeEnvironmentalSensor() = default;

BmeEnvironmentalSensor::BmeEnvironmentalSensor(const int signalGpioPin, const uint8_t i2cAddress,
                                               EventHandler* parentHandler)
    : BmeEnvironmentalSensor(nullptr, signalGpioPin, i2cAddress, parentHandler)
{
    Wire.begin();
}

BmeEnvironmentalSensor::BmeEnvironmentalSensor(I2cBus& bus, const int signalGpioPin,
                                               const uint8_t i2cAddress,
                                               EventHandler* parentHandler)
    : BmeEnvironmentalSensor(&bus, signalGpioPin, i2cAddress, parentHandler) {}

BmeEnvironmentalSensor::BmeEnvironmentalSensor(I2cBus* bus, const int signalGpioPin,
                                               const uint8_t i2cAddress,
                                               EventHandler* parentHandler)
    : Sensor(signalGpioPin, parentHandler)
    , bus(bus)
    , lastI2cResult{I2cStatus::Ok, 0, 0}
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
{}

bool BmeEnvironmentalSensor::isInitialized() const {
    return initialized;
}

I2cResult BmeEnvironmentalSensor::getLastI2cResult() const {
    return lastI2cResult;
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
    uint8_t bytes[2]{};
    if (readBytes(registerAddress, bytes, sizeof(bytes)).isInvalidOrIncomplete()) {
        return 0;
    }
    return static_cast<int16_t>((static_cast<uint16_t>(bytes[1]) << 8) | bytes[0]);
}

bool BmeEnvironmentalSensor::readRegister8(const uint8_t registerAddress, uint8_t& value) const {
    uint8_t byte = 0;
    if (readBytes(registerAddress, &byte, 1).isInvalidOrIncomplete()) {
        return false;
    }
    value = byte;
    return true;
}

I2cResult BmeEnvironmentalSensor::readBytes(const uint8_t registerAddress, uint8_t* bytes,
                                            const std::size_t length) const {
    if (bus != nullptr) {
        lastI2cResult = bus->writeRead(i2cAddress, &registerAddress, 1, bytes, length);
        return lastI2cResult;
    }

    if (bytes == nullptr || length == 0) {
        lastI2cResult = {I2cStatus::InvalidArgument, 0, length};
        return lastI2cResult;
    }

    Wire.beginTransmission(i2cAddress);
    const std::size_t written = Wire.write(&registerAddress, 1);
    const I2cStatus txStatus = written == 1 ? mapWireStatus(Wire.endTransmission(false))
                                             : I2cStatus::InvalidArgument;
    if (txStatus != I2cStatus::Ok) {
        lastI2cResult = {txStatus, 0, length};
        return lastI2cResult;
    }

    const std::size_t reported = Wire.requestFrom(i2cAddress, length);
    const std::size_t available = reported < length ? reported : length;
    std::size_t received = 0;
    while (received < available && Wire.available() > 0) {
        const int value = Wire.read();
        if (value < 0) {
            break;
        }
        bytes[received++] = static_cast<uint8_t>(value);
    }
    lastI2cResult = {reported == length && received == length ? I2cStatus::Ok : I2cStatus::ShortRead,
                     received, length};
    return lastI2cResult;
}

I2cResult BmeEnvironmentalSensor::writeBytes(const uint8_t* bytes, const std::size_t length) const {
    if (bus != nullptr) {
        lastI2cResult = bus->write(i2cAddress, bytes, length);
        return lastI2cResult;
    }
    if (bytes == nullptr || length == 0) {
        lastI2cResult = {I2cStatus::InvalidArgument, 0, length};
        return lastI2cResult;
    }

    Wire.beginTransmission(i2cAddress);
    const std::size_t written = Wire.write(bytes, length);
    const I2cStatus status = written == length ? mapWireStatus(Wire.endTransmission())
                                                : I2cStatus::InvalidArgument;
    lastI2cResult = {status, status == I2cStatus::Ok ? length : 0, length};
    return lastI2cResult;
}

bool BmeEnvironmentalSensor::readCalibrationData() {
    uint8_t calibration[BME_CALIBRATION_TEMP_PRESSURE_BYTE_COUNT]{};
    if (readBytes(BME_REGISTER_CALIBRATION_START, calibration, sizeof(calibration)).isInvalidOrIncomplete()) {
        return false;
    }
    uint8_t humidity[BME_CALIBRATION_HUMIDITY_BYTE_COUNT]{};
    if (readBytes(BME_REGISTER_CALIBRATION_HUMIDITY, humidity, sizeof(humidity)).isInvalidOrIncomplete()) {
        return false;
    }

    digT1 = static_cast<uint16_t>(calibration[0] | (static_cast<uint16_t>(calibration[1]) << 8));
    digT2 = static_cast<int16_t>(calibration[2] | (static_cast<uint16_t>(calibration[3]) << 8));
    digT3 = static_cast<int16_t>(calibration[4] | (static_cast<uint16_t>(calibration[5]) << 8));
    digP1 = static_cast<uint16_t>(calibration[6] | (static_cast<uint16_t>(calibration[7]) << 8));
    digP2 = static_cast<int16_t>(calibration[8] | (static_cast<uint16_t>(calibration[9]) << 8));
    digP3 = calibration[10];
    digP4 = calibration[11];
    digP5 = calibration[12];
    digP6 = calibration[13];
    digP7 = calibration[14];
    digP8 = calibration[15];
    digP9 = calibration[16];

    digH1 = humidity[0];
    digH2 = static_cast<int16_t>(humidity[1] | (static_cast<uint16_t>(humidity[2]) << 8));
    digH3 = humidity[3];
    digH4 = (humidity[4] << 4) | (humidity[5] & 0x0F);
    digH5 = (humidity[5] << 4) | (humidity[6] >> 4);
    digH6 = humidity[6];

    return true;
}

bool BmeEnvironmentalSensor::readRawData(int32_t& rawTemperature, int32_t& rawPressure,
                                         uint16_t& rawHumidity) const {
    uint8_t data[BME_RAW_DATA_BYTE_COUNT]{};
    if (readBytes(BME_REGISTER_DATA, data, sizeof(data)).isInvalidOrIncomplete()) {
        return false;
    }

    uint8_t msb = data[0];
    uint8_t lsb = data[1];
    uint8_t xlsb = data[2];
    rawTemperature = (static_cast<int32_t>(msb) << BME_RAW_DATA_SHIFT_MSB) |
                     (static_cast<int32_t>(lsb) << BME_RAW_DATA_SHIFT_LSB) |
                     (xlsb >> BME_RAW_DATA_SHIFT_LSB);

    msb = data[3];
    lsb = data[4];
    xlsb = data[5];
    rawPressure = (static_cast<int32_t>(msb) << BME_RAW_DATA_SHIFT_MSB) |
                  (static_cast<int32_t>(lsb) << BME_RAW_DATA_SHIFT_LSB) |
                  (xlsb >> BME_RAW_DATA_SHIFT_LSB);

    msb = data[6];
    lsb = data[7];
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
