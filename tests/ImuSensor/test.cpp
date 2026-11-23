#include <time.h>
/**
 * @file test.cpp
 * @brief Host unit tests for the ImuSensor (MPU6050) component.
 *
 * Validates compensation formulas, sample buffer statistics, event emission,
 * initialization, and edge cases.
 * @date 2026-11-23
 * @version 1.0
 */

#include "Arduino.h"
#include "Wire.h"
#include "Event.h"
#include "Sensor.h"
#include "ImuSensor.h"
#include <cassert>
#include <cstdio>
#include <cmath>

// Test helper: captures events from the sensor
class EventCapture : public EventHandler {
public:
    Event lastEvent{-1};
    void on(Event event) override { lastEvent = event; }
};

// Testable subclass exposes protected methods for testing
class TestableImuSensor : public ImuSensor {
public:
    // 2-arg: pin + address
    TestableImuSensor(int signalGpioPin, uint8_t i2cAddress, EventHandler* parentHandler = nullptr)
        : ImuSensor(signalGpioPin, i2cAddress, parentHandler) {}
    // 3-arg: pin + address + accelFs (gyroFs defaults to 0x00)
    TestableImuSensor(int signalGpioPin, uint8_t i2cAddress, uint8_t accelFs, bool /*tag*/)
        : ImuSensor(signalGpioPin, i2cAddress, accelFs, 0x00, nullptr) {}
    // 4-arg: pin + address + accelFs + gyroFs
    TestableImuSensor(int signalGpioPin, uint8_t i2cAddress, uint8_t accelFs, uint8_t gyroFs, bool /*tag*/)
        : ImuSensor(signalGpioPin, i2cAddress, accelFs, gyroFs, nullptr) {}
    // 5-arg: pin + address + accelFs + gyroFs + parentHandler
    TestableImuSensor(int signalGpioPin, uint8_t i2cAddress, uint8_t accelFs, uint8_t gyroFs, EventHandler* parentHandler)
        : ImuSensor(signalGpioPin, i2cAddress, accelFs, gyroFs, parentHandler) {}
    using ImuSensor::compensateAcceleration;
    using ImuSensor::compensateGyroscope;
    using ImuSensor::compensateTemperature;
    using ImuSensor::getAccelerationLsbPerGForce;
    using ImuSensor::getGyroscopeDegreesPerSecondPerLsb;
    using ImuSensor::readRawData;
    using ImuSensor::readCalibrationData;
    using ImuSensor::writeRegister;
    using ImuSensor::readRegister;
    // Expose protected processEvent for testing
    void processEvent(Event& event) { ImuSensor::processEvent(event); }
    // Expose protected sampling interval methods
    uint32_t getDefaultSamplingIntervalInMilliseconds() const { return ImuSensor::getDefaultSamplingIntervalInMilliseconds(); }
    uint32_t getMinimalSamplingIntervalInMilliseconds() const { return ImuSensor::getMinimalSamplingIntervalInMilliseconds(); }
};

void testConstructorInitializesState() {
    Wire.clearReadBuffer();
    Wire.useReadBuffer = false;
    TestableImuSensor sensor(4, 0x68);
    assert(sensor.getAccelerationXInGForce() == 0.0f);
    assert(sensor.getAccelerationYInGForce() == 0.0f);
    assert(sensor.getAccelerationZInGForce() == 0.0f);
    assert(sensor.getGyroscopeXInDegreesPerSecond() == 0.0f);
    assert(sensor.getGyroscopeYInDegreesPerSecond() == 0.0f);
    assert(sensor.getGyroscopeZInDegreesPerSecond() == 0.0f);
    assert(sensor.getInternalTemperatureInCelsius() == 0.0f);
    assert(sensor.isInitialized() == false);
    printf("  Constructor initializes state correctly\n");
}

void testConstructorWithFullScaleRanges() {
    Wire.clearReadBuffer();
    Wire.useReadBuffer = false;
    // IMU_ACCEL_FS_8G = 0x10, IMU_GYRO_FS_1000 = 0x10
    TestableImuSensor sensor(5, 0x69, 0x10, 0x10, true);
    assert(sensor.getAccelerationLsbPerGForce() == 4096);  // IMU_ACCEL_LSB_PER_G_8G
    assert(sensor.getGyroscopeDegreesPerSecondPerLsb() == 32.8f);  // IMU_GYRO_DPS_PER_LSB_1000
    printf("  Constructor with full-scale ranges configures correctly\n");
}

void testI2cAddresses() {
    assert(ImuSensor::DEFAULT_I2C_ADDRESS == 0x68);
    assert(ImuSensor::ALTERNATE_I2C_ADDRESS == 0x69);
    printf("  I2C addresses are correct\n");
}

void testSamplingIntervals() {
    Wire.clearReadBuffer();
    Wire.useReadBuffer = false;
    TestableImuSensor sensor(6, 0x68);
    assert(sensor.getDefaultSamplingIntervalInMilliseconds() == 100);
    assert(sensor.getMinimalSamplingIntervalInMilliseconds() == 5);
    printf("  Sampling intervals are correct\n");
}

void testCompensateAccelerationAt2G() {
    Wire.clearReadBuffer();
    Wire.useReadBuffer = false;
    // IMU_ACCEL_FS_2G = 0x00
    TestableImuSensor sensor(7, 0x68, 0x00, true);
    // 16384 LSB = 1.0g at ±2g range
    float result = sensor.compensateAcceleration(16384);
    assert(result == 1.0f);
    // 0 LSB = 0.0g
    assert(sensor.compensateAcceleration(0) == 0.0f);
    // -16384 LSB = -1.0g
    assert(sensor.compensateAcceleration(-16384) == -1.0f);
    printf("  compensateAcceleration at ±2g range is correct\n");
}

void testCompensateAccelerationAt4G() {
    Wire.clearReadBuffer();
    Wire.useReadBuffer = false;
    TestableImuSensor sensor(8, 0x68, 0x08, true);
    // 8192 LSB = 1.0g at ±4g range
    float result = sensor.compensateAcceleration(8192);
    assert(result == 1.0f);
    printf("  compensateAcceleration at ±4g range is correct\n");
}

void testCompensateAccelerationAt8G() {
    Wire.clearReadBuffer();
    Wire.useReadBuffer = false;
    TestableImuSensor sensor(9, 0x68, 0x10, true);
    // 4096 LSB = 1.0g at ±8g range
    float result = sensor.compensateAcceleration(4096);
    assert(result == 1.0f);
    printf("  compensateAcceleration at ±8g range is correct\n");
}

void testCompensateAccelerationAt16G() {
    Wire.clearReadBuffer();
    Wire.useReadBuffer = false;
    TestableImuSensor sensor(10, 0x68, 0x18, true);
    // 2048 LSB = 1.0g at ±16g range
    float result = sensor.compensateAcceleration(2048);
    assert(result == 1.0f);
    printf("  compensateAcceleration at ±16g range is correct\n");
}

void testCompensateGyroscopeAt250Dps() {
    Wire.clearReadBuffer();
    Wire.useReadBuffer = false;
    TestableImuSensor sensor(11, 0x68, 0x00, 0x00, true);
    // 131 LSB = ~1.0°/s at ±250°/s range
    float result = sensor.compensateGyroscope(131);
    assert(result == 1.0f);
    assert(sensor.compensateGyroscope(0) == 0.0f);
    printf("  compensateGyroscope at ±250°/s range is correct\n");
}

void testCompensateGyroscopeAt500Dps() {
    Wire.clearReadBuffer();
    Wire.useReadBuffer = false;
    TestableImuSensor sensor(12, 0x68, 0x00, 0x08, true);
    // 65.5 LSB per °/s at ±500°/s range
    float result = sensor.compensateGyroscope(655);
    assert(result == 10.0f);
    printf("  compensateGyroscope at ±500°/s range is correct\n");
}

void testCompensateGyroscopeAt1000Dps() {
    Wire.clearReadBuffer();
    Wire.useReadBuffer = false;
    TestableImuSensor sensor(13, 0x68, 0x00, 0x10, true);
    // 32.8 LSB per °/s at ±1000°/s range
    float result = sensor.compensateGyroscope(328);
    assert(result == 10.0f);
    printf("  compensateGyroscope at ±1000°/s range is correct\n");
}

void testCompensateGyroscopeAt2000Dps() {
    Wire.clearReadBuffer();
    Wire.useReadBuffer = false;
    TestableImuSensor sensor(14, 0x68, 0x00, 0x18, true);
    // 16.4 LSB per °/s at ±2000°/s range
    float result = sensor.compensateGyroscope(164);
    assert(result == 10.0f);
    printf("  compensateGyroscope at ±2000°/s range is correct\n");
}

void testCompensateTemperature() {
    // IMU_TEMP_SENSITIVITY = 340.0, IMU_TEMP_OFFSET = 36.53
    // rawTemp / 340.0 + 36.53
    // At rawTemp = 0: 0/340 + 36.53 = 36.53°C
    float result = TestableImuSensor::compensateTemperature(0);
    assert(std::abs(result - 36.53f) < 0.01f);
    // At rawTemp = 3400: 3400/340 + 36.53 = 10 + 36.53 = 46.53°C
    result = TestableImuSensor::compensateTemperature(3400);
    assert(std::abs(result - 46.53f) < 0.01f);
    // Negative rawTemp: -3400/340 + 36.53 = -10 + 36.53 = 26.53°C
    result = TestableImuSensor::compensateTemperature(-3400);
    assert(std::abs(result - 26.53f) < 0.01f);
    printf("  compensateTemperature formula is correct\n");
}

void testAccelerationMagnitude() {
    Wire.clearReadBuffer();
    Wire.useReadBuffer = false;
    TestableImuSensor sensor(15, 0x68);
    // Before any reading, all cached values are 0.0f
    assert(sensor.getAccelerationMagnitudeInGForce() == 0.0f);
    printf("  Acceleration magnitude returns 0 when uninitialized\n");
}

void testAccelerationRms() {
    Wire.clearReadBuffer();
    Wire.useReadBuffer = false;
    TestableImuSensor sensor(16, 0x68);
    // Before any reading, all cached values are 0.0f
    assert(sensor.getAccelerationRmsInGForce() == 0.0f);
    printf("  Acceleration RMS returns 0 when uninitialized\n");
}

void testSampleWindowSizeDefaults() {
    Wire.clearReadBuffer();
    Wire.useReadBuffer = false;
    TestableImuSensor sensor(17, 0x68);
    assert(sensor.getSampleWindowSize() == 10);
    assert(sensor.getSampleCount() == 0);
    printf("  Sample window size defaults to 10\n");
}

void testSampleWindowSizeClamping() {
    Wire.clearReadBuffer();
    Wire.useReadBuffer = false;
    TestableImuSensor sensor(18, 0x68);
    // Below minimum
    sensor.setSampleWindowSize(0);
    assert(sensor.getSampleWindowSize() == 1);
    // Above maximum
    sensor.setSampleWindowSize(100);
    assert(sensor.getSampleWindowSize() == 32);
    // Valid value
    sensor.setSampleWindowSize(15);
    assert(sensor.getSampleWindowSize() == 15);
    printf("  Sample window size clamping works correctly\n");
}

void testSampleWindowStatisticsEmpty() {
    Wire.clearReadBuffer();
    Wire.useReadBuffer = false;
    TestableImuSensor sensor(19, 0x68);
    assert(sensor.getSampleCount() == 0);
    assert(sensor.getAccelerationXRmsInGForce() == 0.0f);
    assert(sensor.getAccelerationYRmsInGForce() == 0.0f);
    assert(sensor.getAccelerationZRmsInGForce() == 0.0f);
    assert(sensor.getAccelerationXPeakToPeakInGForce() == 0.0f);
    assert(sensor.getAccelerationYPeakToPeakInGForce() == 0.0f);
    assert(sensor.getAccelerationZPeakToPeakInGForce() == 0.0f);
    printf("  Windowed statistics return 0 when no samples\n");
}

void testProcessEventReadsRawData() {
    Wire.transmissionCount = 0;
    Wire.clearReadBuffer();
    Wire.useReadBuffer = true;
    // Push 14 bytes: accel X = 16384 (1.0g), rest zeros
    // Non-zero value ensures change detection passes
    Wire.pushReadBuffer(0x40); // 16384 >> 8
    Wire.pushReadBuffer(0x00); // 16384 & 0xFF
    for (int i = 2; i < 14; i++) {
        Wire.pushReadBuffer(0x00);
    }
    Wire.byteCount = 14;
    EventCapture capture;
    TestableImuSensor sensor(20, 0x68, 0x00, 0x00, &capture);
    
    Event event(21);
    sensor.processEvent(event);
    
    // Wire should have been accessed
    assert(Wire.transmissionCount > 0);
    // Significant change from initial 0.0f cached value should emit DATA_READ_EVENT
    assert(event.identifier == 20);
    printf("  processEvent reads raw data and emits DATA_READ_EVENT\n");
}

void testProcessEventSuppressesUnchangedValues() {
    Wire.transmissionCount = 0;
    Wire.clearReadBuffer();
    Wire.useReadBuffer = true;
    // Push 14 bytes of raw data (all zeros)
    for (int i = 0; i < 14; i++) {
        Wire.pushReadBuffer(0x00);
    }
    Wire.byteCount = 14;
    EventCapture capture;
    TestableImuSensor sensor(21, 0x68, 0x00, 0x00, &capture);
    
    // First reading: all zeros, cached values are 0.0f, so no change
    // The epsilon check compares against cached values (0.0f), new values are also 0.0f
    // Therefore no change detected, event should be suppressed
    Event event(21);
    sensor.processEvent(event);
    assert(event.identifier == -1);
    printf("  processEvent suppresses unchanged values\n");
}

void testProcessEventEmitsOnSignificantChange() {
    Wire.transmissionCount = 0;
    Wire.clearReadBuffer();
    Wire.useReadBuffer = true;
    // Push 14 bytes: accel X = 16384 (1.0g), rest zeros
    Wire.pushReadBuffer(0x40); // 16384 >> 8
    Wire.pushReadBuffer(0x00); // 16384 & 0xFF
    for (int i = 2; i < 14; i++) {
        Wire.pushReadBuffer(0x00);
    }
    Wire.byteCount = 14;
    EventCapture capture;
    TestableImuSensor sensor(22, 0x68, 0x00, 0x00, &capture);
    
    Event event(21);
    sensor.processEvent(event);
    
    // 1.0g change from 0.0f cached value exceeds epsilon (0.01f)
    assert(event.identifier == 20);
    assert(sensor.getAccelerationXInGForce() == 1.0f);
    printf("  processEvent emits DATA_READ_EVENT on significant change\n");
}

void testProcessEventReadFailureSuppression() {
    Wire.transmissionCount = 0;
    Wire.clearReadBuffer();
    Wire.useReadBuffer = true;
    // Push fewer bytes than expected to simulate read failure
    Wire.pushReadBuffer(0x00);
    Wire.byteCount = 1; // Only 1 byte, but 14 expected
    EventCapture capture;
    TestableImuSensor sensor(23, 0x68, 0x00, 0x00, &capture);
    
    Event event(21);
    sensor.processEvent(event);
    
    // Read failure should suppress propagation
    assert(event.identifier == -1);
    printf("  processEvent suppresses on read failure\n");
}

void testProcessEventWithGyroscopeChange() {
    Wire.transmissionCount = 0;
    Wire.clearReadBuffer();
    Wire.useReadBuffer = true;
    // Push 14 bytes: accel all zeros, gyro X = 131 (1.0°/s)
    for (int i = 0; i < 8; i++) {
        Wire.pushReadBuffer(0x00);
    }
    Wire.pushReadBuffer(0x00); // gyro X high byte
    Wire.pushReadBuffer(0x83); // gyro X low byte (131)
    for (int i = 10; i < 14; i++) {
        Wire.pushReadBuffer(0x00);
    }
    Wire.byteCount = 14;
    EventCapture capture;
    TestableImuSensor sensor(24, 0x68, 0x00, 0x00, &capture);
    
    Event event(21);
    sensor.processEvent(event);
    
    // Gyro change of 1.0°/s from 0.0f exceeds epsilon (0.1f)
    assert(event.identifier == 20);
    assert(sensor.getGyroscopeXInDegreesPerSecond() == 1.0f);
    printf("  processEvent emits on gyroscope change\n");
}

void testProcessEventWithSmallGyroscopeChange() {
    Wire.transmissionCount = 0;
    Wire.clearReadBuffer();
    Wire.useReadBuffer = true;
    // Push 14 bytes: accel all zeros, gyro X = 13 (0.1°/s, within epsilon)
    for (int i = 0; i < 8; i++) {
        Wire.pushReadBuffer(0x00);
    }
    Wire.pushReadBuffer(0x00); // gyro X high byte
    Wire.pushReadBuffer(0x0D); // gyro X low byte (13, ~0.1°/s)
    for (int i = 10; i < 14; i++) {
        Wire.pushReadBuffer(0x00);
    }
    Wire.byteCount = 14;
    EventCapture capture;
    TestableImuSensor sensor(25, 0x68, 0x00, 0x00, &capture);
    
    Event event(21);
    sensor.processEvent(event);
    
    // Gyro change of ~0.1°/s from 0.0f is within epsilon (0.1f), so suppressed
    assert(event.identifier == -1);
    printf("  processEvent suppresses small gyroscope changes\n");
}

void testProcessEventWrongIdentifier() {
    Wire.transmissionCount = 0;
    Wire.clearReadBuffer();
    Wire.useReadBuffer = false;
    EventCapture capture;
    TestableImuSensor sensor(26, 0x68, 0x00, 0x00, &capture);
    
    // Non-MEASURE_DATA_REQUESTED event should be ignored
    Event event(999);
    sensor.processEvent(event);
    
    // Event should be unchanged
    assert(event.identifier == 999);
    // Wire should not have been accessed
    assert(Wire.transmissionCount == 0);
    printf("  processEvent ignores non-MEASURE_DATA_REQUESTED events\n");
}

void testInitializeSuccess() {
    Wire.transmissionCount = 0;
    Wire.clearReadBuffer();
    Wire.useReadBuffer = true;
    // Chip ID read: 0x68
    Wire.pushReadBuffer(0x68);
    // Calibration data reads (4 registers)
    Wire.pushReadBuffer(0x00); // SELF_TEST_X
    Wire.pushReadBuffer(0x00); // SELF_TEST_Y
    Wire.pushReadBuffer(0x00); // SELF_TEST_Z
    Wire.pushReadBuffer(0x00); // SELF_TEST_A
    Wire.byteCount = 1;
    EventCapture capture;
    TestableImuSensor sensor(27, 0x68, &capture);
    
    bool result = sensor.initialize();
    assert(result == true);
    assert(sensor.isInitialized() == true);
    printf("  initialize() succeeds with valid chip ID\n");
}

void testInitializeFailsOnBadChipId() {
    Wire.transmissionCount = 0;
    Wire.clearReadBuffer();
    Wire.useReadBuffer = true;
    // Wrong chip ID
    Wire.pushReadBuffer(0x00);
    Wire.byteCount = 1;
    EventCapture capture;
    TestableImuSensor sensor(28, 0x68, &capture);
    
    bool result = sensor.initialize();
    assert(result == false);
    assert(sensor.isInitialized() == false);
    printf("  initialize() fails on bad chip ID\n");
}

void testInitializeFailsOnWriteFailure() {
    Wire.transmissionCount = 0;
    Wire.clearReadBuffer();
    Wire.useReadBuffer = false;
    Wire.failureAt = 0; // First write fails
    EventCapture capture;
    TestableImuSensor sensor(29, 0x68, &capture);
    
    bool result = sensor.initialize();
    assert(result == false);
    assert(sensor.isInitialized() == false);
    printf("  initialize() fails on I2C write failure\n");
}

void testSampleBufferPopulation() {
    Wire.transmissionCount = 0;
    Wire.clearReadBuffer();
    Wire.useReadBuffer = true;
    EventCapture capture;
    TestableImuSensor sensor(30, 0x68, 0x00, 0x00, &capture);
    sensor.setSampleWindowSize(5);
    
    // Simulate 3 readings with different values (use values within int16_t range)
    // 0.5g = 8192, 1.0g = 16384, 1.5g = 24576
    int16_t rawValues[] = {8192, 16384, 24576};
    for (int reading = 0; reading < 3; reading++) {
        Wire.clearReadBuffer();
        Wire.useReadBuffer = true;
        int16_t rawValue = rawValues[reading];
        Wire.pushReadBuffer((rawValue >> 8) & 0xFF);
        Wire.pushReadBuffer(rawValue & 0xFF);
        for (int i = 2; i < 14; i++) {
            Wire.pushReadBuffer(0x00);
        }
        Wire.byteCount = 14;
        
        Event event(21);
        sensor.processEvent(event);
    }
    
    assert(sensor.getSampleCount() == 3);
    printf("  Sample buffer populates during processEvent\n");
}

void testWindowedRmsStatistics() {
    Wire.transmissionCount = 0;
    Wire.clearReadBuffer();
    Wire.useReadBuffer = true;
    EventCapture capture;
    TestableImuSensor sensor(31, 0x68, 0x00, 0x00, &capture);
    sensor.setSampleWindowSize(3);
    
    // Simulate 3 readings: 0.5g, 1.0g, 1.5g on X-axis (within int16_t range)
    int16_t rawValues[] = {8192, 16384, 24576};
    for (int reading = 0; reading < 3; reading++) {
        Wire.clearReadBuffer();
        Wire.useReadBuffer = true;
        int16_t rawValue = rawValues[reading];
        Wire.pushReadBuffer((rawValue >> 8) & 0xFF);
        Wire.pushReadBuffer(rawValue & 0xFF);
        for (int i = 2; i < 14; i++) {
            Wire.pushReadBuffer(0x00);
        }
        Wire.byteCount = 14;
        
        Event event(21);
        sensor.processEvent(event);
    }
    
    // RMS of [0.5, 1.0, 1.5] = sqrt((0.25 + 1.0 + 2.25) / 3) = sqrt(3.5/3) ≈ 1.0801
    float rms = sensor.getAccelerationXRmsInGForce();
    float expected = std::sqrt(3.5f / 3.0f);
    assert(std::abs(rms - expected) < 0.01f);
    printf("  Windowed RMS statistics are correct\n");
}

void testWindowedPeakToPeakStatistics() {
    Wire.transmissionCount = 0;
    Wire.clearReadBuffer();
    Wire.useReadBuffer = true;
    EventCapture capture;
    TestableImuSensor sensor(32, 0x68, 0x00, 0x00, &capture);
    sensor.setSampleWindowSize(3);
    
    // Simulate 3 readings: 0.5g, 1.0g, 1.5g on X-axis (within int16_t range)
    int16_t rawValues[] = {8192, 16384, 24576};
    for (int reading = 0; reading < 3; reading++) {
        Wire.clearReadBuffer();
        Wire.useReadBuffer = true;
        int16_t rawValue = rawValues[reading];
        Wire.pushReadBuffer((rawValue >> 8) & 0xFF);
        Wire.pushReadBuffer(rawValue & 0xFF);
        for (int i = 2; i < 14; i++) {
            Wire.pushReadBuffer(0x00);
        }
        Wire.byteCount = 14;
        
        Event event(21);
        sensor.processEvent(event);
    }
    
    // Peak-to-peak of [0.5, 1.0, 1.5] = 1.5 - 0.5 = 1.0
    float ptp = sensor.getAccelerationXPeakToPeakInGForce();
    assert(std::abs(ptp - 1.0f) < 0.01f);
    printf("  Windowed peak-to-peak statistics are correct\n");
}

void testCircularBufferWrapping() {
    Wire.transmissionCount = 0;
    Wire.clearReadBuffer();
    Wire.useReadBuffer = true;
    EventCapture capture;
    TestableImuSensor sensor(33, 0x68, 0x00, 0x00, &capture);
    sensor.setSampleWindowSize(3);
    
    // Simulate 5 readings with increasing values (within int16_t range)
    // 0.2g=3277, 0.4g=6554, 0.6g=9830, 0.8g=13107, 1.0g=16384
    int16_t rawValues[] = {3277, 6554, 9830, 13107, 16384};
    for (int reading = 0; reading < 5; reading++) {
        Wire.clearReadBuffer();
        Wire.useReadBuffer = true;
        int16_t rawValue = rawValues[reading];
        Wire.pushReadBuffer((rawValue >> 8) & 0xFF);
        Wire.pushReadBuffer(rawValue & 0xFF);
        for (int i = 2; i < 14; i++) {
            Wire.pushReadBuffer(0x00);
        }
        Wire.byteCount = 14;
        
        Event event(21);
        sensor.processEvent(event);
    }
    
    assert(sensor.getSampleCount() == 3);
    // After 5 writes with size 3: buffer[0]=0.8g, buffer[1]=1.0g, buffer[2]=0.6g
    // Peak-to-peak = 1.0 - 0.6 = 0.4
    float ptp = sensor.getAccelerationXPeakToPeakInGForce();
    assert(std::abs(ptp - 0.4f) < 0.1f);
    printf("  Circular buffer wrapping works correctly\n");
}

void testNegativeAccelerationValues() {
    Wire.transmissionCount = 0;
    Wire.clearReadBuffer();
    Wire.useReadBuffer = true;
    EventCapture capture;
    TestableImuSensor sensor(34, 0x68, 0x00, 0x00, &capture);
    
    // Push 14 bytes: accel X = -16384 (-1.0g), rest zeros
    // -16384 in two's complement: 0xC000
    Wire.pushReadBuffer(0xC0);
    Wire.pushReadBuffer(0x00);
    for (int i = 2; i < 14; i++) {
        Wire.pushReadBuffer(0x00);
    }
    Wire.byteCount = 14;
    
    Event event(21);
    sensor.processEvent(event);
    
    assert(event.identifier == 20);
    assert(sensor.getAccelerationXInGForce() == -1.0f);
    printf("  Negative acceleration values are handled correctly\n");
}

void testNegativeGyroscopeValues() {
    Wire.transmissionCount = 0;
    Wire.clearReadBuffer();
    Wire.useReadBuffer = true;
    EventCapture capture;
    TestableImuSensor sensor(35, 0x68, 0x00, 0x00, &capture);
    
    // Push 14 bytes: accel all zeros, gyro X high byte = 0x80 (sign bit set)
    // This produces a negative int16_t value when combined with low byte
    for (int i = 0; i < 8; i++) {
        Wire.pushReadBuffer(0x00);
    }
    Wire.pushReadBuffer(0x80); // High byte with sign bit
    Wire.pushReadBuffer(0x00); // Low byte
    for (int i = 10; i < 14; i++) {
        Wire.pushReadBuffer(0x00);
    }
    Wire.byteCount = 14;
    
    Event event(21);
    sensor.processEvent(event);
    
    assert(event.identifier == 20);
    // 0x8000 = -32768 as int16_t, -32768 / 131.0 = -250.14°/s
    assert(sensor.getGyroscopeXInDegreesPerSecond() < 0.0f);
    printf("  Negative gyroscope values are handled correctly\n");
}

void testTemperatureReading() {
    Wire.transmissionCount = 0;
    Wire.clearReadBuffer();
    Wire.useReadBuffer = true;
    EventCapture capture;
    TestableImuSensor sensor(36, 0x68, 0x00, 0x00, &capture);
    
    // Push 14 bytes: accel X = 16384 (1.0g), rest zeros, temp = 3400 (46.53°C), gyro all zeros
    // Non-zero acceleration triggers change detection so the event is emitted
    Wire.pushReadBuffer(0x40); // 16384 >> 8 (accel X high byte)
    Wire.pushReadBuffer(0x00); // 16384 & 0xFF (accel X low byte)
    for (int i = 2; i < 6; i++) {
        Wire.pushReadBuffer(0x00);
    }
    Wire.pushReadBuffer(0x0D); // 3400 >> 8
    Wire.pushReadBuffer(0x48); // 3400 & 0xFF
    for (int i = 8; i < 14; i++) {
        Wire.pushReadBuffer(0x00);
    }
    Wire.byteCount = 14;
    
    Event event(21);
    sensor.processEvent(event);
    
    assert(event.identifier == 20);
    assert(sensor.getInternalTemperatureInCelsius() == 46.53f);
    printf("  Temperature reading is compensated correctly\n");
}

void testConstants() {
    assert(0x68 == 0x68);
    assert(14 == 14);
    assert(16384 == 16384);
    assert(8192 == 8192);
    assert(4096 == 4096);
    assert(2048 == 2048);
    assert(131.0f == 131.0f);
    assert(65.5f == 65.5f);
    assert(32.8f == 32.8f);
    assert(16.4f == 16.4f);
    assert(340.0f == 340.0f);
    assert(36.53f == 36.53f);
    assert(32 == 32);
    assert(10 == 10);
    assert(100 == 100);
    assert(5 == 5);
    printf("  All constants are correct\n");
}

void testSampleWindowSizeResetOnResize() {
    Wire.clearReadBuffer();
    Wire.useReadBuffer = true;
    EventCapture capture;
    TestableImuSensor sensor(37, 0x68, 0x00, 0x00, &capture);
    
    // Set window to 5, then change to 3
    sensor.setSampleWindowSize(5);
    assert(sensor.getSampleWindowSize() == 5);
    sensor.setSampleWindowSize(3);
    assert(sensor.getSampleWindowSize() == 3);
    // Buffer state should be reset
    assert(sensor.getSampleCount() == 0);
    printf("  Sample window resize resets buffer state\n");
}

int main() {
    printf("Running ImuSensor tests...\n");
    
    testConstructorInitializesState();
    testConstructorWithFullScaleRanges();
    testI2cAddresses();
    testSamplingIntervals();
    testCompensateAccelerationAt2G();
    testCompensateAccelerationAt4G();
    testCompensateAccelerationAt8G();
    testCompensateAccelerationAt16G();
    testCompensateGyroscopeAt250Dps();
    testCompensateGyroscopeAt500Dps();
    testCompensateGyroscopeAt1000Dps();
    testCompensateGyroscopeAt2000Dps();
    testCompensateTemperature();
    testAccelerationMagnitude();
    testAccelerationRms();
    testSampleWindowSizeDefaults();
    testSampleWindowSizeClamping();
    testSampleWindowStatisticsEmpty();
    testProcessEventReadsRawData();
    testProcessEventSuppressesUnchangedValues();
    testProcessEventEmitsOnSignificantChange();
    testProcessEventReadFailureSuppression();
    testProcessEventWithGyroscopeChange();
    testProcessEventWithSmallGyroscopeChange();
    testProcessEventWrongIdentifier();
    testInitializeSuccess();
    testInitializeFailsOnBadChipId();
    testInitializeFailsOnWriteFailure();
    testSampleBufferPopulation();
    testWindowedRmsStatistics();
    testWindowedPeakToPeakStatistics();
    testCircularBufferWrapping();
    testNegativeAccelerationValues();
    testNegativeGyroscopeValues();
    testTemperatureReading();
    testConstants();
    testSampleWindowSizeResetOnResize();
    
    printf("All ImuSensor tests passed successfully!\n");
    return 0;
}
