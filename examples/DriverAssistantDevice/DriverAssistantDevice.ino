/**
 * @file DriverAssistantDevice.ino
 * @brief Driver assistant example using Geolocation, Ultrasonic sensors, and LCD display.
 * @date 2026-06-15
 * @version 1.0
 */

#include <Arduino.h>
#include <ModestIoT.h>

// Pin definitions

static const int GPS_RECEIVE_PIN    = 16;
static const int GPS_TX_PIN         = 17;
static const int FRONT_TRIGGER_PIN  = 4;
static const int FRONT_ECHO_PIN     = 5;
static const int REAR_TRIGGER_PIN   = 18;
static const int REAR_ECHO_PIN      = 19;
static const int LCD_I2C_ADDRESS    = 0x27;

// Configuration constants

static const int GPS_BAUD_RATE = 9600;

// LCD dimensions

static const int LCD_COLUMNS    = 16;
static const int LCD_ROWS       = 2;

// Conversion factors

static const float KNOTS_TO_KPH_CONVERSION_FACTOR = 1.852f;
static const float CENTIMETER_TO_METER_CONVERSION_FACTOR = 0.01f;

inline constexpr float convertCentimetersToMeters(float measureInCentimeters) {
    return measureInCentimeters * CENTIMETER_TO_METER_CONVERSION_FACTOR;
}

// Timing constants

static const unsigned long SERIAL_BAUD_RATE = 115200;

// Asynchronous engine configuration

static const int            ASYNC_ENGINE_QUEUE_DEPTH    = 8;
static const unsigned long  SAMPLING_INTERVAL_MS        = 1000;
static const unsigned long  MAIN_LOOP_DELAY_MS          = 60000;
static const int            TIMER_GROUP_INDEX           = 0;

/**
 * @brief Mediator for the Driver Assistant Device.
 * Coordinates Geolocation and Ultrasonic sensors to provide navigation
 * and parking assistance on an LCD display.
 */
class DriverAssistantDevice : public Device {
private:
    GeolocationSensor gpsSensor;
    UltrasonicSensor frontSensor;
    UltrasonicSensor rearSensor;
    CharacterLcdDisplay statusDisplay;

    // Internal state
    LocationData lastLocation;
    float frontDistanceInMeters;
    float rearDistanceInMeters;

    /**
     * @brief Internal state maintenance and LCD interface update.
     * Updates the status display with the last known position and sensor metrics.
     */
    void updateStatusDisplay() {
        char locationStatusBuffer[17];
        char measureStatusBuffer[17];
        
        // Speed in km/h: knots * factor
        float speedInKph = lastLocation.speedInKnots * KNOTS_TO_KPH_CONVERSION_FACTOR;
        
        if (lastLocation.fixQuality == 0) {
            snprintf(locationStatusBuffer, sizeof(locationStatusBuffer), "No GPS Fix");
        } else {
            snprintf(locationStatusBuffer, sizeof(locationStatusBuffer), "Pos:%.1f,%.1f",
                lastLocation.latitudeInDegrees, lastLocation.longitudeInDegrees);
        }
        
        snprintf(measureStatusBuffer, sizeof(measureStatusBuffer), "S:%.1f F:%.1fm R:%.1fm",
            speedInKph, frontDistanceInMeters, rearDistanceInMeters);

        statusDisplay.setLineBuffer(0, locationStatusBuffer);
        statusDisplay.setLineBuffer(1, measureStatusBuffer);
        statusDisplay.handle(CharacterLcdDisplay::UPDATE_TEXT_COMMAND);
    }

public:
    /**
     * @param gpsReceivePin GPIO pin for GPS RX.
     * @param gpsTransmitPin GPIO pin for GPS TX.
     * @param frontTriggerPin GPIO pin for Front Ultrasonic Trigger.
     * @param frontEchoPin GPIO pin for Front Ultrasonic Echo.
     * @param rearTriggerPin GPIO pin for Rear Ultrasonic Trigger.
     * @param rearEchoPin GPIO pin for Rear Ultrasonic Echo.
     * @param lcdI2cAddress I2C device address for the character display.
     */
    DriverAssistantDevice(HardwareSerial& gpsSerialPort, int gpsReceivePin, int gpsTransmitPin, int frontTriggerPin,
        int frontEchoPin, int rearTriggerPin, int rearEchoPin, uint8_t lcdI2cAddress)
        : Device(SAMPLING_INTERVAL_MS, TIMER_GROUP_INDEX),
          gpsSensor(gpsSerialPort, gpsReceivePin, gpsTransmitPin, GPS_BAUD_RATE, this),
          frontSensor(frontTriggerPin, frontEchoPin, this),
          rearSensor(rearTriggerPin, rearEchoPin, this),
          statusDisplay(lcdI2cAddress, LCD_COLUMNS, LCD_ROWS, this),
          lastLocation(),
          frontDistanceInMeters(0.0f),
          rearDistanceInMeters(0.0f)
    {
        initializeAsynchronousEngine(ASYNC_ENGINE_QUEUE_DEPTH);

        appendSensorToScheduler(&gpsSensor,     Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        appendSensorToScheduler(&frontSensor,   Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        appendSensorToScheduler(&rearSensor,    Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    }

    /**
     * @brief Inversion of Control interface contract handler.
     * Intercepts framework events dispatched from background tasks.
     * @param event The triggered event context payload.
     */
    void on(Event event) override {
        if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
            if (event.sourceId == FRONT_TRIGGER_PIN) {
                frontDistanceInMeters = convertCentimetersToMeters(frontSensor.getDistanceInCentimeters());
            } else if (event.sourceId == REAR_TRIGGER_PIN) {
                rearDistanceInMeters = convertCentimetersToMeters(rearSensor.getDistanceInCentimeters());
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
        Serial2,
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
