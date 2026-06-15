/**
 * @file DriverAssistantSystem.ino
 * @brief Driver assistant example using Geolocation, Ultrasonic sensors, and LCD display.
 * @date 2026-06-15
 * @version 1.0
 */

#include <Arduino.h>
#include <ModestIoT.h>
#include <GeolocationSensor.h>
#include <UltrasonicSensor.h>
#include <CharacterLcdDisplay.h>

static const int GPS_RECEIVE_PIN = 16;
static const int GPS_TX_PIN = 17;
static const int FRONT_TRIGGER_PIN = 4;
static const int FRONT_ECHO_PIN = 5;
static const int REAR_TRIGGER_PIN = 18;
static const int REAR_ECHO_PIN = 19;
static const int LCD_I2C_ADDRESS = 0x27;

static const unsigned long SAMPLING_INTERVAL_MS = 1000;
static const int GPS_BAUD_RATE = 9600;
static const int ASYNC_ENGINE_QUEUE_DEPTH = 8;
static const int LCD_COLUMNS = 16;
static const int LCD_ROWS = 2;
static const float KNOTS_TO_KPH_CONVERSION_FACTOR = 1.852f;
static const float CM_TO_METERS_CONVERSION_FACTOR = 0.01f;
static const unsigned long SERIAL_BAUD_RATE = 115200;
static const unsigned long MAIN_LOOP_DELAY_MS = 60000;
static const int TIMER_GROUP_INDEX = 0;

class DriverAssistantSystem : public Device {
private:
    GeolocationSensor gpsSensor;
    UltrasonicSensor frontSensor;
    UltrasonicSensor rearSensor;
    CharacterLcdDisplay statusDisplay;

    // Internal state
    LocationData lastLocation;
    float frontDistanceInMeters;
    float rearDistanceInMeters;

    void updateStatusDisplay() {
        char bufferLine0[17];
        char bufferLine1[17];
        
        // Speed in km/h: knots * factor
        float speedKph = lastLocation.speedInKnots * KNOTS_TO_KPH_CONVERSION_FACTOR;
        
        snprintf(bufferLine0, sizeof(bufferLine0), "Pos:%.1f,%.1f", lastLocation.latitudeInDegrees, lastLocation.longitudeInDegrees);
        snprintf(bufferLine1, sizeof(bufferLine1), "S:%.1f F:%.1fm R:%.1fm", speedKph, frontDistanceInMeters, rearDistanceInMeters);

        statusDisplay.setLineBuffer(0, bufferLine0);
        statusDisplay.setLineBuffer(1, bufferLine1);
        statusDisplay.handle(CharacterLcdDisplay::UPDATE_TEXT_COMMAND);
    }

public:
    DriverAssistantDevice(int rx, int tx, int frontTrig, int frontEcho, int rearTrig, int rearEcho, uint8_t lcdAddr)
        : Device(SAMPLING_INTERVAL_MS, TIMER_GROUP_INDEX),
          gpsSensor(rx, tx, GPS_BAUD_RATE, this),
          frontSensor(frontTrig, frontEcho, this),
          rearSensor(rearTrig, rearEcho, this),
          statusDisplay(lcdAddr, LCD_COLUMNS, LCD_ROWS, this),
          lastLocation(),
          frontDistanceInMeters(0.0f),
          rearDistanceInMeters(0.0f)
    {
        initializeAsynchronousEngine(ASYNC_ENGINE_QUEUE_DEPTH);

        appendSensorToScheduler(&gpsSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        appendSensorToScheduler(&frontSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        appendSensorToScheduler(&rearSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    }

    void on(Event event) override {
        if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
            if (event.sourceId == FRONT_TRIGGER_PIN) {
                frontDistanceInMeters = frontSensor.getDistanceInCentimeters() * CM_TO_METERS_CONVERSION_FACTOR;
            } else if (event.sourceId == REAR_TRIGGER_PIN) {
                rearDistanceInMeters = rearSensor.getDistanceInCentimeters() * CM_TO_METERS_CONVERSION_FACTOR;
            } else if (event.sourceId == GPS_RX_PIN) {
                lastLocation = gpsSensor.getLocationData();
            }
            updateStatusDisplay();
        }
    }
};

static DriverAssistantDevice* driverAssistant = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    driverAssistant = new DriverAssistantDevice(
        GPS_RX_PIN, GPS_TX_PIN,
        FRONT_TRIGGER_PIN, FRONT_ECHO_PIN,
        REAR_TRIGGER_PIN, REAR_ECHO_PIN,
        LCD_I2C_ADDRESS
    );
    Serial.println("[System Boot] Driver Assistant active.");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_MS));
}
