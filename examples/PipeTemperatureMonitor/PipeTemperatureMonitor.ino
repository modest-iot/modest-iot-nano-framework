/**
 * @file PipeTemperatureMonitor.ino
 * @brief Freeze-alert system that monitors pipe temperature and triggers an alarm if too cold.
 *
 * This example demonstrates a grounded real-world device: a pipe freeze monitor installed
 * near water pipes in unheated spaces (garage, basement, attic). It uses a DS18B20
 * 1-Wire temperature sensor to measure pipe temperature. If the temperature drops below
 * a warning threshold (4°C), the RGB LED turns yellow and the buzzer sounds a warning.
 * If it drops below a critical threshold (2°C), the LED turns red and the buzzer sounds
 * an alarm to warn of potential pipe freeze.
 *
 * The DS18B20 sensor uses the native 1-Wire protocol implementation in the framework,
 * requiring no external OneWire or DallasTemperature libraries.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `OneWireTemperatureSensor` for DS18B20 native 1-Wire protocol (no external library)
 * - `RgbLed` for color-coded status indication (green/yellow/red)
 * - `Buzzer` for audible alarm
 * - `setColor()` and `SET_COLOR_COMMAND` for RgbLed color changes
 * - Fault detection with `setFaultThreshold()` for sensor health monitoring
 * - `SENSOR_FAULT_EVENT_IDENTIFIER` and `SENSOR_RECOVERED_EVENT_IDENTIFIER` for fault handling
 * - Threshold-based alarm logic with hysteresis
 * - Loop-less, event-driven asynchronous execution via `initializeAsynchronousEngine()`
 *
 * @par Alarm states:
 * - Normal (>4°C): LED green, no alarm
 * - Warning (2-4°C): LED yellow, intermittent buzzer
 * - Critical (<2°C): LED red, continuous buzzer
 *
 * @date 2026-11-12
 * @version 1.0
 */

/** @name Temperature Thresholds (°C) */
//@{
static constexpr float FREEZE_WARNING_THRESHOLD = 4.0f;  ///< Warning level: pipe may freeze soon.
static constexpr float FREEZE_ALARM_THRESHOLD = 2.0f;    ///< Critical level: pipe likely to freeze.
//@} // Temperature Thresholds

/** @name Alarm Timing */
//@{
static constexpr unsigned long BUZZER_WARNING_DURATION_MS = 200;  ///< Buzzer on duration for warning.
static constexpr unsigned long BUZZER_WARNING_INTERVAL_MS = 2000; ///< Interval between warning beeps.
//@} // Alarm Timing

/** @brief Sampler event queue length for pipe temperature monitoring. */
#define SAMPLER_QUEUE_LENGTH 10

#include <Arduino.h>
#include "ModestIoT.h"

/** @brief Serial communication baud rate. */
static const unsigned long SERIAL_BAUD_RATE = 115200;

/** @brief Delay in the main loop when no work is pending. */
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;

/** @name Application Mediator Implementation */
//@{

/**
 * @brief Pipe freeze alert monitor device.
 *
 * This class extends the `Device` base class to manage a DS18B20 temperature sensor,
 * an RGB LED for visual status, and a buzzer for audible alarms. Temperature readings
 * are compared against warning and critical thresholds to determine the alarm state.
 */
class PipeTemperatureMonitor : public Device {
private:
    OneWireTemperatureSensor pipeSensor;
    RgbLed statusLed;
    Buzzer alarmBuzzer;

    bool alarmActive; ///< Whether the critical alarm is currently sounding.

    /**
     * @brief Updates the LED color and buzzer state based on current temperature.
     * @param temperatureInCelsius Current pipe temperature reading.
     */
    void updateAlarmState(float temperatureInCelsius) {
        if (temperatureInCelsius < FREEZE_ALARM_THRESHOLD) {
            // Critical: red LED, continuous buzzer
            statusLed.setColor(RgbLed::RED);
            statusLed.handle(RgbLed::SET_COLOR_COMMAND);
            alarmBuzzer.handle(Buzzer::TURN_ON_COMMAND);
            alarmActive = true;
            Serial.println("[PipeTemperatureMonitor] CRITICAL: Pipe temperature below 2°C!");
        } else if (temperatureInCelsius < FREEZE_WARNING_THRESHOLD) {
            // Warning: yellow LED, intermittent buzzer
            statusLed.setColor(RgbLed::YELLOW);
            statusLed.handle(RgbLed::SET_COLOR_COMMAND);
            alarmBuzzer.handle(Buzzer::TURN_ON_COMMAND);
            delay(BUZZER_WARNING_DURATION_MS);
            alarmBuzzer.handle(Buzzer::TURN_OFF_COMMAND);
            alarmActive = false;
            Serial.println("[PipeTemperatureMonitor] WARNING: Pipe temperature below 4°C");
        } else {
            // Normal: green LED, no alarm
            statusLed.setColor(RgbLed::GREEN);
            statusLed.handle(RgbLed::SET_COLOR_COMMAND);
            alarmBuzzer.handle(Buzzer::TURN_OFF_COMMAND);
            alarmActive = false;
            Serial.printf("[PipeTemperatureMonitor] Normal: Pipe temperature %.1f°C\n",
                          temperatureInCelsius);
        }
    }

protected:
    void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const override {
        // This device does not send telemetry; it only monitors locally.
        (void)rawQueueItemPayload;
    }

    void maintainNetworkServices() const override {
        // No network services for this standalone monitoring device.
    }

public:
    /**
     * @brief Constructs a new PipeTemperatureMonitor application instance.
     *
     * @param dataPin GPIO pin connected to the DS18B20 1-Wire data line.
     * @param redPin PWM GPIO pin for the RGB LED red channel.
     * @param greenPin PWM GPIO pin for the RGB LED green channel.
     * @param bluePin PWM GPIO pin for the RGB LED blue channel.
     * @param buzzerPin GPIO pin connected to the buzzer.
     * @param samplingIntervalInMilliseconds How often to sample temperature (ms).
     * @param timerGroupIndex Hardware timer group index (0-3).
     */
    PipeTemperatureMonitor(int dataPin, int redPin, int greenPin, int bluePin, int buzzerPin,
                           unsigned long samplingIntervalInMilliseconds,
                           uint8_t timerGroupIndex = 0)
        : Device(samplingIntervalInMilliseconds, timerGroupIndex),
          pipeSensor(dataPin, this),
          statusLed(redPin, greenPin, bluePin, false, this),
          alarmBuzzer(buzzerPin, this),
          alarmActive(false)
    {
        initializeAsynchronousEngine(SAMPLER_QUEUE_LENGTH);

        // Configure fault detection: 3 consecutive failures triggers fault event
        pipeSensor.setFaultThreshold(3);

        appendSensorToScheduler(&pipeSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    }

    ~PipeTemperatureMonitor() override = default;

    /**
     * @brief Handles sensor data events and fault/recovery events.
     *
     * @param event The triggered event context payload.
     * @runs_on{sampler}
     */
    void on(Event event) override {
        if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
            float temp = pipeSensor.getTemperatureInCelsius();
            updateAlarmState(temp);
        } else if (event.identifier == Sensor::SENSOR_FAULT_EVENT_IDENTIFIER) {
            Serial.printf("[PipeTemperatureMonitor] Sensor fault detected (source: %d)\n",
                          event.sourceId);
            // Flash red to indicate sensor fault
            statusLed.setColor(RgbLed::RED);
            statusLed.handle(RgbLed::SET_COLOR_COMMAND);
            statusLed.handle(RgbLed::TOGGLE_LED_COMMAND);
        } else if (event.identifier == Sensor::SENSOR_RECOVERED_EVENT_IDENTIFIER) {
            Serial.printf("[PipeTemperatureMonitor] Sensor recovered (source: %d)\n",
                          event.sourceId);
        }
    }

    void handle(Command command) override {
        // This device does not process external commands.
        (void)command;
    }

    /**
     * @brief Initializes the device and sets the initial LED state.
     * @return True if the sensor is responding.
     */
    bool start() {
        // Set initial LED to green (normal state)
        statusLed.setColor(RgbLed::GREEN);
        statusLed.handle(RgbLed::SET_COLOR_COMMAND);

        Serial.println("[PipeTemperatureMonitor] Pipe freeze monitor online.");
        Serial.printf("[PipeTemperatureMonitor] Warning threshold: %.1f°C\n",
                      FREEZE_WARNING_THRESHOLD);
        Serial.printf("[PipeTemperatureMonitor] Alarm threshold: %.1f°C\n",
                      FREEZE_ALARM_THRESHOLD);
        return true;
    }
};
//@} // Application Mediator Implementation

/** @name Top-Level Declarative Canvas */
//@{

static const int DS18B20_DATA_PIN = 4;    ///< GPIO pin for DS18B20 1-Wire data line.
static const int RGB_LED_RED_PIN = 25;    ///< PWM pin for RGB LED red channel.
static const int RGB_LED_GREEN_PIN = 26;  ///< PWM pin for RGB LED green channel.
static const int RGB_LED_BLUE_PIN = 27;   ///< PWM pin for RGB LED blue channel.
static const int BUZZER_PIN = 15;         ///< GPIO pin for the buzzer.

static const unsigned long SAMPLING_INTERVAL_IN_MILLISECONDS = 1000;
static const uint8_t HARDWARE_TIMER_CHANNEL_INDEX = 0;

static PipeTemperatureMonitor* pipeMonitor = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    pipeMonitor = new PipeTemperatureMonitor(
        DS18B20_DATA_PIN,
        RGB_LED_RED_PIN,
        RGB_LED_GREEN_PIN,
        RGB_LED_BLUE_PIN,
        BUZZER_PIN,
        SAMPLING_INTERVAL_IN_MILLISECONDS,
        HARDWARE_TIMER_CHANNEL_INDEX
    );

    if (!pipeMonitor->start()) {
        Serial.println("[PipeTemperatureMonitor] Device startup failed.");
        return;
    }
}

void loop() {
    // The master loop is completely clean and empty.
    // The framework handles all timing, sensor sampling, and alarm control.
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
//@} // Top-Level Declarative Canvas
