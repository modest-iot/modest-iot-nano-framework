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

// Event identifiers
#define WATER_LEAK_DETECTED_EVENT_IDENTIFIER 70
#define WATER_LEAK_CLEARED_EVENT_IDENTIFIER 71
#define DOOR_OPENED_EVENT_IDENTIFIER 72
#define DOOR_CLOSED_EVENT_IDENTIFIER 73
#define MOTION_DETECTED_EVENT_IDENTIFIER 40
#define MOTION_STOPPED_EVENT_IDENTIFIER 41

class HomeSecurityDevice : public Device {
private:
    WaterLeakSensor& waterLeakSensor;
    DoorWindowSensor& doorWindowSensor;
    PassiveInfraredSensor& pirSensor;
    Buzzer& buzzer;

public:
    HomeSecurityDevice(WaterLeakSensor& waterLeakSensor,
                       DoorWindowSensor& doorWindowSensor,
                       PassiveInfraredSensor& pirSensor,
                       Buzzer& buzzer)
        : Device("HomeSecurity", 200, 0, 1, 10)
        , waterLeakSensor(waterLeakSensor)
        , doorWindowSensor(doorWindowSensor)
        , pirSensor(pirSensor)
        , buzzer(buzzer)
    {
    }

protected:
    void on(Event& event) override {
        switch (event.identifier) {
            case WATER_LEAK_DETECTED_EVENT_IDENTIFIER:
                Serial.println("[HomeSecurity] ⚠️  WATER LEAK DETECTED!");
                buzzer.handle(Buzzer::PLAY_TONE_COMMAND);
                break;
            case WATER_LEAK_CLEARED_EVENT_IDENTIFIER:
                Serial.println("[HomeSecurity] ✓ Water leak cleared.");
                break;
            case DOOR_OPENED_EVENT_IDENTIFIER:
                Serial.println("[HomeSecurity] 🚪 Door/window opened!");
                buzzer.handle(Buzzer::PLAY_TONE_COMMAND);
                break;
            case DOOR_CLOSED_EVENT_IDENTIFIER:
                Serial.println("[HomeSecurity] 🚪 Door/window closed.");
                break;
            case MOTION_DETECTED_EVENT_IDENTIFIER:
                Serial.println("[HomeSecurity] 🏃 Motion detected!");
                buzzer.handle(Buzzer::PLAY_TONE_COMMAND);
                break;
            case MOTION_STOPPED_EVENT_IDENTIFIER:
                Serial.println("[HomeSecurity] Motion stopped.");
                break;
            default:
                break;
        }
    }
};

// Sensor and actuator instances
WaterLeakSensor waterLeakSensor(WATER_LEAK_SENSOR_PIN);
DoorWindowSensor doorWindowSensor(DOOR_WINDOW_SENSOR_PIN);
PassiveInfraredSensor pirSensor(PIR_SENSOR_PIN);
Buzzer buzzer(BUZZER_PIN, &waterLeakSensor);

// Application instance
HomeSecurityDevice homeSecurityApp(waterLeakSensor, doorWindowSensor, pirSensor, buzzer);

void setup() {
    Serial.begin(115200);
    Serial.println("[HomeSecurity] Initializing home security system...");

    // Register all sensors with interrupt-driven operation
    homeSecurityApp.appendSensorToTriggeringScheduler(
        &waterLeakSensor, WaterLeakSensor::WATER_LEAK_DETECTED_EVENT_IDENTIFIER, CHANGE);
    homeSecurityApp.appendSensorToTriggeringScheduler(
        &doorWindowSensor, DoorWindowSensor::DOOR_OPENED_EVENT_IDENTIFIER, CHANGE);
    homeSecurityApp.appendSensorToTriggeringScheduler(
        &pirSensor, PassiveInfraredSensor::MOTION_DETECTED_EVENT_IDENTIFIER, CHANGE);

    Serial.println("[HomeSecurity] System armed. Monitoring...");
}

void loop() {
    // All event handling is interrupt-driven; main loop remains idle.
}
