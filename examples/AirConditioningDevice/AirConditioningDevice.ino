/**
 * @file AirConditioningDevice.ino
 * @brief Complex example demonstrating multi-input, multi-output coordination.
 * Uses a DHT sensor, two adjustment buttons, a 16x2 I2C LCD, and a Servo Motor
 * to build an event-driven temperature control loop under Modest-IoT.
 * @date 2026-06-12
 * @version 1.0
 */

#include <Arduino.h>
#include <ModestIoT.h>

/** @brief Serial communication baud rate. */
static const unsigned long SERIAL_BAUD_RATE = 115200;

/** @brief Delay in the main loop when no work is pending. */
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;

/** @brief Sampling interval for the DHT sensor (ms). */
static const unsigned long SAMPLING_INTERVAL_IN_MILLISECONDS = 2000;

/** @brief Debounce time for buttons (ms). */
static const unsigned long BUTTON_DEBOUNCE_MS = 150;

/** @brief Sampler event queue length. */
static const int ASYNC_ENGINE_QUEUE_DEPTH = 8;

/** @brief Default target temperature (°C). */
static const int DEFAULT_TARGET_TEMPERATURE = 22;

/** @name Pin Definitions */
//@{

static const int HUMIDITY_AND_TEMPERATURE_SENSOR_PIN    = 4;
static const int TARGET_TEMPERATURE_UP_BUTTON_PIN       = 12;
static const int TARGET_TEMPERATURE_DOWN_BUTTON_PIN     = 14;
static const int STATUS_DISPLAY_ADDRESS                 = 0x27;
static const int AIR_HANDLER_PIN                        = 13;
//@} // Pin Definitions

/** @name Application Mediator Implementation */
//@{

class AirConditioningDevice : public Device {
    private:
    // Inputs (Sensors)
    DhtSensor humidityAndTemperatureSensor;
    Button targetTemperatureUpButton;
    Button targetTemperatureDownButton;

    // Outputs (Actuators)
    CharacterLcdDisplay statusDisplay;
    ServoMotor airHandler;

    // Internal Application State Context
    float currentTemperature;
    float currentHumidity;
    int targetTemperature;

    /**
     * @brief Formats and updates the first line of the LCD buffer (Line 0).
     * Focuses strictly on real-time environmental metrics.
     */
    void updateCurrentMeasureDisplay() {
        char currentMeasureBuffer[17];
        snprintf(currentMeasureBuffer, sizeof(currentMeasureBuffer), "Cur: %.1fC  %.0f%%",
            currentTemperature, currentHumidity);

        statusDisplay.setLineBuffer(0, currentMeasureBuffer);
        statusDisplay.handle(CharacterLcdDisplay::UPDATE_TEXT_COMMAND);
    }

    /**
     * @brief Formats and updates the second line of the LCD buffer (Line 1).
     * Focuses strictly on user-targeted baseline configurations.
     */
    void updateTargetMeasureDisplay() {
        char targetMeasureBuffer[17];
        snprintf(targetMeasureBuffer, sizeof(targetMeasureBuffer), "Target: %dC", targetTemperature);

        statusDisplay.setLineBuffer(1, targetMeasureBuffer);
        statusDisplay.handle(CharacterLcdDisplay::UPDATE_TEXT_COMMAND);
    }

    /**
     * @brief Adjusts the air vent angle based on current vs target error offsets.
     */
    void evaluateAirHandlerOperationState() {
        // Open the vent fully if room temperature exceeds target; otherwise close it safely
        if (currentTemperature > targetTemperature) {
            airHandler.handle(ServoMotor::LOCK_TO_MAXIMUM_LIMIT_COMMAND);
        } else {
            airHandler.handle(ServoMotor::LOCK_TO_MINIMUM_LIMIT_COMMAND);
        }
    }

    public:

    /**
     * @param dhtPin GPIO pin connected to the DHT climate module.
     * @param upPin GPIO pin linked to the Increase temperature button.
     * @param downPin GPIO pin linked to the Decrease temperature button.
     * @param lcdAddr I2C device address for the character display.
     * @param servoPin GPIO PWM output pin driving the vent servo.
     */
    AirConditioningDevice(int dhtPin, int upPin, int downPin, uint8_t lcdAddr, int servoPin)
        : Device(SAMPLING_INTERVAL_IN_MILLISECONDS, 0),
        humidityAndTemperatureSensor(dhtPin, DhtSensor::SENSOR_MODEL_DHT22),
        targetTemperatureUpButton(upPin, BUTTON_DEBOUNCE_MS, this),
        targetTemperatureDownButton(downPin, BUTTON_DEBOUNCE_MS, this),
        statusDisplay(lcdAddr, 16, 2, this),
        airHandler(servoPin, 0, this),
        currentTemperature(0.0f),
        currentHumidity(0.0f),
        targetTemperature(DEFAULT_TARGET_TEMPERATURE)
    {
        // 1. Launch FreeRTOS execution background engines
        initializeAsynchronousEngine(ASYNC_ENGINE_QUEUE_DEPTH);

        // 2. Register the DHT sensor using the baseline scheduler ticker token
        appendSensorToScheduler(&humidityAndTemperatureSensor,  Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);

        // 3. Register buttons for interrupt-driven operation (FALLING edge)
        appendSensorToTriggeringScheduler(&targetTemperatureUpButton,
                                          Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER,
                                          FALLING);
        appendSensorToTriggeringScheduler(&targetTemperatureDownButton,
                                          Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER,
                                          FALLING);

        // 4. Initial rendering pass to populate both display lines
        updateCurrentMeasureDisplay();
        updateTargetMeasureDisplay();
    }

    /**
     * @brief Inversion of Control interface contract handler.
     * Intercepts framework events dispatched from background tasks.
     * @param event The triggered event context payload.
     */
    void on(Event event) override {

        // Context A: DHT Sensor successfully finalized data acquisition
        if (event.identifier == DhtSensor::DATA_READ_EVENT_IDENTIFIER) {
            currentTemperature = humidityAndTemperatureSensor.getTemperatureInCelsius();
            currentHumidity = humidityAndTemperatureSensor.getRelativeHumidityInPercentage();
            updateCurrentMeasureDisplay();
        }

        // Context B & C: User pressed one ot the buttons
        else if (event.identifier == Button::BUTTON_PRESSED_EVENT_IDENTIFIER) {
            char* buttonContext = nullptr;
            if (event.sourceId == TARGET_TEMPERATURE_UP_BUTTON_PIN) {
                targetTemperature++;
                buttonContext = "Increase";
            } else if (event.sourceId == TARGET_TEMPERATURE_DOWN_BUTTON_PIN) {
                targetTemperature--;
                buttonContext = "Decrease";
            }
            updateTargetMeasureDisplay();
            Serial.printf("[AC Device] Target temperature %s button pressed.\n", buttonContext);
        }
        // Optimized Sequence: Evaluate control loops at the very end.
        // Ensures physical updates operate exclusively on fully settled states.
        evaluateAirHandlerOperationState();
    }
};
//@} // Application Mediator Implementation

/** @name Top-Level Declarative Canvas */
//@{

static AirConditioningDevice* airConditioningController = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    // Boot complete application context framework dynamically
    airConditioningController = new AirConditioningDevice(
        HUMIDITY_AND_TEMPERATURE_SENSOR_PIN,
        TARGET_TEMPERATURE_UP_BUTTON_PIN,
        TARGET_TEMPERATURE_DOWN_BUTTON_PIN,
        STATUS_DISPLAY_ADDRESS,
        AIR_HANDLER_PIN
    );

    Serial.println("[System Boot] Loop-less Air Conditioning Orchestrator active.");
}

void loop() {
    // 100% of execution resources are permanently yielded to background FreeRTOS workers
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
//@} // Top-Level Declarative Canvas