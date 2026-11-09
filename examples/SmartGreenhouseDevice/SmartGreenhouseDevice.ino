/**
 * @file SmartGreenhouseDevice.ino
 * @brief Smart greenhouse example using SoilMoisture, DHT, Relay, and ServoMotor.
 * @date 2026-11-09
 * @version 1.0
 *
 * Demonstrates event-driven greenhouse automation:
 * - SoilMoistureSensor triggers RelayActuator (water pump) when soil is dry.
 * - DhtSensor triggers ServoMotor (vent) when temperature is high.
 */

#include <Arduino.h>
#include <ModestIoT.h>

// Pin definitions

static const int SOIL_MOISTURE_PIN    = 34;
static const int DHT_DATA_PIN         = 27;
static const int RELAY_PUMP_PIN       = 26;
static const int SERVO_VENT_PIN       = 13;

// Threshold constants

static constexpr int   MOISTURE_DRY_THRESHOLD    = 300;  ///< ADC value below which soil is considered dry
static constexpr float TEMPERATURE_HIGH_THRESHOLD = 30.0f; ///< Celsius above which ventilation is needed

// Servo angles

static constexpr int VENT_CLOSED_ANGLE = 0;
static constexpr int VENT_OPEN_ANGLE   = 90;

// Timing constants

static const unsigned long SERIAL_BAUD_RATE = 115200;

// Asynchronous engine configuration

static const int            ASYNC_ENGINE_QUEUE_DEPTH    = 16;
static const unsigned long  SAMPLING_INTERVAL_MS        = 1000;
static const unsigned long  MAIN_LOOP_DELAY_MS          = 60000;
static const int            TIMER_GROUP_INDEX           = 0;

static const char*          SYSTEM_BOOT_MESSAGE         = "[System Boot] Smart Greenhouse active.";
static const char*          PUMP_ON_MESSAGE             = "[Irrigation] Pump ON — soil dry.";
static const char*          PUMP_OFF_MESSAGE            = "[Irrigation] Pump OFF — soil moist.";
static const char*          VENT_ON_MESSAGE             = "[Ventilation] Vent OPEN — temperature high.";
static const char*          VENT_OFF_MESSAGE            = "[Ventilation] Vent CLOSED — temperature normal.";

/**
 * @brief Mediator for the Smart Greenhouse Device.
 * Coordinates soil moisture and temperature sensors to control
 * a water pump (relay) and ventilation (servo motor).
 */
class SmartGreenhouseDevice : public Device {
private:
    SoilMoistureSensor soilSensor;
    DhtSensor          temperatureSensor;
    RelayActuator      waterPump;
    ServoMotor         ventMotor;

    /**
     * @brief Logs a message to Serial.
     * @param message Message string.
     */
    void logMessage(const char* message) {
        Serial.println(message);
    }

public:
    SmartGreenhouseDevice()
        : Device(SAMPLING_INTERVAL_MS, TIMER_GROUP_INDEX),
          soilSensor(SOIL_MOISTURE_PIN, this),
          temperatureSensor(DHT_DATA_PIN, DhtSensor::SENSOR_MODEL_DHT22, this),
          waterPump(RELAY_PUMP_PIN, false, this),
          ventMotor(SERVO_VENT_PIN, VENT_CLOSED_ANGLE, this)
    {
        initializeAsynchronousEngine(ASYNC_ENGINE_QUEUE_DEPTH);

        appendSensorToScheduler(&soilSensor,        Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        appendSensorToScheduler(&temperatureSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    }

    /**
     * @brief Inversion of Control interface contract handler.
     * Intercepts sensor data-ready events to control actuators.
     * @param event The triggered event context payload.
     */
    void on(Event event) override {
        if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
            if (event.sourceId == SOIL_MOISTURE_PIN) {
                // Control water pump based on soil moisture
                int moisture = soilSensor.getMoistureLevel();
                if (moisture < MOISTURE_DRY_THRESHOLD) {
                    waterPump.handle(RelayActuator::RELAY_ON_COMMAND);
                    logMessage(PUMP_ON_MESSAGE);
                } else {
                    waterPump.handle(RelayActuator::RELAY_OFF_COMMAND);
                    logMessage(PUMP_OFF_MESSAGE);
                }
            } else if (event.sourceId == DHT_DATA_PIN) {
                // Control ventilation based on temperature
                float temperature = temperatureSensor.getTemperatureInCelsius();
                if (temperature > TEMPERATURE_HIGH_THRESHOLD) {
                    ventMotor.setTargetAngleBufferInDegrees(VENT_OPEN_ANGLE);
                    ventMotor.handle(ServoMotor::MOVE_SERVO_COMMAND);
                    logMessage(VENT_ON_MESSAGE);
                } else {
                    ventMotor.setTargetAngleBufferInDegrees(VENT_CLOSED_ANGLE);
                    ventMotor.handle(ServoMotor::MOVE_SERVO_COMMAND);
                    logMessage(VENT_OFF_MESSAGE);
                }
            }
        }
    }
};

static SmartGreenhouseDevice* greenhouse = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    greenhouse = new SmartGreenhouseDevice();
    Serial.println(SYSTEM_BOOT_MESSAGE);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_MS));
}
