/**
 * @file IndoorAirQualityDisplay.ino
 * @brief Indoor air quality monitor with OLED display showing VOC, temperature, humidity, and pressure.
 *
 * This example demonstrates a grounded real-world device: a desktop air quality monitor
 * that uses a BME680 sensor to measure volatile organic compounds (VOC), temperature,
 * humidity, and atmospheric pressure. The readings are displayed in real-time on a
 * 128x64 OLED screen using the SSD1306 driver.
 *
 * The BME680 gas resistance reading serves as a VOC/IAQ indicator: lower resistance
 * indicates higher VOC concentration (poorer air quality). The display shows a simple
 * bar graph for the VOC level alongside numeric readings.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `EnvironmentalGasSensor` for BME680 VOC/IAQ detection (extends BmeEnvironmentalSensor)
 * - `OledDisplay` for SSD1306-based 128x64 monochrome display
 * - `setText()` for rendering text on the OLED display
 * - `fillRectangle()` for drawing a VOC level bar graph
 * - `drawHorizontalLine()` for display layout separators
 * - `UPDATE_DISPLAY_COMMAND` and `CLEAR_DISPLAY_COMMAND` for display control
 * - Multi-sensor data aggregation on a single display
 * - Loop-less, event-driven asynchronous execution via `initializeAsynchronousEngine()`
 *
 * @par Display layout (128x64 pixels):
 * - Row 0 (y=0):   "Air Quality Monitor"
 * - Row 1 (y=8):   Separator line
 * - Row 2 (y=16):  VOC level bar graph + label
 * - Row 3 (y=24):  Temperature reading
 * - Row 4 (y=32):  Humidity reading
 * - Row 5 (y=40):  Pressure reading
 * - Row 6 (y=48):  Gas resistance (kOhms)
 *
 * @date 2026-11-12
 * @version 1.0
 */

#include <Arduino.h>
#include <cstddef>
#include "ModestIoT.h"

/** @brief Sampler event queue length for air quality display. */
static const int SAMPLER_QUEUE_LENGTH = 10;

/** @brief Buffer size for formatted display strings. */
static const size_t DISPLAY_STRING_BUFFER_SIZE = 32;

/** @brief Serial communication baud rate. */
static const unsigned long SERIAL_BAUD_RATE = 115200;

/** @brief Delay in the main loop when no work is pending. */
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;

/** @name Application Mediator Implementation */
//@{

/**
 * @brief Indoor air quality monitor with OLED display.
 *
 * This class extends the `Device` base class to manage a BME680 environmental gas sensor
 * and an SSD1306 OLED display. Sensor readings are rendered on the display with a VOC
 * level bar graph and numeric values for temperature, humidity, and pressure.
 */
class IndoorAirQualityDisplay : public Device {
private:
    EnvironmentalGasSensor airQualitySensor;
    OledDisplay statusDisplay;

    char displayBuffer[DISPLAY_STRING_BUFFER_SIZE]; ///< Reusable buffer for formatted strings.

    /**
     * @brief Computes a VOC level (0-100) from gas resistance.
     *
     * Lower gas resistance indicates higher VOC concentration.
     * The scale is approximate: >100kOhms = excellent (0%), <10kOhms = poor (100%).
     *
     * @param gasResistanceInKOhms Current gas resistance reading.
     * @return VOC level percentage (0-100).
     */
    uint8_t computeVocLevelPercentage(float gasResistanceInKOhms) const {
        if (gasResistanceInKOhms <= 0.0f) return 0;
        // Map resistance to VOC level: higher resistance = lower VOC = better air
        float vocLevel = 100.0f * (1.0f - (gasResistanceInKOhms / 100.0f));
        if (vocLevel < 0.0f) vocLevel = 0.0f;
        if (vocLevel > 100.0f) vocLevel = 100.0f;
        return static_cast<uint8_t>(vocLevel);
    }

    /**
     * @brief Renders all sensor readings on the OLED display.
     */
    void renderDisplay() {
        // Clear the display
        statusDisplay.clearFrameBuffer();

        // Title
        statusDisplay.setText(F("Air Quality Monitor"), 0, 0);

        // Separator line
        statusDisplay.drawHorizontalLine(0, 8, 128);

        // VOC level bar graph
        uint8_t vocLevel = computeVocLevelPercentage(
            airQualitySensor.getGasResistanceInKOhms());
        uint8_t barWidth = (vocLevel * 80) / 100;  // 80 pixels max width
        statusDisplay.fillRectangle(2, 18, barWidth, 4);
        statusDisplay.setText(F("VOC"), 90, 16);

        // Temperature
        snprintf(displayBuffer, sizeof(displayBuffer), "Temp: %.1f C",
                 airQualitySensor.getTemperatureInCelsius());
        statusDisplay.setText(displayBuffer, 0, 24);

        // Humidity
        snprintf(displayBuffer, sizeof(displayBuffer), "Hum: %.1f%%",
                 airQualitySensor.getHumidityInPercentage());
        statusDisplay.setText(displayBuffer, 0, 32);

        // Pressure
        snprintf(displayBuffer, sizeof(displayBuffer), "Pres: %.1f hPa",
                 airQualitySensor.getPressureInHectopascals());
        statusDisplay.setText(displayBuffer, 0, 40);

        // Gas resistance
        snprintf(displayBuffer, sizeof(displayBuffer), "Gas: %.0fkOhm",
                 airQualitySensor.getGasResistanceInKOhms());
        statusDisplay.setText(displayBuffer, 0, 48);

        // Update the display
        statusDisplay.handle(OledDisplay::UPDATE_DISPLAY_COMMAND);
    }

protected:
    void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const override {
        // This device does not send telemetry; it only displays readings locally.
        (void)rawQueueItemPayload;
    }

    void maintainNetworkServices() const override {
        // No network services for this standalone display device.
    }

public:
    /**
     * @brief Constructs a new IndoorAirQualityDisplay application instance.
     *
     * @param sensorPin Source identifier for the BME680 sensor (not a GPIO pin;
     *                   BME680 uses I2C).
     * @param displayI2cAddress I2C address of the SSD1306 OLED display.
     * @param samplingIntervalInMilliseconds How often to sample sensors (ms).
     * @param timerGroupIndex Hardware timer group index (0-3).
     */
    IndoorAirQualityDisplay(int sensorPin, uint8_t displayI2cAddress,
                            unsigned long samplingIntervalInMilliseconds,
                            uint8_t timerGroupIndex = 0)
        : Device(samplingIntervalInMilliseconds, timerGroupIndex),
          airQualitySensor(sensorPin, EnvironmentalGasSensor::DEFAULT_I2C_ADDRESS, this),
          statusDisplay(displayI2cAddress, this)
    {
        initializeAsynchronousEngine(SAMPLER_QUEUE_LENGTH);

        appendSensorToScheduler(&airQualitySensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    }

    ~IndoorAirQualityDisplay() override = default;

    /**
     * @brief Handles sensor data events and updates the display.
     *
     * @param event The triggered event context payload.
     * @runs_on{sampler}
     */
    void on(Event event) override {
        if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
            renderDisplay();
            Serial.printf("[IndoorAirQualityDisplay] VOC: %.0fkOhm, Temp: %.1fC, "
                          "Hum: %.1f%%, Pres: %.1fhPa\n",
                          airQualitySensor.getGasResistanceInKOhms(),
                          airQualitySensor.getTemperatureInCelsius(),
                          airQualitySensor.getHumidityInPercentage(),
                          airQualitySensor.getPressureInHectopascals());
        }
    }

    void handle(Command command) override {
        // This device does not process external commands.
        (void)command;
    }

    /**
     * @brief Initializes the display and shows a startup message.
     * @return True if the sensor initialized successfully.
     */
    bool start() {
        if (!airQualitySensor.isInitialized()) {
            Serial.println("[IndoorAirQualityDisplay] BME680 sensor not found on I2C bus");
            return false;
        }

        // Show startup message
        statusDisplay.clearFrameBuffer();
        statusDisplay.setText(F("Air Quality"), 0, 0);
        statusDisplay.setText(F("Monitor"), 0, 8);
        statusDisplay.setText(F("Starting..."), 0, 24);
        statusDisplay.handle(OledDisplay::UPDATE_DISPLAY_COMMAND);

        Serial.println("[IndoorAirQualityDisplay] Indoor air quality monitor online.");
        return true;
    }
};
//@} // Application Mediator Implementation

/** @name Top-Level Declarative Canvas */
//@{

static const int BME680_SOURCE_IDENTIFIER = 1; ///< Source ID for BME680 (not a GPIO pin).
static const uint8_t OLED_DISPLAY_I2C_ADDRESS = 0x3C; ///< I2C address of SSD1306 display.

static const unsigned long SAMPLING_INTERVAL_IN_MILLISECONDS = 2000;
static const uint8_t HARDWARE_TIMER_CHANNEL_INDEX = 0;

static IndoorAirQualityDisplay* airQualityApplication = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    airQualityApplication = new IndoorAirQualityDisplay(
        BME680_SOURCE_IDENTIFIER,
        OLED_DISPLAY_I2C_ADDRESS,
        SAMPLING_INTERVAL_IN_MILLISECONDS,
        HARDWARE_TIMER_CHANNEL_INDEX
    );

    if (!airQualityApplication->start()) {
        Serial.println("[IndoorAirQualityDisplay] Device startup failed.");
        return;
    }
}

void loop() {
    // The master loop is completely clean and empty.
    // The framework handles all timing, sensor sampling, and display updates.
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
//@} // Top-Level Declarative Canvas
