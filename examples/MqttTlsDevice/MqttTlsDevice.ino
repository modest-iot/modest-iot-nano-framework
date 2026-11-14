/**
 * @file MqttTlsDevice.ino
 * @brief Secure environmental monitor that transmits telemetry over encrypted MQTT (TLS).
 *
 * @details This example demonstrates MQTT over TLS using `WiFiConnectivityDriver::createSecureSocket()`
 *          to create and configure a `WiFiClientSecure` in one call. The device monitors temperature,
 *          humidity, and pressure via a BME280 sensor and publishes encrypted telemetry to an MQTT
 *          broker supporting TLS (e.g., port 8883).
 *
 * Modest IoT Nano Framework aspects illustrated:
 * - `MqttGatewayClient` with `WiFiClientSecure` for TLS 1.2/1.3 encrypted MQTT
 * - `WiFiConnectivityDriver::createSecureSocket()` builder method for TLS socket creation
 * - `PthSensor` (BME280) for environmental telemetry
 * - `TelemetryPackage` serialization and queue-based publishing
 * - `Device` mediator pattern with async engine and per-slot scheduler
 * - Empty `loop()` — all execution is event-driven and async
 *
 * @note For production use, pass a CA certificate to `createSecureSocket()` rather than
 *       enabling `insecure` mode. Self-signed certificates should only be used for testing.
 *
 * @date 2026-11-12
 * @version 1.0
 */

#include <Arduino.h>
#include <memory>
#include "ModestIoT.h"

/** @brief Sampler event queue length for MQTT TLS telemetry. */
static const int MQTT_TLS_SAMPLER_QUEUE_LENGTH = 10;

/** @name Telemetry JSON Keys */
//@{
/** @brief JSON key for temperature in Celsius. */
#define JSON_KEY_TEMPERATURE_CELSIUS "temperatureCelsius"
/** @brief JSON key for humidity in percentage. */
#define JSON_KEY_HUMIDITY_PERCENTAGE "humidityPercentage"
/** @brief JSON key for pressure in hectopascals. */
#define JSON_KEY_PRESSURE_HECTOPASCALS "pressureHectopascals"
//@} // Telemetry JSON Keys

/** @name Concrete Application Data Schema Specification */
//@{

class EnvironmentalTelemetryPackage : public TelemetryPackage {
    private:
    float temperatureInCelsius;
    float humidityInPercentage;
    float pressureInHectopascals;

    public:
    EnvironmentalTelemetryPackage(float temperature, float humidity, float pressure)
        : temperatureInCelsius(temperature),
          humidityInPercentage(humidity),
          pressureInHectopascals(pressure) {}

    ~EnvironmentalTelemetryPackage() override = default;

    void serialize(JsonDocument& serializationDestination) const override {
        serializationDestination[JSON_KEY_TEMPERATURE_CELSIUS]   = this->temperatureInCelsius;
        serializationDestination[JSON_KEY_HUMIDITY_PERCENTAGE]   = this->humidityInPercentage;
        serializationDestination[JSON_KEY_PRESSURE_HECTOPASCALS] = this->pressureInHectopascals;
    }
};
//@} // Concrete Application Data Schema Specification

/** @name Application Mediator Implementation */
//@{

class MqttTlsDevice : public Device {
    private:
    PthSensor environmentalSensor;
    MqttGatewayClient& gatewayClient;

    void packageAndQueueTelemetrySnapshot() {
        auto packet = std::make_unique<EnvironmentalTelemetryPackage>(
            environmentalSensor.getTemperatureInCelsius(),
            environmentalSensor.getHumidityInPercentage(),
            environmentalSensor.getPressureInHectopascals()
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
    MqttTlsDevice(int sensorPin, MqttGatewayClient& client,
                  unsigned long samplingIntervalInMilliseconds, uint8_t timerGroupIndex = 0)
        : Device(samplingIntervalInMilliseconds, timerGroupIndex),
          environmentalSensor(sensorPin, PthSensor::DEFAULT_I2C_ADDRESS, this),
          gatewayClient(client)
    {
        initializeAsynchronousEngine(MQTT_TLS_SAMPLER_QUEUE_LENGTH);

        appendSensorToScheduler(&environmentalSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    }

    ~MqttTlsDevice() override = default;

    void on(Event event) override {
        if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
            packageAndQueueTelemetrySnapshot();
        }
    }

    void handle(Command command) override {}
};
//@} // Application Mediator Implementation

/** @name Top-Level Declarative Canvas */
//@{

static const int ENVIRONMENTAL_SENSOR_I2C_PIN = 21;

static const char* WIRELESS_SSID              = "SecureNetwork";
static const char* WIRELESS_PASSWORD          = "SecurePassword123";

/** @brief MQTT broker address supporting TLS (e.g., test.mosquitto.org). */
static const char* MQTT_BROKER_IP             = "test.mosquitto.org";

/** @brief Standard MQTT over TLS port. */
static const uint16_t MQTT_TLS_PORT           = 8883;

static const char* MQTT_PUBLISH_TOPIC         = "nodes/environmental/tls-telemetry";

static const unsigned long TELEMETRY_POLLING_INTERVAL_IN_MILLISECONDS = 30000;
static const uint8_t HARDWARE_TIMER_CHANNEL_INDEX = 0;

/** @brief Serial communication baud rate. */
static const unsigned long SERIAL_BAUD_RATE = 115200;

/** @brief Delay in the main loop when no work is pending. */
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;

static WiFiConnectivityDriver* transportDriver = nullptr;
static WiFiClientSecure* secureSocket          = nullptr;
static MqttGatewayClient* gatewayClient        = nullptr;
static MqttTlsDevice* tlsApplication           = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    transportDriver = new WiFiConnectivityDriver(WIRELESS_SSID, WIRELESS_PASSWORD);

    /**
     * @brief Create and configure a TLS-encrypted socket using the builder method.
     *
     * @warning `insecure = true` disables certificate verification — use only for testing.
     *          For production, pass a CA certificate:
     *          @code
     *          static const char caCert[] = R"EOF(
     *          -----BEGIN CERTIFICATE-----
     *          MIID... (base64-encoded certificate) ...
     *          -----END CERTIFICATE-----
     *          )EOF";
     *          secureSocket = transportDriver->createSecureSocket(caCert, false);
     *          @endcode
     */
    secureSocket = transportDriver->createSecureSocket(nullptr, true);

    /**
     * @brief Construct MqttGatewayClient with the TLS-encrypted socket.
     *
     * The broker port is 8883 (MQTT over TLS) instead of 1883 (plain MQTT).
     * All other parameters are identical to plain MQTT configuration.
     */
    gatewayClient = new MqttGatewayClient(
        *transportDriver,
        *secureSocket,
        MQTT_BROKER_IP,
        MQTT_TLS_PORT,
        MQTT_PUBLISH_TOPIC,
        "MqttTlsDevice_01"
    );

    tlsApplication = new MqttTlsDevice(
        ENVIRONMENTAL_SENSOR_I2C_PIN,
        *gatewayClient,
        TELEMETRY_POLLING_INTERVAL_IN_MILLISECONDS,
        HARDWARE_TIMER_CHANNEL_INDEX
    );

    Serial.println("[MqttTlsDevice] TLS-encrypted MQTT environmental monitor online.");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
