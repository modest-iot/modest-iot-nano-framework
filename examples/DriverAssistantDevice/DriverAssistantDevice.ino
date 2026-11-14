/**
 * @file DriverAssistantDevice.ino
 * @brief Vehicle driver assistant with GPS navigation and parking distance alerts.
 *
 * Combines a GPS module for speed tracking and two ultrasonic sensors (front and rear)
 * for parking assistance. Displays speed and distance readings on a 16x2 LCD. The GPS
 * uses a dedicated HardwareSerial port, while ultrasonic sensors use GPIO pins for
 * trigger/echo timing.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `Device` base class with async engine for loop-less execution
 * - `GeolocationSensor` with `HardwareSerial` for GPS NMEA parsing
 * - `UltrasonicSensor` (two instances) for front/rear distance measurement
 * - `CharacterLcdDisplay` actuator with `UPDATE_TEXT_COMMAND` for real-time display
 * - `event.sourceId` disambiguation for multiple sensors of the same type
 * - `LocationData` value object for GPS coordinate storage
 * - Cross-task event routing via `on(Event)` handler
 * @date 2026-06-15
 * @version 1.0
 */

#include <Arduino.h>
#include <ModestIoT.h>

// Pin definitions

static const int GPS_RX_PIN         = 16;
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

static constexpr float KNOTS_TO_KPH_CONVERSION_FACTOR = 1.852f;
static constexpr float CENTIMETER_TO_METER_CONVERSION_FACTOR = 0.01f;

// Timing constants

static const unsigned long SERIAL_BAUD_RATE = 115200;

// Asynchronous engine configuration

static const int            ASYNC_ENGINE_QUEUE_DEPTH    = 8;
static const unsigned long  SAMPLING_INTERVAL_IN_MILLISECONDS        = 1000;
static const unsigned long  MAIN_LOOP_DELAY_IN_MILLISECONDS          = 60000;
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

    static constexpr float convertCentimetersToMeters(float measureInCentimeters) {
        return measureInCentimeters * CENTIMETER_TO_METER_CONVERSION_FACTOR;
    }

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
        // snprintf(locationStatusBuffer, sizeof(locationStatusBuffer), "Pos:%.1f,%.1f",
        //    lastLocation.latitudeInDegrees, lastLocation.longitudeInDegrees);
        snprintf(locationStatusBuffer, sizeof(locationStatusBuffer), "S:%.1fKmph", speedInKph);
        // Show additional information by console
        Serial.print(F("Latitude: ")); Serial.println(lastLocation.latitudeInDegrees, 6);
        Serial.print(F("Longitude: "));Serial.println(lastLocation.longitudeInDegrees, 6);
        Serial.print(F("Milliseconds: "));Serial.println(lastLocation.milliseconds, 6);
    }

        snprintf(measureStatusBuffer, sizeof(measureStatusBuffer), "F:%.1fm R:%.1fm",
        frontDistanceInMeters, rearDistanceInMeters);

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
        : Device(SAMPLING_INTERVAL_IN_MILLISECONDS, TIMER_GROUP_INDEX),
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
     * @runs_on{sampler}
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

static DriverAssistantDevice* driverAssistantApplication = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    driverAssistantApplication = new DriverAssistantDevice(
        Serial2,
        GPS_RX_PIN, GPS_TX_PIN,
        FRONT_TRIGGER_PIN, FRONT_ECHO_PIN,
        REAR_TRIGGER_PIN, REAR_ECHO_PIN,
        LCD_I2C_ADDRESS
    );
    Serial.println("[System Boot] Driver Assistant active.");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
