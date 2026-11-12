/**
 * @file HomeSecurityDevice.ino
 * @brief Demonstrates a home security monitoring station.
 *
 * Bundles WaterLeakSensor, DoorWindowSensor, PassiveInfraredSensor, and Buzzer
 * to create a complete home security device. Water leak sensors detect flooding,
 * door/window sensors monitor perimeter access, PIR sensors detect motion, and
 * the buzzer provides audible alerts.
 *
 * All sensors use interrupt-driven operation via appendSensorToTriggeringScheduler()
 * for zero-latency event detection.
 */

#include <ModestIoT.h>

/** @brief Serial communication baud rate. */
static const unsigned long SERIAL_BAUD_RATE = 115200;

/** @brief Delay in the main loop when no work is pending. */
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;

/** @brief Sampler event queue length. */
static const int ASYNC_ENGINE_QUEUE_DEPTH = 5;

/** @brief Sampling interval for security sensors (ms). */
static const unsigned long SAMPLING_INTERVAL_IN_MILLISECONDS = 200;

/** @name GPIO Pin Assignments */
//@{
static const int WATER_LEAK_SENSOR_PIN = 4;
static const int DOOR_WINDOW_SENSOR_PIN = 5;
static const int PIR_SENSOR_PIN = 18;
static const int BUZZER_PIN = 25;
//@} // GPIO Pin Assignments

class HomeSecurityDevice : public Device {
private:
    WaterLeakSensor waterLeakSensor;
    DoorWindowSensor doorWindowSensor;
    PassiveInfraredSensor pirSensor;
    Buzzer buzzer;

public:
    HomeSecurityDevice()
        : Device(SAMPLING_INTERVAL_IN_MILLISECONDS, 0)
        , waterLeakSensor(WATER_LEAK_SENSOR_PIN, this)
        , doorWindowSensor(DOOR_WINDOW_SENSOR_PIN, this)
        , pirSensor(PIR_SENSOR_PIN, this)
        , buzzer(BUZZER_PIN, this)
    {
        // Launch internal background task workers
        initializeAsynchronousEngine(ASYNC_ENGINE_QUEUE_DEPTH);

        // Register all sensors with interrupt-driven operation (CHANGE edge)
        appendSensorToTriggeringScheduler(
            &waterLeakSensor, WaterLeakSensor::WATER_LEAK_DETECTED_EVENT_IDENTIFIER, CHANGE);
        appendSensorToTriggeringScheduler(
            &doorWindowSensor, DoorWindowSensor::DOOR_OPENED_EVENT_IDENTIFIER, CHANGE);
        appendSensorToTriggeringScheduler(
            &pirSensor, PassiveInfraredSensor::MOTION_DETECTED_EVENT_IDENTIFIER, CHANGE);
    }

protected:
    void on(Event event) override {
        switch (event.identifier) {
            case WaterLeakSensor::WATER_LEAK_DETECTED_EVENT_IDENTIFIER:
                Serial.println("[HomeSecurity] ⚠️  WATER LEAK DETECTED!");
                buzzer.handle(Buzzer::PLAY_TONE_COMMAND);
                break;
            case WaterLeakSensor::WATER_LEAK_CLEARED_EVENT_IDENTIFIER:
                Serial.println("[HomeSecurity] ✓ Water leak cleared.");
                break;
            case DoorWindowSensor::DOOR_OPENED_EVENT_IDENTIFIER:
                Serial.println("[HomeSecurity] 🚪 Door/window opened!");
                buzzer.handle(Buzzer::PLAY_TONE_COMMAND);
                break;
            case DoorWindowSensor::DOOR_CLOSED_EVENT_IDENTIFIER:
                Serial.println("[HomeSecurity] 🚪 Door/window closed.");
                break;
            case PassiveInfraredSensor::MOTION_DETECTED_EVENT_IDENTIFIER:
                Serial.println("[HomeSecurity] 🏃 Motion detected!");
                buzzer.handle(Buzzer::PLAY_TONE_COMMAND);
                break;
            case PassiveInfraredSensor::MOTION_STOPPED_EVENT_IDENTIFIER:
                Serial.println("[HomeSecurity] Motion stopped.");
                break;
            default:
                break;
        }
    }
};

// Application instance
static HomeSecurityDevice* homeSecurityApp = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println("[HomeSecurity] Initializing home security system...");

    // Boot the framework application
    homeSecurityApp = new HomeSecurityDevice();

    Serial.println("[HomeSecurity] System armed. Monitoring...");
}

void loop() {
    // All event handling is interrupt-driven; main loop remains idle.
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
