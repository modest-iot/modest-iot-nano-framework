/**
 * @file AirQualityMonitor.ino
 * @brief Desktop air quality monitor with gas detection, LCD display, and audible alarm.
 *
 * Uses an MQ-2 gas sensor to detect combustible gases (LPG, propane, methane), displays
 * real-time PPM readings and sensor mode on a 16x2 LCD, and sounds a buzzer when gas
 * is detected. The sensor auto-calibrates on startup, then switches to accurate mode
 * for continuous monitoring.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `Device` base class with async engine for loop-less execution
 * - `GasSensor` with calibration flow (`CalibratingMode` → `AccurateMode`)
 * - `Buzzer` actuator with `PLAY_TONE_COMMAND` for audible alerts
 * - `CharacterLcdDisplay` actuator with `UPDATE_TEXT_COMMAND` for real-time display
 * - Event routing via `on(Event)` with `DATA_READ_EVENT_IDENTIFIER` and `DIGITAL_GAS_DETECTED_EVENT`
 * - Sensor operating mode tracking via `getOperatingMode()`
 * - Direct actuator command dispatch via `handle(Command)`
 * @date 2026-06-12
 * @version 1.0
 */

#include <Arduino.h>
#include <memory>
#include "ModestIoT.h"

/** @brief Serial communication baud rate. */
static const unsigned long SERIAL_BAUD_RATE = 115200;

/** @brief Delay in the main loop when no work is pending. */
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;

/** @brief Serial port connection timeout (ms). */
static const unsigned long SERIAL_TIMEOUT_IN_MILLISECONDS = 5000;

/** @brief Sampler event queue length. */
static const int ASYNC_ENGINE_QUEUE_DEPTH = 10;
/** @brief Shared I2C bus configuration for the character LCD. */
static const I2cBusConfiguration AIR_QUALITY_MONITOR_I2C_CONFIGURATION{-1, -1, 100000};

/** @brief Buzzer tone frequency (Hz). */
static const unsigned long BUZZER_TONE_FREQ = 2000;

/** @brief Buzzer tone duration (ms). */
static const unsigned long BUZZER_TONE_DURATION_IN_MILLISECONDS = 100;

/** @name Telemetry JSON Keys */
//@{
/** @brief JSON key for gas concentration in PPM. */
#define JSON_KEY_GAS_PPM "gasPpm"
/** @brief JSON key for gas detection status. */
#define JSON_KEY_GAS_DETECTED "gasDetected"
/** @brief JSON key for sensor operating mode. */
#define JSON_KEY_SENSOR_MODE "sensorMode"
//@} // Telemetry JSON Keys

/** @name Concrete Application Data Schema Specification */
//@{

/**
 * @brief Telemetry package for Air Quality Monitor data.
 * Contains gas concentration, detection status, and sensor mode.
 */
class AirQualityTelemetryPackage : public TelemetryPackage {
private:
    /// @brief Gas concentration in PPM.
    float gasConcentrationInPpm;
    /// @brief Flag indicating if gas is detected.
    bool gasDetected;
    /// @brief The operating mode of the sensor, cast from `GasSensor::OperatingMode`.
    int32_t operatingMode;

public:
    AirQualityTelemetryPackage(float ppm, bool detected, GasSensor::OperatingMode mode)
        : gasConcentrationInPpm(ppm), gasDetected(detected), operatingMode(static_cast<int32_t>(mode)) {}

    ~AirQualityTelemetryPackage() override = default;

    /**
     * @brief Writes the telemetry data fields to the given writer.
     * @param writer The telemetry writer to populate.
     */
    void write(TelemetryWriter& writer) const override {
        writer.field(JSON_KEY_GAS_PPM, this->gasConcentrationInPpm);
        writer.field(JSON_KEY_GAS_DETECTED, this->gasDetected);
        writer.field(JSON_KEY_SENSOR_MODE, this->operatingMode);
    }
};
//@} // Concrete Application Data Schema Specification

/** @name Application Mediator Implementation */
//@{

/**
 * @brief Orchestrates the GasSensor, Buzzer, and CharacterLcdDisplay for air quality monitoring.
 *
 * This device manages the gas sensor's calibration and operational modes, updates
 * an LCD with real-time data, and provides audible alerts via a buzzer.
 */
class AirQualityMonitorDevice : public Device {
private:
    ArduinoI2cBus i2cBus;
    /// @brief The gas sensor instance.
    GasSensor gasSensor;
    /// @brief The buzzer instance for alarms.
    Buzzer alarmBuzzer;
    /// @brief The LCD display instance for status.
    CharacterLcdDisplay statusDisplay;
    /// @brief The MQTT gateway client reference.
    MqttGatewayClient& gatewayClient;

    /**
     * @brief Updates the LCD display with current sensor status and backlight based on gas detection.
     *
     * Depending on the sensor's `OperatingMode` (Raw, Calibrating, or Accurate),
     * it formats the appropriate text and displays it on the LCD.
     * Also controls the backlight based on whether gas is detected.
     */
    void updateOperationStatusDisplay() {
        char textBufferInternal[2][21]; // Max 20 chars + null terminator for 2 lines
        const char* textBufferPtrs[2];

        switch (gasSensor.getOperatingMode()) {
            case GasSensor::OperatingMode::RawMode:
                snprintf(textBufferInternal[0], sizeof(textBufferInternal[0]), "Mode: %s",
                         GasSensor::operatingModeName(gasSensor.getOperatingMode()));
                snprintf(textBufferInternal[1], sizeof(textBufferInternal[1]), "Raw Val: %d", gasSensor.getRawAnalogValue());
                break;
            case GasSensor::OperatingMode::CalibratingMode:
                snprintf(textBufferInternal[0], sizeof(textBufferInternal[0]), "Mode: %s",
                         GasSensor::operatingModeName(gasSensor.getOperatingMode()));
                snprintf(textBufferInternal[1], sizeof(textBufferInternal[1]), "Keep in clean air!");
                break;
            case GasSensor::OperatingMode::AccurateMode:
                snprintf(textBufferInternal[0], sizeof(textBufferInternal[0]), "Mode: %s",
                         GasSensor::operatingModeName(gasSensor.getOperatingMode()));
                if (gasSensor.getTargetGasType() != GasSensor::GasType::None) {
                    snprintf(textBufferInternal[1], sizeof(textBufferInternal[1]), "%s: %.1f PPM",
                             GasSensor::gasTypeName(gasSensor.getTargetGasType()),
                             gasSensor.getGasConcentrationInPpm());
                } else {
                    snprintf(textBufferInternal[1], sizeof(textBufferInternal[1]), "Rs/R0: %.2f", gasSensor.getRsR0Ratio());
                }
                break;
        }

        textBufferPtrs[0] = textBufferInternal[0];
        textBufferPtrs[1] = textBufferInternal[1];

        statusDisplay.setLineBuffer(textBufferPtrs); // Use the overloaded method
        statusDisplay.handle(CharacterLcdDisplay::UPDATE_TEXT_COMMAND);

        // Control backlight based on gas detection
        if (gasSensor.isGasDetected()) {
            statusDisplay.handle(CharacterLcdDisplay::TURN_BACKLIGHT_ON_COMMAND);
        } else {
            statusDisplay.handle(CharacterLcdDisplay::TURN_BACKLIGHT_OFF_COMMAND);
        }
    }

protected:
    /**
     * @brief Processes telemetry data by sending it via MQTT.
     * @param rawQueueItemPayload Pointer to the queued telemetry payload.
     */
    void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const override {
        if (rawQueueItemPayload != nullptr) {
            gatewayClient.sendTelemetryRecord(*rawQueueItemPayload);
        }
    }

    /**
     * @brief Executes periodic background network maintenance.
     */
    void maintainNetworkServices() const override {
        gatewayClient.service();
    }

public:
    /**
     * @brief Constructs the AirQualityMonitorDevice.
     * @param gasAnalogPin Analog pin for the gas sensor.
     * @param gasDigitalPin Digital pin for the gas sensor's threshold output.
     * @param buzzerPin GPIO pin for the buzzer.
     * @param lcdI2CAddress I2C address of the LCD.
     * @param lcdColumns Number of columns on the LCD.
     * @param lcdRows Number of rows on the LCD.
     * @param client MQTT Gateway Client reference.
     * @param samplingIntervalInMilliseconds Polling interval for sensors.
     * @param timerGroupIndex Hardware timer index.
     */
    AirQualityMonitorDevice(int gasAnalogPin, int gasDigitalPin, int buzzerPin,
                            uint8_t lcdI2CAddress, uint8_t lcdColumns, uint8_t lcdRows,
                            MqttGatewayClient& client,
                            unsigned long samplingIntervalInMilliseconds, uint8_t timerGroupIndex = 0)
        : Device(samplingIntervalInMilliseconds, timerGroupIndex),
          i2cBus(),
          gasSensor(gasAnalogPin, gasDigitalPin, this, GasSensor::OperatingMode::RawMode), // Start in RawMode
          alarmBuzzer(buzzerPin, this),
          statusDisplay(i2cBus, lcdI2CAddress, lcdColumns, lcdRows, true, this), // Backlight on initially
          gatewayClient(client)
    {}

    bool start() {
        const I2cStatus busStatus = i2cBus.start(AIR_QUALITY_MONITOR_I2C_CONFIGURATION);
        if (busStatus != I2cStatus::Ok) {
            Serial.printf("[AirQualityMonitor] I2C bus startup failed (status %u).\n",
                          static_cast<unsigned int>(busStatus));
            return false;
        }
        if (!statusDisplay.start()) {
            Serial.printf("[AirQualityMonitor] LCD startup failed (status %u).\n",
                          static_cast<unsigned int>(statusDisplay.getLastI2cResult().status));
            return false;
        }

        // Set handlers for sensors (this device acts as the EventHandler)
        gasSensor.setHandler(this);

        // Append sensors to the scheduler
        // IMPORTANT: Registration must occur before initializeAsynchronousEngine(), because
        // the engine start guard rejects any registration after the engine is running.
        appendSensorToScheduler(&gasSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);

        // Initialize asynchronous engine
        initializeAsynchronousEngine(ASYNC_ENGINE_QUEUE_DEPTH);
        return true;
    }

    /**
     * @brief Handles sensor data-ready and gas detection events.
     * @param event The triggered event context payload.
     * @runs_on{sampler}
     */
    void on(Event event) override {
        if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER ||
            event.identifier == GasSensor::DIGITAL_GAS_DETECTED_EVENT_IDENTIFIER) {

            updateOperationStatusDisplay(); // Renamed from updateLcd

            // Control buzzer based on gas detection status
            if (gasSensor.isGasDetected()) {
                alarmBuzzer.setTone(BUZZER_TONE_FREQ, BUZZER_TONE_DURATION_IN_MILLISECONDS);
                alarmBuzzer.handle(Buzzer::PLAY_TONE_COMMAND);
            } else {
                alarmBuzzer.handle(Buzzer::TURN_OFF_COMMAND);
            }

            // Enqueue telemetry
            auto packet = std::make_unique<AirQualityTelemetryPackage>(
                gasSensor.getGasConcentrationInPpm(),
                gasSensor.isGasDetected(),
                gasSensor.getOperatingMode()
            );
            enqueueTelemetryPayload(std::move(packet));
        }

        if (gasSensor.getOperatingMode() == GasSensor::OperatingMode::CalibratingMode) {
            // During calibration, we want to update the LCD with instructions and status even if no new data is propagated
            updateOperationStatusDisplay();
        }
    }

    /**
     * @brief Handles incoming commands (not directly used by this device, but required by interface).
     * @param command The command to handle.
     */
    void handle(Command command) override {
        // Not directly handling commands for this example, but could be extended
    }

    /**
     * @brief Starts the gas sensor calibration process.
     */
    void startGasSensorCalibration() {
        gasSensor.startCalibration();
    }

    /**
     * @brief Sets the target gas type for PPM calculation.
     * @param gasType The GasType to target.
     */
    void setGasSensorTargetGas(GasSensor::GasType gasType) {
        gasSensor.setTargetGas(gasType);
    }
};
//@} // Application Mediator Implementation

/** @name Top-Level Declarative Canvas */
//@{

/** @brief Hardware Pin Definitions */
/// @brief Analog pin for the gas sensor (ADC input).
static const int GAS_ANALOG_PIN = 34; // Example: ESP32 ADC1_CH6
/// @brief Digital pin for the gas sensor's threshold output (digital I/O).
static const int GAS_DIGITAL_PIN = 2; // Example: ESP32 GPIO2 (D0 on some MQ boards)
/// @brief GPIO pin for the buzzer (PWM output).
static const int BUZZER_PIN = 13; // Example: ESP32 GPIO13

/** @brief LCD Display Configurations */
/// @brief I2C address of the LCD module (e.g., 0x27 or 0x3F).
static const uint8_t LCD_I2C_ADDRESS = 0x27; 
/// @brief Number of columns on the LCD (e.g., 16 or 20).
static const uint8_t LCD_COLUMNS = 16;
/// @brief Number of rows on the LCD (e.g., 2 or 4).
static const uint8_t LCD_ROWS = 2;

/** @brief Network and MQTT Configuration */
/// @brief WiFi SSID for network connection.
static const char* WIFI_SSID = "Wokwi-GUEST";
/// @brief WiFi password for network connection.
static const char* WIFI_PASSWORD = "";
/// @brief MQTT broker IP address.
static const char* MQTT_BROKER_IP = "192.168.1.100"; 
/// @brief MQTT broker port.
static const uint16_t MQTT_BROKER_PORT = 1883;
/// @brief MQTT topic for publishing telemetry data.
static const char* MQTT_PUBLISH_TOPIC = "airquality/monitor/data";
/// @brief MQTT client ID to identify this device.
static const char* MQTT_CLIENT_ID = "AirQualityMonitor01";

// Framework Instance Pointers
static WiFiConnectivityDriver* transportDriver = nullptr;
static MqttGatewayClient* gatewayClient = nullptr;
static AirQualityMonitorDevice* airQualityApplication = nullptr;

/** @name Serial Output Configuration */
//@{
/** @brief Log prefix for all AirQualityMonitor serial output. */
static const char* AIR_QUALITY_MONITOR_LOG_PREFIX = "[AirQualityMonitor]";
//@} // Serial Output Configuration

/**
 * @brief Initializes the device, sensors, connectivity, and MQTT gateway.
 */
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    Device::setFirmwareVersion("AQM-200 FW 1.0.0");
    while (!Serial && millis() < SERIAL_TIMEOUT_IN_MILLISECONDS) { /* wait for serial port to connect */ }
    Serial.println((String(AIR_QUALITY_MONITOR_LOG_PREFIX) + " Starting up...").c_str());

    // 1. Initialize Connectivity
    transportDriver = new WiFiConnectivityDriver(WIFI_SSID, WIFI_PASSWORD);
    transportDriver->connect(); // Start WiFi connection in background

    // 2. Initialize MQTT Gateway
    gatewayClient = new MqttGatewayClient(
        *transportDriver,
        transportDriver->getNetworkSocket(), // Use the exposed WiFiClient
        MQTT_BROKER_IP,
        MQTT_BROKER_PORT,
        MQTT_PUBLISH_TOPIC,
        MQTT_CLIENT_ID
    );

    // 3. Initialize Air Quality Monitor Device
    airQualityApplication = new AirQualityMonitorDevice(
        GAS_ANALOG_PIN,
        GAS_DIGITAL_PIN,
        BUZZER_PIN,
        LCD_I2C_ADDRESS,
        LCD_COLUMNS,
        LCD_ROWS,
        *gatewayClient,
        1000 // Sample every 1 second
    );

    // 4. Configure Gas Sensor Calibration and Target Gas
    // IMPORTANT: Ensure the sensor is in clean air during calibration!
    airQualityApplication->startGasSensorCalibration();
    airQualityApplication->setGasSensorTargetGas(GasSensor::GasType::LPG); // Example: Target LPG for PPM calculation

    if (airQualityApplication->start()) {
        Serial.println((String(AIR_QUALITY_MONITOR_LOG_PREFIX) + " Setup complete. Monitoring started.").c_str());
    } else {
        Serial.println("[AirQualityMonitor] Startup failed; monitoring and telemetry were not started.");
    }
}

/**
 * @brief The main application loop, kept empty as the device runs asynchronously.
 */
void loop() {
    // The master loop is completely clean and empty.
    // The framework handles all timing, sensor sampling, and telemetry publishing asynchronously.
    // vTaskDelay clarifies that this loop yields to FreeRTOS — the device is loopless.
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
//@} // Top-Level Declarative Canvas