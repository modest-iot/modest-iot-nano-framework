/**
 * @file OutdoorSecurityLightController.ino
 * @brief Outdoor security light that turns on only when it is dark and motion is detected.
 *
 * Combines a GY-302 / BH1750 DigitalLightIntensitySensor, a PassiveInfraredSensor and a Led.
 * The device mediator keeps the two latest conditions (darkness and motion) and drives the
 * light from their combination, so the light is on only while both are true.
 *
 * Sensor fault detection is enabled: after 3 consecutive read failures a sensor emits a fault
 * event and the light turns off as a safe default. When the sensor recovers, normal operation
 * resumes.
 *
 * Wiring (ESP32): BH1750 SDA -> GPIO 21, SCL -> GPIO 22, ADDR floating or low (0x23).
 * Mount the light sensor so the controlled light does not illuminate it; otherwise turning
 * the light on can be read as daylight and switch it off again.
 */

#include <Arduino.h>
#include <ModestIoT.h>

// Pin definitions
static const int     LIGHT_SENSOR_SOURCE_ID     = 21;  // Event source ID for the I2C light sensor (SDA pin)
static const int     MOTION_SENSOR_PIN          = 4;
static const int     SECURITY_LED_PIN           = 5;
static const uint8_t LIGHT_SENSOR_I2C_ADDRESS   = DigitalLightIntensitySensor::DEFAULT_I2C_ADDRESS;

// Behavior
static const float   DARKNESS_THRESHOLD_IN_LUX  = 10.0f;

// Fault detection: emit fault event after this many consecutive read failures
static const uint8_t SENSOR_FAULT_THRESHOLD     = 3;

// Timing
static const unsigned long SAMPLING_INTERVAL_IN_MILLISECONDS     = 200;
static const unsigned long DEVICE_EVENT_DELAY_IN_MILLISECONDS    = 0;
static const int           ASYNC_ENGINE_QUEUE_DEPTH = 8;
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS       = 1000;

// Initial state: bright, no motion -> light off
static const bool SECURITY_LED_INITIAL_STATE   = false;
static const bool INITIAL_IS_DARK              = false;
static const bool INITIAL_IS_MOTION_PRESENT    = false;

/**
 * @brief Outdoor Security Light Controller Device.
 */
class OutdoorSecurityLightController : public Device {
private:
    DigitalLightIntensitySensor lightSensor;
    PassiveInfraredSensor motionSensor;
    Led securityLight;

    // Until the first sensor events arrive: bright, no motion -> light off.
    bool isDark;
    bool isMotionPresent;

    // Track sensor health for safe-state behavior
    bool lightSensorFaulted = false;
    bool motionSensorFaulted = false;

    void updateSecurityLight() {
        // If either sensor is faulted, turn light off as a safe default
        if (lightSensorFaulted || motionSensorFaulted) {
            securityLight.handle(Led::TURN_OFF_COMMAND);
            return;
        }
        securityLight.handle(isDark && isMotionPresent ? Led::TURN_ON_COMMAND : Led::TURN_OFF_COMMAND);
    }

public:
    OutdoorSecurityLightController()
        : Device(SAMPLING_INTERVAL_IN_MILLISECONDS, DEVICE_EVENT_DELAY_IN_MILLISECONDS),
          lightSensor(LIGHT_SENSOR_SOURCE_ID, LIGHT_SENSOR_I2C_ADDRESS, DARKNESS_THRESHOLD_IN_LUX, this),
          motionSensor(MOTION_SENSOR_PIN, this),
          securityLight(SECURITY_LED_PIN, SECURITY_LED_INITIAL_STATE, this),
          isDark(INITIAL_IS_DARK),
          isMotionPresent(INITIAL_IS_MOTION_PRESENT)
    {
        initializeAsynchronousEngine(ASYNC_ENGINE_QUEUE_DEPTH);

        // Enable fault detection on both sensors
        lightSensor.setFaultThreshold(SENSOR_FAULT_THRESHOLD);
        motionSensor.setFaultThreshold(SENSOR_FAULT_THRESHOLD);

        appendSensorToScheduler(&lightSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        // PIR sensor uses interrupt-driven operation (CHANGE edge) for zero-latency motion detection
        appendSensorToTriggeringScheduler(&motionSensor,
                                          Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER,
                                          CHANGE);
    }

    void on(Event event) override {
        switch (event.identifier) {
            case DigitalLightIntensitySensor::LIGHT_ABSENT_EVENT_IDENTIFIER:
                isDark = true;
                break;
            case DigitalLightIntensitySensor::LIGHT_DETECTED_EVENT_IDENTIFIER:
                isDark = false;
                break;
            case PassiveInfraredSensor::MOTION_DETECTED_EVENT_IDENTIFIER:
                isMotionPresent = true;
                break;
            case PassiveInfraredSensor::MOTION_STOPPED_EVENT_IDENTIFIER:
                isMotionPresent = false;
                break;
            case Sensor::SENSOR_FAULT_EVENT_IDENTIFIER:
                // React to sensor fault by tracking which sensor failed
                if (event.sourceId == LIGHT_SENSOR_SOURCE_ID) {
                    lightSensorFaulted = true;
                } else if (event.sourceId == MOTION_SENSOR_PIN) {
                    motionSensorFaulted = true;
                }
                break;
            case Sensor::SENSOR_RECOVERED_EVENT_IDENTIFIER:
                // React to sensor recovery by clearing fault state
                if (event.sourceId == LIGHT_SENSOR_SOURCE_ID) {
                    lightSensorFaulted = false;
                } else if (event.sourceId == MOTION_SENSOR_PIN) {
                    motionSensorFaulted = false;
                }
                break;
            default:
                return;
        }
        updateSecurityLight();
    }
};

static OutdoorSecurityLightController* controller = nullptr;

void setup() {
    controller = new OutdoorSecurityLightController();
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
