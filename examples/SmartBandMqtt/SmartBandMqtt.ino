/**
 * @file SmartBandMqtt.ino
 * @brief SmartBand Embedded Application entry point for the Modest-IoT Nano-Framework.
 * @date 2026-06-12
 * @version 1.0
 */

#include <Arduino.h>
#include <memory>
#include "ModestIoT.h"

/** @name Concrete Application Data Schema Specification */
//@{

class SmartBandTelemetryPackage : public TelemetryPackage {
    private:
    int heartBeatsPerMinute;
    float skinTemperatureInCelsius;
    float relativeHumidityInPercentage;

    public:
    SmartBandTelemetryPackage(int heartRate, float temperature, float humidity)
        : heartBeatsPerMinute(heartRate),
        skinTemperatureInCelsius(temperature),
        relativeHumidityInPercentage(humidity) {}

    ~SmartBandTelemetryPackage() override = default;

    void serialize(JsonDocument& serializationDestination) const override {
        serializationDestination["heartBeatsPerMinute"]       = this->heartBeatsPerMinute;
        serializationDestination["skinTemperatureInCelsius"]   = this->skinTemperatureInCelsius;
        serializationDestination["relativeHumidityPercentage"] = this->relativeHumidityInPercentage;
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

        initializeAsynchronousEngine(10);

        // Sensors are registered for periodic polling; the button uses interrupt-driven mode
        appendSensorToScheduler(&heartRateSensor,       Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        appendSensorToScheduler(&ambientClimateSensor,  Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
        appendSensorToTriggeringScheduler(&userActionButton,
                                          Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER,
                                          FALLING);
    }

    ~SmartBandDevice() override = default;

    void on(Event event) override {
        if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
            packageAndQueueTelemetrySnapshot();
        }
        else if (event.identifier == Button::BUTTON_PRESSED_EVENT_IDENTIFIER) {
            Serial.println("[SmartBand UI] Action button click registered safely out of ISR!");
        }
    }

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

    Serial.println("[SmartBand Core] Thread execution loop online. Architecture finalized.");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}