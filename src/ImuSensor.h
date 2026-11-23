#ifndef IMU_SENSOR_H
#define IMU_SENSOR_H

/**
 * @file ImuSensor.h
 * @brief Declares the MPU6050 6-axis inertial measurement unit (IMU) sensor.
 * @ingroup sensors_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-23
 * @version 1.0
 */

#include "Sensor.h"
#include <Arduino.h>

/**
 * @brief Sensor adapter for MPU6050 6-axis inertial measurement unit (IMU).
 *
 * Implements I²C communication with the MPU6050 chip, reading raw accelerometer
 * and gyroscope data, applying factory calibration offsets and compensation
 * formulas, and providing cached readings in physical units (g for acceleration,
 * degrees/s for angular rate).
 *
 * The MPU6050 is widely used in vibration monitoring, motion detection, and
 * inertial measurement applications. It supports configurable full-scale ranges
 * for both accelerometer (±2/4/8/16 g) and gyroscope (±250/500/1000/2000 °/s).
 *
 * @note The constructor calls Wire.begin() to initialize the shared Arduino I²C
 *       interface. Call initialize() to verify connectivity and read calibration
 *       data.
 * @see BmeEnvironmentalSensor
 * @ingroup sensors_module
 */
class ImuSensor : public Sensor {
protected:
    /**
     * @brief Constructs an MPU6050 IMU adapter.
     * @param signalGpioPin Framework source identifier attached to emitted events.
     * @param i2cAddress Sensor's 7-bit I²C address (0x68 or 0x69).
     * @param parentHandler Optional handler that receives emitted events.
     * @note Calls Wire.begin() and initializes all calibration offsets to zero.
     */
    explicit ImuSensor(int signalGpioPin, uint8_t i2cAddress,
                        EventHandler* parentHandler = nullptr);

    uint8_t i2cAddress; ///< Configured MPU6050 7-bit I²C address.

    /** @name MPU6050 Factory Calibration Offsets */
    //@{
    int16_t accelerationOffsetX; ///< X-axis accelerometer offset (from factory trim registers).
    int16_t accelerationOffsetY; ///< Y-axis accelerometer offset (from factory trim registers).
    int16_t accelerationOffsetZ; ///< Z-axis accelerometer offset (from factory trim registers).
    int16_t gyroscopeOffsetX;  ///< X-axis gyroscope offset (from factory trim registers).
    int16_t gyroscopeOffsetY;  ///< Y-axis gyroscope offset (from factory trim registers).
    int16_t gyroscopeOffsetZ;  ///< Z-axis gyroscope offset (from factory trim registers).
    //@} // MPU6050 Factory Calibration Offsets

    /** @name Cached Readings */
    //@{
    float cachedAccelerationXInGForce;      ///< Cached X-axis acceleration in g-force.
    float cachedAccelerationYInGForce;      ///< Cached Y-axis acceleration in g-force.
    float cachedAccelerationZInGForce;      ///< Cached Z-axis acceleration in g-force.
    float cachedGyroscopeXInDegreesPerSecond;     ///< Cached X-axis angular rate in degrees/s.
    float cachedGyroscopeYInDegreesPerSecond;     ///< Cached Y-axis angular rate in degrees/s.
    float cachedGyroscopeZInDegreesPerSecond;     ///< Cached Z-axis angular rate in degrees/s.
    float cachedInternalTemperatureInCelsius; ///< Cached internal chip temperature in °C.
    //@} // Cached Readings

    /** @name Sample Window Configuration */
    //@{
    static constexpr int MAXIMUM_SAMPLE_WINDOW_SIZE = 32; ///< Maximum number of samples in the circular buffer.
    static constexpr int DEFAULT_SAMPLE_WINDOW_SIZE = 10; ///< Default number of samples for windowed statistics.
    //@} // Sample Window Configuration

    /** @name Circular Sample Buffer for Windowed Statistics */
    //@{
    float sampleBufferX[MAXIMUM_SAMPLE_WINDOW_SIZE]; ///< Circular buffer of X-axis acceleration samples.
    float sampleBufferY[MAXIMUM_SAMPLE_WINDOW_SIZE]; ///< Circular buffer of Y-axis acceleration samples.
    float sampleBufferZ[MAXIMUM_SAMPLE_WINDOW_SIZE]; ///< Circular buffer of Z-axis acceleration samples.
    int sampleWindowIndex; ///< Current write position in the circular buffer.
    int sampleWindowCount; ///< Number of valid samples in the buffer.
    int sampleWindowSize;  ///< Configured window size (1 to MAXIMUM_SAMPLE_WINDOW_SIZE).
    //@} // Circular Sample Buffer for Windowed Statistics

    /** @name Last-Notified Readings */
    //@{
    float lastNotifiedAccelerationXInGForce;      ///< Last-notified X-axis acceleration for change detection.
    float lastNotifiedAccelerationYInGForce;      ///< Last-notified Y-axis acceleration for change detection.
    float lastNotifiedAccelerationZInGForce;      ///< Last-notified Z-axis acceleration for change detection.
    float lastNotifiedGyroscopeXInDegreesPerSecond;     ///< Last-notified X-axis angular rate for change detection.
    float lastNotifiedGyroscopeYInDegreesPerSecond;     ///< Last-notified Y-axis angular rate for change detection.
    float lastNotifiedGyroscopeZInDegreesPerSecond;     ///< Last-notified Z-axis angular rate for change detection.
    //@} // Last-Notified Readings

    bool initialized; ///< Whether chip ID was verified and calibration data was read.

    /** @name Change Detection Epsilon Thresholds */
    //@{
    static constexpr float ACCELERATION_CHANGE_EPSILON = 0.01f;  ///< Minimum acceleration change to emit event (g-force).
    static constexpr float GYROSCOPE_CHANGE_EPSILON = 0.1f;    ///< Minimum angular rate change to emit event (°/s).
    //@} // Change Detection Epsilon Thresholds

    /** @name Initial Cached Values */
    //@{
    static constexpr float INITIAL_CACHED_ACCELERATION_IN_GFORCE = 0.0f;    ///< Initial cached acceleration.
    static constexpr float INITIAL_CACHED_GYROSCOPE_IN_DEGREES_PER_SECOND = 0.0f;   ///< Initial cached angular rate.
    static constexpr float INITIAL_CACHED_TEMPERATURE_IN_CELSIUS = 0.0f;    ///< Initial cached temperature.
    //@} // Initial Cached Values

    /** @name Initial Last-Notified Values */
    //@{
    static constexpr float INITIAL_LAST_NOTIFIED_ACCELERATION_IN_GFORCE = -1.0f;  ///< Initial last-notified acceleration sentinel.
    static constexpr float INITIAL_LAST_NOTIFIED_GYROSCOPE_IN_DEGREES_PER_SECOND = -1.0f; ///< Initial last-notified angular rate sentinel.
    //@} // Initial Last-Notified Values

    static constexpr int PROPAGATION_SUPPRESSION_IDENTIFIER = -1; ///< Event identifier used internally to prevent propagation.

    /** @name IMU I²C Register Addresses */
    //@{
    static constexpr uint8_t IMU_REGISTER_CHIP_ID = 0x75;
    static constexpr uint8_t IMU_REGISTER_SMPLRT_DIV = 0x19;
    static constexpr uint8_t IMU_REGISTER_CONFIG = 0x1A;
    static constexpr uint8_t IMU_REGISTER_GYRO_CONFIG = 0x1B;
    static constexpr uint8_t IMU_REGISTER_ACCEL_CONFIG = 0x1C;
    static constexpr uint8_t IMU_REGISTER_ACCEL_CONFIG2 = 0x1D;
    static constexpr uint8_t IMU_REGISTER_PWR_MGMT_1 = 0x6B;
    static constexpr uint8_t IMU_REGISTER_PWR_MGMT_2 = 0x6C;
    static constexpr uint8_t IMU_REGISTER_INT_ENABLE = 0x38;
    static constexpr uint8_t IMU_REGISTER_INT_STATUS = 0x3A;
    static constexpr uint8_t IMU_REGISTER_ACCEL_XOUT_H = 0x3B;
    static constexpr uint8_t IMU_REGISTER_TEMP_OUT_H = 0x41;
    static constexpr uint8_t IMU_REGISTER_GYRO_XOUT_H = 0x43;
    static constexpr uint8_t IMU_REGISTER_SELF_TEST_X = 0x0D;
    static constexpr uint8_t IMU_REGISTER_SELF_TEST_Y = 0x0E;
    static constexpr uint8_t IMU_REGISTER_SELF_TEST_Z = 0x0F;
    static constexpr uint8_t IMU_REGISTER_SELF_TEST_A = 0x10;
    //@} // IMU I²C Register Addresses

    /** @name IMU Internal Configuration Constants */
    //@{
    static constexpr uint8_t IMU_CHIP_ID_INTERNAL = 0x68; ///< Expected chip ID for MPU6050.
    static constexpr uint8_t IMU_CLOCK_SRC_PLL = 0x01; ///< Use PLL with X-axis gyroscope reference.
    static constexpr uint8_t IMU_DLPF_CFG_184HZ = 0x01; ///< DLPF configuration: 184 Hz bandwidth.
    static constexpr uint8_t IMU_SLEEP_BIT = 0x40; ///< Sleep mode bit in PWR_MGMT_1.
    static constexpr uint8_t IMU_CYCLE_BIT = 0x20; ///< Cycle mode bit in PWR_MGMT_1.
    static constexpr uint8_t IMU_TEMP_DIS_BIT = 0x08; ///< Temperature disable bit in PWR_MGMT_1.
    static constexpr uint32_t IMU_WAKE_UP_DELAY_IN_MILLISECONDS = 30; ///< Gyro ZRO settling time after wake-up (datasheet spec).
    //@} // IMU Internal Configuration Constants

    /** @name IMU Data Format Constants */
    //@{
    static constexpr uint8_t IMU_RAW_DATA_BYTE_COUNT = 14; ///< 6 axes + temperature (2 bytes each).
    static constexpr int16_t IMU_ACCEL_LSB_PER_G_2G = 16384; ///< LSB/g for ±2 g range.
    static constexpr int16_t IMU_ACCEL_LSB_PER_G_4G = 8192;  ///< LSB/g for ±4 g range.
    static constexpr int16_t IMU_ACCEL_LSB_PER_G_8G = 4096;  ///< LSB/g for ±8 g range.
    static constexpr int16_t IMU_ACCEL_LSB_PER_G_16G = 2048; ///< LSB/g for ±16 g range.
    static constexpr float IMU_GYRO_DPS_PER_LSB_250 = 131.0f;   ///< LSB per °/s for ±250 °/s.
    static constexpr float IMU_GYRO_DPS_PER_LSB_500 = 65.5f;    ///< LSB per °/s for ±500 °/s.
    static constexpr float IMU_GYRO_DPS_PER_LSB_1000 = 32.8f;   ///< LSB per °/s for ±1000 °/s.
    static constexpr float IMU_GYRO_DPS_PER_LSB_2000 = 16.4f;   ///< LSB per °/s for ±2000 °/s.
    static constexpr float IMU_TEMP_SENSITIVITY = 340.0f;       ///< Temperature sensitivity (counts/°C).
    static constexpr float IMU_TEMP_OFFSET = 36.53f;            ///< Temperature offset (°C).
    //@} // IMU Data Format Constants

    uint8_t accelerationFullScale; ///< Current accelerometer full-scale range configuration.
    uint8_t gyroscopeFullScale;  ///< Current gyroscope full-scale range configuration.

    /**
     * @brief Writes a single byte to an MPU6050 register.
     * @param registerAddress MPU6050 register address.
     * @param data Byte to write.
     * @return @c true if the write was successful.
     * @runs_on{sampler}
     */
    bool writeRegister(uint8_t registerAddress, uint8_t data) const;

    /**
     * @brief Reads a single byte from an MPU6050 register.
     * @param registerAddress MPU6050 register address.
     * @return Byte read from the register.
     * @runs_on{sampler}
     */
    [[nodiscard]] uint8_t readRegister(uint8_t registerAddress) const;

    /**
     * @brief Reads the factory calibration offsets from self-test/trim registers.
     * @return @c true if all calibration registers were read successfully.
     * @runs_on{sampler}
     */
    [[nodiscard]] bool readCalibrationData();

    /**
     * @brief Reads raw acceleration, temperature, and gyroscope data.
     * @param rawAccelerationX Output receiving raw X-axis acceleration (16-bit).
     * @param rawAccelerationY Output receiving raw Y-axis acceleration (16-bit).
     * @param rawAccelerationZ Output receiving raw Z-axis acceleration (16-bit).
     * @param rawTemperature Output receiving raw temperature (16-bit).
     * @param rawGyroscopeX Output receiving raw X-axis angular rate (16-bit).
     * @param rawGyroscopeY Output receiving raw Y-axis angular rate (16-bit).
     * @param rawGyroscopeZ Output receiving raw Z-axis angular rate (16-bit).
     * @return @c true if all 14 data bytes were read successfully.
     * @runs_on{sampler}
     */
    bool readRawData(int16_t& rawAccelerationX, int16_t& rawAccelerationY, int16_t& rawAccelerationZ,
                     int16_t& rawTemperature, int16_t& rawGyroscopeX, int16_t& rawGyroscopeY,
                     int16_t& rawGyroscopeZ) const;

    /**
     * @brief Compensates raw acceleration reading to physical units.
     * @param rawAcceleration Raw 16-bit accelerometer value.
     * @return Compensated acceleration in g-force.
     * @runs_on{sampler}
     */
    float compensateAcceleration(int16_t rawAcceleration) const;

    /**
     * @brief Compensates raw gyroscope reading to physical units.
     * @param rawGyroscope Raw 16-bit gyroscope value.
     * @return Compensated angular rate in degrees/s.
     * @runs_on{sampler}
     */
    float compensateGyroscope(int16_t rawGyroscope) const;

    /**
     * @brief Compensates raw temperature reading.
     * @param rawTemp Raw 16-bit temperature value.
     * @return Compensated temperature in degrees Celsius.
     * @runs_on{sampler}
     */
    static float compensateTemperature(int16_t rawTemp);

    /**
     * @brief Returns the LSB/g-force divisor for the current accelerometer full-scale range.
     * @return LSB per g-force for the configured range.
     * @runs_on{sampler}
     */
    [[nodiscard]] int16_t getAccelerationLsbPerGForce() const;

    /**
     * @brief Returns the degrees/s per LSB divisor for the current gyroscope full-scale range.
     * @return Degrees/s per LSB for the configured range.
     * @runs_on{sampler}
     */
    [[nodiscard]] float getGyroscopeDegreesPerSecondPerLsb() const;

protected:
    /**
     * @brief Performs an MPU6050 read cycle and updates cached measurements.
     * @param event Mutable event request/response payload.
     * @runs_on{sampler}
     */
    void processEvent(Event& event) override;

    [[nodiscard]] uint32_t getDefaultSamplingIntervalInMilliseconds() const override;
    [[nodiscard]] uint32_t getMinimalSamplingIntervalInMilliseconds() const override;

public:
    /// Default MPU6050 address when AD0 pin is low.
    static constexpr uint8_t DEFAULT_I2C_ADDRESS = 0x68;
    /// Alternate MPU6050 address when AD0 pin is high.
    static constexpr uint8_t ALTERNATE_I2C_ADDRESS = 0x69;

    /// Default sampling interval in milliseconds (100 ms = 10 Hz).
    static constexpr uint32_t DEFAULT_SAMPLING_INTERVAL_IN_MILLISECONDS = 100;
    /// Minimum sampling interval in milliseconds (5 ms = 200 Hz, limited by I²C speed).
    static constexpr uint32_t MINIMUM_SAMPLING_INTERVAL_IN_MILLISECONDS = 5;

    /// Accelerometer full scale: ±2 g.
    static constexpr uint8_t IMU_ACCEL_FS_2G = 0x00;
    /// Accelerometer full scale: ±4 g.
    static constexpr uint8_t IMU_ACCEL_FS_4G = 0x08;
    /// Accelerometer full scale: ±8 g.
    static constexpr uint8_t IMU_ACCEL_FS_8G = 0x10;
    /// Accelerometer full scale: ±16 g.
    static constexpr uint8_t IMU_ACCEL_FS_16G = 0x18;
    /// Gyroscope full scale: ±250 °/s.
    static constexpr uint8_t IMU_GYRO_FS_250 = 0x00;
    /// Gyroscope full scale: ±500 °/s.
    static constexpr uint8_t IMU_GYRO_FS_500 = 0x08;
    /// Gyroscope full scale: ±1000 °/s.
    static constexpr uint8_t IMU_GYRO_FS_1000 = 0x10;
    /// Gyroscope full scale: ±2000 °/s.
    static constexpr uint8_t IMU_GYRO_FS_2000 = 0x18;

    /**
     * @brief Constructs an MPU6050 IMU sensor.
     * @param signalGpioPin Framework source identifier attached to emitted events.
     * @param i2cAddress Sensor's 7-bit I²C address (default: 0x68).
     * @param accelerationFullScale Accelerometer full-scale range (default: ±2 g-force).
     * @param gyroscopeFullScale Gyroscope full-scale range (default: ±250 °/s).
     * @param parentHandler Optional handler that receives emitted events.
     */
    explicit ImuSensor(int signalGpioPin, uint8_t i2cAddress = DEFAULT_I2C_ADDRESS,
                        uint8_t accelerationFullScale = IMU_ACCEL_FS_2G,
                        uint8_t gyroscopeFullScale = IMU_GYRO_FS_250,
                        EventHandler* parentHandler = nullptr);

    /**
     * @brief Initializes the sensor by verifying chip ID and reading calibration data.
     * @return @c true if chip ID matched and calibration data was read.
     * @details Configures the MPU6050 clock source, DLPF, full-scale ranges,
     *          and disables sleep mode. Reads factory calibration offsets.
     */
    [[nodiscard]] bool initialize();

    /**
     * @brief Reports the result of the most recent initialization attempt.
     * @return @c true if chip ID was verified and calibration data was read.
     */
    [[nodiscard]] bool isInitialized() const;

    /** @name Accelerometer Readings */
    //@{
    /**
     * @brief Returns the X-axis acceleration from the most recent successful sample.
     * @return Acceleration in g-force (1 g-force = 9.81 m/s²).
     * @warning Check isInitialized() first; before initialization this returns 0.
     */
    [[nodiscard]] float getAccelerationXInGForce() const;

    /**
     * @brief Returns the Y-axis acceleration from the most recent successful sample.
     * @return Acceleration in g-force (1 g-force = 9.81 m/s²).
     * @warning Check isInitialized() first; before initialization this returns 0.
     */
    [[nodiscard]] float getAccelerationYInGForce() const;

    /**
     * @brief Returns the Z-axis acceleration from the most recent successful sample.
     * @return Acceleration in g-force (1 g-force = 9.81 m/s²).
     * @warning Check isInitialized() first; before initialization this returns 0.
     */
    [[nodiscard]] float getAccelerationZInGForce() const;
    //@} // Accelerometer Readings

    /** @name Gyroscope Readings */
    //@{
    /**
     * @brief Returns the X-axis angular rate from the most recent successful sample.
     * @return Angular rate in degrees/s.
     * @warning Check isInitialized() first; before initialization this returns 0.
     */
    [[nodiscard]] float getGyroscopeXInDegreesPerSecond() const;

    /**
     * @brief Returns the Y-axis angular rate from the most recent successful sample.
     * @return Angular rate in degrees/s.
     * @warning Check isInitialized() first; before initialization this returns 0.
     */
    [[nodiscard]] float getGyroscopeYInDegreesPerSecond() const;

    /**
     * @brief Returns the Z-axis angular rate from the most recent successful sample.
     * @return Angular rate in degrees/s.
     * @warning Check isInitialized() first; before initialization this returns 0.
     */
    [[nodiscard]] float getGyroscopeZInDegreesPerSecond() const;
    //@} // Gyroscope Readings

    /**
     * @brief Returns the internal chip temperature from the most recent successful sample.
     * @return Internal temperature in degrees Celsius.
     * @note This is the IMU internal die temperature, not the ambient environmental
     *       temperature. It reflects chip heating from operation and ambient conditions.
     * @warning Check isInitialized() first; before initialization this returns 0.
     */
    [[nodiscard]] float getInternalTemperatureInCelsius() const;

    /**
     * @brief Computes the total acceleration magnitude from the three axes.
     * @return Magnitude in g-force (√(X² + Y² + Z²)).
     * @warning Check isInitialized() first; before initialization this returns 0.
     */
    [[nodiscard]] float getAccelerationMagnitudeInGForce() const;

    /**
     * @brief Computes the RMS acceleration from the three axes.
     * @return RMS acceleration in g-force (√((X² + Y² + Z²) / 3)).
     * @warning Check isInitialized() first; before initialization this returns 0.
     */
    [[nodiscard]] float getAccelerationRmsInGForce() const;

    /**
     * @brief Configures the sample window size for windowed statistics.
     * @param size Number of recent samples to retain (1 to MAXIMUM_SAMPLE_WINDOW_SIZE).
     * @details The sensor automatically populates the circular buffer during each
     *          processEvent() call. Windowed statistics (RMS, peak-to-peak) use
     *          the samples in this buffer.
     */
    void setSampleWindowSize(int size);

    /**
     * @brief Returns the current sample window size.
     * @return Number of samples configured for windowed statistics.
     */
    [[nodiscard]] int getSampleWindowSize() const;

    /**
     * @brief Returns the number of samples currently in the buffer.
     * @return Number of valid samples (up to the configured window size).
     */
    [[nodiscard]] int getSampleCount() const;

    /** @name Windowed Per-Axis Acceleration Statistics */
    //@{
    /**
     * @brief Computes the RMS acceleration of the X-axis over the sample window.
     * @return RMS acceleration in g-force.
     * @warning Returns 0 if no samples are available.
     */
    [[nodiscard]] float getAccelerationXRmsInGForce() const;

    /**
     * @brief Computes the RMS acceleration of the Y-axis over the sample window.
     * @return RMS acceleration in g-force.
     * @warning Returns 0 if no samples are available.
     */
    [[nodiscard]] float getAccelerationYRmsInGForce() const;

    /**
     * @brief Computes the RMS acceleration of the Z-axis over the sample window.
     * @return RMS acceleration in g-force.
     * @warning Returns 0 if no samples are available.
     */
    [[nodiscard]] float getAccelerationZRmsInGForce() const;
    //@} // Windowed Per-Axis Acceleration RMS

    /** @name Windowed Per-Axis Peak-to-Peak Acceleration */
    //@{
    /**
     * @brief Computes the peak-to-peak acceleration of the X-axis over the sample window.
     * @return Peak-to-peak value in g-force (max - min).
     * @warning Returns 0 if no samples are available.
     */
    [[nodiscard]] float getAccelerationXPeakToPeakInGForce() const;

    /**
     * @brief Computes the peak-to-peak acceleration of the Y-axis over the sample window.
     * @return Peak-to-peak value in g-force (max - min).
     * @warning Returns 0 if no samples are available.
     */
    [[nodiscard]] float getAccelerationYPeakToPeakInGForce() const;

    /**
     * @brief Computes the peak-to-peak acceleration of the Z-axis over the sample window.
     * @return Peak-to-peak value in g-force (max - min).
     * @warning Returns 0 if no samples are available.
     */
    [[nodiscard]] float getAccelerationZPeakToPeakInGForce() const;
    //@} // Windowed Per-Axis Peak-to-Peak Acceleration
};

#endif // IMU_SENSOR_H
