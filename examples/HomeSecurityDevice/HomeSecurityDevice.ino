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

// GPIO pin assignments
#define WATER_LEAK_SENSOR_PIN 4
#define DOOR_WINDOW_SENSOR_PIN 5
#define PIR_SENSOR_PIN 18
#define BUZZER_PIN 25

class HomeSecurityDevice : public Device {
private:
    WaterLeakSensor waterLeakSensor;
    DoorWindowSensor doorWindowSensor;
    PassiveInfraredSensor pirSensor;
    Buzzer buzzer;

public:
    HomeSecurityDevice()
        : Device(200, 0)
        , waterLeakSensor(WATER_LEAK_SENSOR_PIN, this)
        , doorWindowSensor(DOOR_WINDOW_SENSOR_PIN, this)
        , pirSensor(PIR_SENSOR_PIN, this)
        , buzzer(BUZZER_PIN, this)
    {
        // Launch internal background task workers with a safe queue boundary depth of 5
        initializeAsynchronousEngine(5);

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
    Serial.begin(115200);
    Serial.println("[HomeSecurity] Initializing home security system...");

    // Boot the framework application
    homeSecurityApp = new HomeSecurityDevice();

    Serial.println("[HomeSecurity] System armed. Monitoring...");
}

void loop() {
    // All event handling is interrupt-driven; main loop remains idle.
    vTaskDelay(pdMS_TO_TICKS(60000));
}
