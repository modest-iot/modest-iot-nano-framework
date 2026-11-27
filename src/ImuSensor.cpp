/**
 * @file ImuSensor.cpp
 * @brief Implements MPU6050 IMU sampling with I²C communication and compensation.
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-23
 * @version 1.0
 */

#include "ImuSensor.h"
#include <Wire.h>
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

ImuSensor::ImuSensor(const int signalGpioPin, const uint8_t i2cAddress,
                     EventHandler* parentHandler)
    : ImuSensor(signalGpioPin, i2cAddress, IMU_ACCEL_FS_2G, IMU_GYRO_FS_250, parentHandler) {}

ImuSensor::ImuSensor(const int signalGpioPin, const uint8_t i2cAddress,
                      const uint8_t accelerationFullScale,
                      const uint8_t gyroscopeFullScale,
                      EventHandler* parentHandler)
    : Sensor(signalGpioPin, parentHandler)
    , bus(nullptr)
    , lastI2cResult{I2cStatus::Ok, 0, 0}
    , i2cAddress(i2cAddress)
    , accelerationOffsetX(0)
    , accelerationOffsetY(0)
    , accelerationOffsetZ(0)
    , gyroscopeOffsetX(0)
    , gyroscopeOffsetY(0)
    , gyroscopeOffsetZ(0)
    , cachedAccelerationXInGForce(INITIAL_CACHED_ACCELERATION_IN_GFORCE)
    , cachedAccelerationYInGForce(INITIAL_CACHED_ACCELERATION_IN_GFORCE)
    , cachedAccelerationZInGForce(INITIAL_CACHED_ACCELERATION_IN_GFORCE)
    , cachedGyroscopeXInDegreesPerSecond(INITIAL_CACHED_GYROSCOPE_IN_DEGREES_PER_SECOND)
    , cachedGyroscopeYInDegreesPerSecond(INITIAL_CACHED_GYROSCOPE_IN_DEGREES_PER_SECOND)
    , cachedGyroscopeZInDegreesPerSecond(INITIAL_CACHED_GYROSCOPE_IN_DEGREES_PER_SECOND)
    , cachedInternalTemperatureInCelsius(INITIAL_CACHED_TEMPERATURE_IN_CELSIUS)
    , sampleBufferX{}
    , sampleBufferY{}
    , sampleBufferZ{}
    , sampleWindowIndex(0)
    , sampleWindowCount(0)
    , sampleWindowSize(DEFAULT_SAMPLE_WINDOW_SIZE)
    , lastNotifiedAccelerationXInGForce(INITIAL_LAST_NOTIFIED_ACCELERATION_IN_GFORCE)
    , lastNotifiedAccelerationYInGForce(INITIAL_LAST_NOTIFIED_ACCELERATION_IN_GFORCE)
    , lastNotifiedAccelerationZInGForce(INITIAL_LAST_NOTIFIED_ACCELERATION_IN_GFORCE)
    , lastNotifiedGyroscopeXInDegreesPerSecond(INITIAL_LAST_NOTIFIED_GYROSCOPE_IN_DEGREES_PER_SECOND)
    , lastNotifiedGyroscopeYInDegreesPerSecond(INITIAL_LAST_NOTIFIED_GYROSCOPE_IN_DEGREES_PER_SECOND)
    , lastNotifiedGyroscopeZInDegreesPerSecond(INITIAL_LAST_NOTIFIED_GYROSCOPE_IN_DEGREES_PER_SECOND)
    , initialized(false)
    , accelerationFullScale(accelerationFullScale)
    , gyroscopeFullScale(gyroscopeFullScale)
{
    Wire.begin();
}

ImuSensor::ImuSensor(I2cBus& bus, const int signalGpioPin, const uint8_t i2cAddress,
                     const uint8_t accelerationFullScale, const uint8_t gyroscopeFullScale,
                     EventHandler* parentHandler)
    : Sensor(signalGpioPin, parentHandler)
    , bus(&bus)
    , lastI2cResult{I2cStatus::Ok, 0, 0}
    , i2cAddress(i2cAddress)
    , accelerationOffsetX(0)
    , accelerationOffsetY(0)
    , accelerationOffsetZ(0)
    , gyroscopeOffsetX(0)
    , gyroscopeOffsetY(0)
    , gyroscopeOffsetZ(0)
    , cachedAccelerationXInGForce(INITIAL_CACHED_ACCELERATION_IN_GFORCE)
    , cachedAccelerationYInGForce(INITIAL_CACHED_ACCELERATION_IN_GFORCE)
    , cachedAccelerationZInGForce(INITIAL_CACHED_ACCELERATION_IN_GFORCE)
    , cachedGyroscopeXInDegreesPerSecond(INITIAL_CACHED_GYROSCOPE_IN_DEGREES_PER_SECOND)
    , cachedGyroscopeYInDegreesPerSecond(INITIAL_CACHED_GYROSCOPE_IN_DEGREES_PER_SECOND)
    , cachedGyroscopeZInDegreesPerSecond(INITIAL_CACHED_GYROSCOPE_IN_DEGREES_PER_SECOND)
    , cachedInternalTemperatureInCelsius(INITIAL_CACHED_TEMPERATURE_IN_CELSIUS)
    , sampleBufferX{}
    , sampleBufferY{}
    , sampleBufferZ{}
    , sampleWindowIndex(0)
    , sampleWindowCount(0)
    , sampleWindowSize(DEFAULT_SAMPLE_WINDOW_SIZE)
    , lastNotifiedAccelerationXInGForce(INITIAL_LAST_NOTIFIED_ACCELERATION_IN_GFORCE)
    , lastNotifiedAccelerationYInGForce(INITIAL_LAST_NOTIFIED_ACCELERATION_IN_GFORCE)
    , lastNotifiedAccelerationZInGForce(INITIAL_LAST_NOTIFIED_ACCELERATION_IN_GFORCE)
    , lastNotifiedGyroscopeXInDegreesPerSecond(INITIAL_LAST_NOTIFIED_GYROSCOPE_IN_DEGREES_PER_SECOND)
    , lastNotifiedGyroscopeYInDegreesPerSecond(INITIAL_LAST_NOTIFIED_GYROSCOPE_IN_DEGREES_PER_SECOND)
    , lastNotifiedGyroscopeZInDegreesPerSecond(INITIAL_LAST_NOTIFIED_GYROSCOPE_IN_DEGREES_PER_SECOND)
    , initialized(false)
    , accelerationFullScale(accelerationFullScale)
    , gyroscopeFullScale(gyroscopeFullScale) {}

bool ImuSensor::writeRegister(const uint8_t registerAddress, const uint8_t data) const {
    if (bus != nullptr) {
        const uint8_t bytes[] = {registerAddress, data};
        lastI2cResult = bus->write(i2cAddress, bytes, sizeof(bytes));
        return !lastI2cResult.isInvalidOrIncomplete();
    }

    Wire.beginTransmission(i2cAddress);
    const std::size_t writtenRegister = Wire.write(registerAddress);
    const std::size_t writtenData = Wire.write(data);
    const uint8_t wireStatus = Wire.endTransmission();
    const I2cStatus status = writtenRegister == 1 && writtenData == 1
                                 ? mapWireStatus(wireStatus)
                                 : I2cStatus::InvalidArgument;
    lastI2cResult = {status, status == I2cStatus::Ok ? 2U : 0U, 2};
    return status == I2cStatus::Ok;
}

uint8_t ImuSensor::readRegister(const uint8_t registerAddress) const {
    uint8_t value = 0;
    (void)readRegisterChecked(registerAddress, value);
    return value;
}

bool ImuSensor::readRegisterChecked(const uint8_t registerAddress, uint8_t& value) const {
    if (bus != nullptr) {
        uint8_t received = 0;
        lastI2cResult = bus->writeRead(i2cAddress, &registerAddress, 1, &received, 1);
        if (lastI2cResult.isInvalidOrIncomplete()) {
            return false;
        }
        value = received;
        return true;
    }

    Wire.beginTransmission(i2cAddress);
    const std::size_t written = Wire.write(registerAddress);
    const uint8_t wireStatus = Wire.endTransmission(false);
    if (written != 1 || wireStatus != 0) {
        const I2cStatus status = written == 1 ? mapWireStatus(wireStatus) : I2cStatus::InvalidArgument;
        lastI2cResult = {status, 0, 1};
        return false;
    }

    if (Wire.requestFrom(i2cAddress, static_cast<uint8_t>(1)) != 1) {
        lastI2cResult = {I2cStatus::ShortRead, 0, 1};
        return false;
    }

    const int received = Wire.read();
    if (received < 0) {
        lastI2cResult = {I2cStatus::ShortRead, 0, 1};
        return false;
    }
    value = static_cast<uint8_t>(received);
    lastI2cResult = {I2cStatus::Ok, 1, 1};
    return true;
}

bool ImuSensor::readCalibrationData() {
    uint8_t gyroX = 0;
    uint8_t gyroY = 0;
    uint8_t gyroZ = 0;
    uint8_t accelX = 0;
    if (!readRegisterChecked(IMU_REGISTER_SELF_TEST_X, gyroX) ||
        !readRegisterChecked(IMU_REGISTER_SELF_TEST_Y, gyroY) ||
        !readRegisterChecked(IMU_REGISTER_SELF_TEST_Z, gyroZ) ||
        !readRegisterChecked(IMU_REGISTER_SELF_TEST_A, accelX)) {
        return false;
    }

    // Read gyroscope factory trim offsets (registers 0x0D-0x0F)
    const int16_t newGyroscopeOffsetX = static_cast<int16_t>(gyroX << 3);
    const int16_t newGyroscopeOffsetY = static_cast<int16_t>(gyroY << 3);
    const int16_t newGyroscopeOffsetZ = static_cast<int16_t>(gyroZ << 3);

    // Read accelerometer factory trim offset (register 0x10)
    const int16_t newAccelerationOffsetX = static_cast<int16_t>(accelX);
    gyroscopeOffsetX = newGyroscopeOffsetX;
    gyroscopeOffsetY = newGyroscopeOffsetY;
    gyroscopeOffsetZ = newGyroscopeOffsetZ;
    accelerationOffsetX = newAccelerationOffsetX;
    accelerationOffsetY = 0; // MPU6050 does not expose Y/Z acceleration trim separately
    accelerationOffsetZ = 0;

    return true;
}

bool ImuSensor::readRawData(int16_t& rawAccelerationX, int16_t& rawAccelerationY, int16_t& rawAccelerationZ,
                                        int16_t& rawTemperature, int16_t& rawGyroscopeX, int16_t& rawGyroscopeY,
                                        int16_t& rawGyroscopeZ) const {
    uint8_t buffer[IMU_RAW_DATA_BYTE_COUNT]{};
    if (bus != nullptr) {
        const uint8_t registerAddress = IMU_REGISTER_ACCEL_XOUT_H;
        lastI2cResult = bus->writeRead(i2cAddress, &registerAddress, 1, buffer, sizeof(buffer));
        if (lastI2cResult.isInvalidOrIncomplete()) {
            return false;
        }
    } else {
        Wire.beginTransmission(i2cAddress);
        const std::size_t written = Wire.write(IMU_REGISTER_ACCEL_XOUT_H);
        const uint8_t wireStatus = Wire.endTransmission(false);
        if (written != 1 || wireStatus != 0) {
            const I2cStatus status = written == 1 ? mapWireStatus(wireStatus) : I2cStatus::InvalidArgument;
            lastI2cResult = {status, 0, IMU_RAW_DATA_BYTE_COUNT};
            return false;
        }

        if (Wire.requestFrom(i2cAddress, IMU_RAW_DATA_BYTE_COUNT) != IMU_RAW_DATA_BYTE_COUNT) {
            lastI2cResult = {I2cStatus::ShortRead, 0, IMU_RAW_DATA_BYTE_COUNT};
            return false;
        }

        for (uint8_t i = 0; i < IMU_RAW_DATA_BYTE_COUNT; i++) {
            const int received = Wire.read();
            if (received < 0) {
                lastI2cResult = {I2cStatus::ShortRead, i, IMU_RAW_DATA_BYTE_COUNT};
                return false;
            }
            buffer[i] = static_cast<uint8_t>(received);
        }
        lastI2cResult = {I2cStatus::Ok, IMU_RAW_DATA_BYTE_COUNT, IMU_RAW_DATA_BYTE_COUNT};
    }

    if (lastI2cResult.isInvalidOrIncomplete()) {
        return false;
    }

    // Accelerometer (bytes 0-5)
    rawAccelerationX = static_cast<int16_t>((buffer[0] << 8) | buffer[1]);
    rawAccelerationY = static_cast<int16_t>((buffer[2] << 8) | buffer[3]);
    rawAccelerationZ = static_cast<int16_t>((buffer[4] << 8) | buffer[5]);

    // Temperature (bytes 6-7)
    rawTemperature = static_cast<int16_t>((buffer[6] << 8) | buffer[7]);

    // Gyroscope (bytes 8-13)
    rawGyroscopeX = static_cast<int16_t>((buffer[8] << 8) | buffer[9]);
    rawGyroscopeY = static_cast<int16_t>((buffer[10] << 8) | buffer[11]);
    rawGyroscopeZ = static_cast<int16_t>((buffer[12] << 8) | buffer[13]);

    return true;
}

float ImuSensor::compensateAcceleration(const int16_t rawAcceleration) const {
    const int16_t lsbPerGForce = getAccelerationLsbPerGForce();
    return static_cast<float>(rawAcceleration) / static_cast<float>(lsbPerGForce);
}

float ImuSensor::compensateGyroscope(const int16_t rawGyroscope) const {
    const float degreesPerSecondPerLsb = getGyroscopeDegreesPerSecondPerLsb();
    return static_cast<float>(rawGyroscope) / degreesPerSecondPerLsb;
}

float ImuSensor::compensateTemperature(const int16_t rawTemp) {
    return static_cast<float>(rawTemp) / IMU_TEMP_SENSITIVITY + IMU_TEMP_OFFSET;
}

int16_t ImuSensor::getAccelerationLsbPerGForce() const {
    switch (accelerationFullScale) {
        case IMU_ACCEL_FS_4G:
            return IMU_ACCEL_LSB_PER_G_4G;
        case IMU_ACCEL_FS_8G:
            return IMU_ACCEL_LSB_PER_G_8G;
        case IMU_ACCEL_FS_16G:
            return IMU_ACCEL_LSB_PER_G_16G;
        default:
            return IMU_ACCEL_LSB_PER_G_2G;
    }
}

float ImuSensor::getGyroscopeDegreesPerSecondPerLsb() const {
    switch (gyroscopeFullScale) {
        case IMU_GYRO_FS_500:
            return IMU_GYRO_DPS_PER_LSB_500;
        case IMU_GYRO_FS_1000:
            return IMU_GYRO_DPS_PER_LSB_1000;
        case IMU_GYRO_FS_2000:
            return IMU_GYRO_DPS_PER_LSB_2000;
        default:
            return IMU_GYRO_DPS_PER_LSB_250;
    }
}

void ImuSensor::processEvent(Event& event) {
    if (event.identifier == MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        int16_t rawAccelerationX, rawAccelerationY, rawAccelerationZ;
        int16_t rawTemperature, rawGyroscopeX, rawGyroscopeY, rawGyroscopeZ;

        if (!readRawData(rawAccelerationX, rawAccelerationY, rawAccelerationZ, rawTemperature,
                         rawGyroscopeX, rawGyroscopeY, rawGyroscopeZ)) {
            reportReadFailure(event);
            if (event.identifier != SENSOR_FAULT_EVENT_IDENTIFIER) {
                event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
            }
            return;
        }

        const float newAccelerationX = compensateAcceleration(rawAccelerationX);
        const float newAccelerationY = compensateAcceleration(rawAccelerationY);
        const float newAccelerationZ = compensateAcceleration(rawAccelerationZ);
        const float newTemperature = compensateTemperature(rawTemperature);
        const float newGyroscopeX = compensateGyroscope(rawGyroscopeX);
        const float newGyroscopeY = compensateGyroscope(rawGyroscopeY);
        const float newGyroscopeZ = compensateGyroscope(rawGyroscopeZ);

        // Suppress propagation if no value changed beyond epsilon thresholds.
        const bool accelerationChanged =
                (std::abs(newAccelerationX - cachedAccelerationXInGForce) > ACCELERATION_CHANGE_EPSILON) ||
                (std::abs(newAccelerationY - cachedAccelerationYInGForce) > ACCELERATION_CHANGE_EPSILON) ||
                (std::abs(newAccelerationZ - cachedAccelerationZInGForce) > ACCELERATION_CHANGE_EPSILON);
        const bool gyroscopeChanged =
                (std::abs(newGyroscopeX - cachedGyroscopeXInDegreesPerSecond) > GYROSCOPE_CHANGE_EPSILON) ||
                (std::abs(newGyroscopeY - cachedGyroscopeYInDegreesPerSecond) > GYROSCOPE_CHANGE_EPSILON) ||
                (std::abs(newGyroscopeZ - cachedGyroscopeZInDegreesPerSecond) > GYROSCOPE_CHANGE_EPSILON);

        if (!accelerationChanged && !gyroscopeChanged) {
            reportReadSuccess(event);
            event.identifier = PROPAGATION_SUPPRESSION_IDENTIFIER;
            return;
        }

        cachedAccelerationXInGForce = newAccelerationX;
        cachedAccelerationYInGForce = newAccelerationY;
        cachedAccelerationZInGForce = newAccelerationZ;
        cachedInternalTemperatureInCelsius = newTemperature;
        cachedGyroscopeXInDegreesPerSecond = newGyroscopeX;
        cachedGyroscopeYInDegreesPerSecond = newGyroscopeY;
        cachedGyroscopeZInDegreesPerSecond = newGyroscopeZ;

        // Append current reading to the circular sample buffer for windowed statistics.
        const int idx = sampleWindowIndex % sampleWindowSize;
        sampleBufferX[idx] = cachedAccelerationXInGForce;
        sampleBufferY[idx] = cachedAccelerationYInGForce;
        sampleBufferZ[idx] = cachedAccelerationZInGForce;
        sampleWindowIndex++;
        if (sampleWindowCount < sampleWindowSize) {
            sampleWindowCount++;
        }

        reportReadSuccess(event);
        event = Event(DATA_READ_EVENT_IDENTIFIER);
    }
}

uint32_t ImuSensor::getDefaultSamplingIntervalInMilliseconds() const {
    return DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

uint32_t ImuSensor::getMinimalSamplingIntervalInMilliseconds() const {
    return MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS;
}

bool ImuSensor::initialize() {
    initialized = false;

    // Wake up the IMU (clear sleep bit)
    if (!writeRegister(IMU_REGISTER_PWR_MGMT_1, 0x00)) {
        return false;
    }
    // Datasheet specifies 30ms gyroscope ZRO settling time from power-on.
    // This delay runs once during setup, not in the periodic sampling loop.
    delay(IMU_WAKE_UP_DELAY_IN_MILLISECONDS);

    // Verify chip ID
    uint8_t chipId = 0;
    if (!readRegisterChecked(IMU_REGISTER_CHIP_ID, chipId) || chipId != IMU_CHIP_ID_INTERNAL) {
        return false;
    }

    // Configure sample rate, DLPF, full-scale ranges, and clock source
    if (!writeRegister(IMU_REGISTER_SMPLRT_DIV, 0x07)) {
        return false;
    }
    if (!writeRegister(IMU_REGISTER_CONFIG, IMU_DLPF_CFG_184HZ)) {
        return false;
    }
    if (!writeRegister(IMU_REGISTER_GYRO_CONFIG, gyroscopeFullScale)) {
        return false;
    }
    if (!writeRegister(IMU_REGISTER_ACCEL_CONFIG, accelerationFullScale)) {
        return false;
    }
    if (!writeRegister(IMU_REGISTER_PWR_MGMT_1, IMU_CLOCK_SRC_PLL)) {
        return false;
    }

    // Read factory calibration data
    if (!readCalibrationData()) {
        return false;
    }

    initialized = true;
    return true;
}

bool ImuSensor::isInitialized() const {
    return initialized;
}

bool ImuSensor::start() {
    return initialize();
}

I2cResult ImuSensor::getLastI2cResult() const {
    return lastI2cResult;
}

float ImuSensor::getAccelerationXInGForce() const {
    return cachedAccelerationXInGForce;
}

float ImuSensor::getAccelerationYInGForce() const {
    return cachedAccelerationYInGForce;
}

float ImuSensor::getAccelerationZInGForce() const {
    return cachedAccelerationZInGForce;
}

float ImuSensor::getGyroscopeXInDegreesPerSecond() const {
    return cachedGyroscopeXInDegreesPerSecond;
}

float ImuSensor::getGyroscopeYInDegreesPerSecond() const {
    return cachedGyroscopeYInDegreesPerSecond;
}

float ImuSensor::getGyroscopeZInDegreesPerSecond() const {
    return cachedGyroscopeZInDegreesPerSecond;
}

float ImuSensor::getInternalTemperatureInCelsius() const {
    return cachedInternalTemperatureInCelsius;
}

float ImuSensor::getAccelerationMagnitudeInGForce() const {
    return std::sqrt(cachedAccelerationXInGForce * cachedAccelerationXInGForce +
                     cachedAccelerationYInGForce * cachedAccelerationYInGForce +
                     cachedAccelerationZInGForce * cachedAccelerationZInGForce);
}

float ImuSensor::getAccelerationRmsInGForce() const {
    return std::sqrt((cachedAccelerationXInGForce * cachedAccelerationXInGForce +
                      cachedAccelerationYInGForce * cachedAccelerationYInGForce +
                      cachedAccelerationZInGForce * cachedAccelerationZInGForce) / 3.0f);
}

void ImuSensor::setSampleWindowSize(const int size) {
    const int clampedSize = (size < 1) ? 1 : (size > MAXIMUM_SAMPLE_WINDOW_SIZE ? MAXIMUM_SAMPLE_WINDOW_SIZE : size);
    sampleWindowSize = clampedSize;
    // Reset buffer state when window size changes to avoid stale data.
    sampleWindowIndex = 0;
    sampleWindowCount = 0;
}

int ImuSensor::getSampleWindowSize() const {
    return sampleWindowSize;
}

int ImuSensor::getSampleCount() const {
    return sampleWindowCount;
}

float ImuSensor::getAccelerationXRmsInGForce() const {
    if (sampleWindowCount == 0) return 0.0f;
    float sumOfSquares = 0.0f;
    for (int i = 0; i < sampleWindowCount; i++) {
        sumOfSquares += sampleBufferX[i] * sampleBufferX[i];
    }
    return std::sqrt(sumOfSquares / sampleWindowCount);
}

float ImuSensor::getAccelerationYRmsInGForce() const {
    if (sampleWindowCount == 0) return 0.0f;
    float sumOfSquares = 0.0f;
    for (int i = 0; i < sampleWindowCount; i++) {
        sumOfSquares += sampleBufferY[i] * sampleBufferY[i];
    }
    return std::sqrt(sumOfSquares / sampleWindowCount);
}

float ImuSensor::getAccelerationZRmsInGForce() const {
    if (sampleWindowCount == 0) return 0.0f;
    float sumOfSquares = 0.0f;
    for (int i = 0; i < sampleWindowCount; i++) {
        sumOfSquares += sampleBufferZ[i] * sampleBufferZ[i];
    }
    return std::sqrt(sumOfSquares / sampleWindowCount);
}

float ImuSensor::getAccelerationXPeakToPeakInGForce() const {
    if (sampleWindowCount == 0) return 0.0f;
    float minValue = sampleBufferX[0];
    float maxValue = sampleBufferX[0];
    for (int i = 1; i < sampleWindowCount; i++) {
        if (sampleBufferX[i] < minValue) minValue = sampleBufferX[i];
        if (sampleBufferX[i] > maxValue) maxValue = sampleBufferX[i];
    }
    return maxValue - minValue;
}
float ImuSensor::getAccelerationYPeakToPeakInGForce() const {
    if (sampleWindowCount == 0) return 0.0f;
    float minValue = sampleBufferY[0];
    float maxValue = sampleBufferY[0];
    for (int i = 1; i < sampleWindowCount; i++) {
        if (sampleBufferY[i] < minValue) minValue = sampleBufferY[i];
        if (sampleBufferY[i] > maxValue) maxValue = sampleBufferY[i];
    }
    return maxValue - minValue;
}

float ImuSensor::getAccelerationZPeakToPeakInGForce() const {
    if (sampleWindowCount == 0) return 0.0f;
    float minValue = sampleBufferZ[0];
    float maxValue = sampleBufferZ[0];
    for (int i = 1; i < sampleWindowCount; i++) {
        if (sampleBufferZ[i] < minValue) minValue = sampleBufferZ[i];
        if (sampleBufferZ[i] > maxValue) maxValue = sampleBufferZ[i];
    }
    return maxValue - minValue;
}
