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

// --- 1. PIN DEFINITIONS & HARDWARE CONSTANTS ---

static const int HUMIDITY_AND_TEMPERATURE_SENSOR_PIN    = 4;
static const int TARGET_TEMPERATURE_UP_BUTTON_PIN       = 12;
static const int TARGET_TEMPERATURE_DOWN_BUTTON_PIN     = 14;
static const int STATUS_DISPLAY_ADDRESS                 = 0x27;
static const int AIR_HANDLER_PIN                        = 13;

// --- 2. EVENT-DRIVEN APPLICATION MEDIATOR ---

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
        : Device(2000, 0), // Master scheduler triggers hardware conversions every 2000ms
        humidityAndTemperatureSensor(dhtPin, 22), // Type 22 explicitly maps to a DHT22 sensor context
        targetTemperatureUpButton(upPin, 150, this),
        targetTemperatureDownButton(downPin, 150, this),
        statusDisplay(lcdAddr, 16, 2, this),
        airHandler(servoPin, 0, this),
        currentTemperature(0.0f),
        currentHumidity(0.0f),
        targetTemperature(22) // Default baseline target temperature (Celsius)
    {
        // 1. Launch FreeRTOS execution background engines (Queue depth of 8)
        initializeAsynchronousEngine(8);

        // 2. Register the DHT sensor using the baseline scheduler ticker token
        appendSensorToScheduler(&humidityAndTemperatureSensor,  Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);

        // 3. Register button handlers using their specific localized identity tokens
        appendSensorToScheduler(&targetTemperatureUpButton,     Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        appendSensorToScheduler(&targetTemperatureDownButton,   Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);

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

// --- 3. DECLARATIVE RUNTIME CANVAS ---

static AirConditioningDevice* airConditioningController = nullptr;

void setup() {
    Serial.begin(115200);

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
    vTaskDelay(pdMS_TO_TICKS(60000));
}