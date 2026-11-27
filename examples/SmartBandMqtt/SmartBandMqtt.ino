/**
 * @file SmartBandMqtt.ino
 * @brief Wearable fitness band that monitors heart rate and ambient climate, publishing via MQTT.
 *
 * Combines a PulseSensor for heart rate, a DHT22 for temperature/humidity, and a Button
 * for user interaction. All readings are packaged into a TelemetryPackage and published
 * over MQTT to a cloud broker. Demonstrates the full telemetry pipeline from sensor to cloud.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `Device` base class with async engine and MQTT gateway integration
 * - `PulseSensor` (analog), `DhtSensor` (digital), `Button` (interrupt-driven) sensors
 * - `TelemetryPackage` subclass with custom `serialize()` for JSON schema
 * - `MqttGatewayClient` for cloud telemetry publishing
 * - `WiFiConnectivityDriver` for WiFi link management
 * - Cross-task event routing via `on(Event)` handler
 * - `@runs_on{sampler}` threading context documentation
 * @date 2026-06-12
 * @version 1.0
 */

#include <Arduino.h>
#include <memory>
#include "ModestIoT.h"

/** @name Telemetry JSON Keys */
//@{
/** @brief JSON key for heart rate in beats per minute. */
#define JSON_KEY_HEART_BEATS_PER_MINUTE "heartBeatsPerMinute"
/** @brief JSON key for skin temperature in Celsius. */
#define JSON_KEY_SKIN_TEMPERATURE_IN_CELSIUS "skinTemperatureInCelsius"
/** @brief JSON key for relative humidity in percentage. */
#define JSON_KEY_RELATIVE_HUMIDITY_PERCENTAGE "relativeHumidityPercentage"
//@} // Telemetry JSON Keys

/** @name Concrete Application Data Schema Specification */
//@{

class SmartBandTelemetryPackage : public TelemetryPackage {
public:
    /** @brief Schema version for this telemetry format. */
    static constexpr const char* SCHEMA_VERSION = "1.0";

    private:
    int32_t heartBeatsPerMinute;
    float skinTemperatureInCelsius;
    float relativeHumidityInPercentage;

    public:
    SmartBandTelemetryPackage(int heartRate, float temperature, float humidity)
        : heartBeatsPerMinute(heartRate),
        skinTemperatureInCelsius(temperature),
        relativeHumidityInPercentage(humidity)
    {
        schemaVersionValue = SCHEMA_VERSION;
    }

    ~SmartBandTelemetryPackage() override = default;

    void write(TelemetryWriter& writer) const override {
        writer.field(JSON_KEY_HEART_BEATS_PER_MINUTE, this->heartBeatsPerMinute);
        writer.field(JSON_KEY_SKIN_TEMPERATURE_IN_CELSIUS, this->skinTemperatureInCelsius);
        writer.field(JSON_KEY_RELATIVE_HUMIDITY_PERCENTAGE, this->relativeHumidityInPercentage);
    }
};
//@} // Concrete Application Data Schema Specification

/** @name Application Mediator Implementation */
//@{

class SmartBandDevice : public Device {
    private:
    PulseSensor heartRateSensor;
    DhtSensor ambientClimateSensor;
    Button userActionButton; // Clean polled button integration
    MqttGatewayClient& gatewayClient;

    void packageAndQueueTelemetrySnapshot() {
        auto packet = std::make_unique<SmartBandTelemetryPackage>(
            heartRateSensor.getBeatsPerMinute(),
            ambientClimateSensor.getTemperatureInCelsius(),
            ambientClimateSensor.getRelativeHumidityInPercentage()
        );
        enqueueTelemetryPayload(std::move(packet));
    }

    protected:
    void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const override {
        if (rawQueueItemPayload != nullptr) {
            gatewayClient.sendTelemetryRecord(*rawQueueItemPayload);
        }
    }

    void maintainNetworkServices() const override {
        gatewayClient.service();
    }

    public:
    SmartBandDevice(int pulsePin, int climatePin, int buttonPin, MqttGatewayClient& client,
    unsigned long samplingIntervalInMilliseconds, uint8_t timerGroupIndex = 0)
    : Device(samplingIntervalInMilliseconds, timerGroupIndex),
    heartRateSensor(pulsePin, nullptr),
    ambientClimateSensor(climatePin, DhtSensor::SENSOR_MODEL_DHT22, nullptr),
    userActionButton(buttonPin, 50, nullptr),
    gatewayClient(client)
    {
        heartRateSensor.setHandler(this);
        ambientClimateSensor.setHandler(this);
        userActionButton.setHandler(this);

        // Sensors are registered for periodic polling; the button uses interrupt-driven mode
        // IMPORTANT: Registration must occur before initializeAsynchronousEngine(), because
        // the engine start guard rejects any registration after the engine is running.
        appendSensorToScheduler(&heartRateSensor,       Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        appendSensorToScheduler(&ambientClimateSensor,  Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        appendSensorToTriggeringScheduler(&userActionButton,
                                          Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER,
                                          FALLING);

        if (!initializeAsynchronousEngine(10)) {
            return;
        }
    }

    ~SmartBandDevice() override {
        stopAsynchronousEngine();
    }

    /**
     * @brief Handles sensor data-ready and button press events.
     * @param event The triggered event context payload.
     * @runs_on{sampler}
     */
    void on(Event event) override {
        if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
            packageAndQueueTelemetrySnapshot();
        }
        else if (event.identifier == Button::BUTTON_PRESSED_EVENT_IDENTIFIER) {
            Serial.println("[SmartBand UI] Action button click registered safely out of ISR!");
        }
    }

    /**
     * @brief Handles commands (unused in this example).
     * @param command The incoming command.
     * @runs_on{sampler}
     */
    void handle(Command command) override {}
};
//@} // Application Mediator Implementation

/** @name Top-Level Declarative Canvas */
//@{

static const int BIOLOGICAL_PULSE_ANALOG_PIN = 34;
static const int CLIMATE_ONE_WIRE_BUS_PIN    = 4;
static const int USER_INTERFACE_BUTTON_PIN   = 14;

static const char* WIRELESS_SSID             = "SmartBand_Secure_Mesh";
static const char* WIRELESS_PASSWORD         = "TelemetryNodePass2026";
static const char* MQTT_BROKER_IP            = "192.168.1.200";
static const uint16_t MQTT_BROKER_PORT       = 1883;
static const char* MQTT_PUBLISH_TOPIC        = "nodes/smartband/vitals";

static const unsigned long TELEMETRY_POLLING_INTERVAL_IN_MILLISECONDS = 1000;
static const uint8_t HARDWARE_TIMER_CHANNEL_INDEX = 0;

/** @brief Serial communication baud rate. */
static const unsigned long SERIAL_BAUD_RATE = 115200;

/** @brief Delay in the main loop when no work is pending. */
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;

static WiFiConnectivityDriver* transportDriver = nullptr;
static MqttGatewayClient* gatewayClient        = nullptr;
static SmartBandDevice* smartBandApplication   = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    Device::setFirmwareVersion("SBM-606 FW 1.0.0");

    transportDriver = new WiFiConnectivityDriver(WIRELESS_SSID, WIRELESS_PASSWORD);

    gatewayClient = new MqttGatewayClient(
    *transportDriver,
    transportDriver->getNetworkSocket(),
        MQTT_BROKER_IP,
        MQTT_BROKER_PORT,
        MQTT_PUBLISH_TOPIC,
        "SmartBand_Wrist_01"
    );

    smartBandApplication = new SmartBandDevice(
        BIOLOGICAL_PULSE_ANALOG_PIN,
        CLIMATE_ONE_WIRE_BUS_PIN,
        USER_INTERFACE_BUTTON_PIN,
        *gatewayClient,
        TELEMETRY_POLLING_INTERVAL_IN_MILLISECONDS,
        HARDWARE_TIMER_CHANNEL_INDEX
    );

    if (!smartBandApplication->isAsynchronousEngineStarted()) {
        Serial.println("[SmartBand] Engine startup failed; telemetry was not started.");
        return;
    }

    Serial.println("[SmartBand Core] Thread execution loop online. Architecture finalized.");
}

void loop() {
    // The master loop is completely clean and empty.
    // The framework handles all timing, sensor sampling, and MQTT telemetry asynchronously.
    // vTaskDelay clarifies that this loop yields to FreeRTOS — the device is loopless.
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}