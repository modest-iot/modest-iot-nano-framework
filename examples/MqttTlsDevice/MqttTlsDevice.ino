/**
 * @file MqttTlsDevice.ino
 * @brief Secure environmental monitor that transmits telemetry over encrypted MQTT (TLS).
 *
 * @details This example demonstrates MQTT over TLS using `WiFiClientSecure` as a drop-in
 *          replacement for `WiFiClient` when constructing `MqttGatewayClient`. The device
 *          monitors temperature, humidity, and pressure via a BME280 sensor and publishes
 *          encrypted telemetry to an MQTT broker supporting TLS (e.g., port 8883).
 *
 * Modest IoT Nano Framework aspects illustrated:
 * - `MqttGatewayClient` with `WiFiClientSecure` for TLS 1.2/1.3 encrypted MQTT
 * - `PthSensor` (BME280) for environmental telemetry
 * - `WiFiConnectivityDriver` for WiFi AP connection
 * - `TelemetryPackage` serialization and queue-based publishing
 * - `Device` mediator pattern with async engine and per-slot scheduler
 * - Empty `loop()` — all execution is event-driven and async
 *
 * @note For production use, replace `setInsecure()` with `setCACert()` to pin a trusted
 *       certificate authority. Self-signed certificates should only be used for testing.
 *
 * @date 2026-11-12
 * @version 1.0
 */

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <memory>
#include "ModestIoT.h"

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
        serializationDestination["temperatureCelsius"]   = this->temperatureInCelsius;
        serializationDestination["humidityPercentage"]   = this->humidityInPercentage;
        serializationDestination["pressureHectopascals"] = this->pressureInHectopascals;
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
            environmentalSensor.getRelativeHumidityInPercentage(),
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
          environmentalSensor(sensorPin, nullptr),
          gatewayClient(client)
    {
        environmentalSensor.setHandler(this);

        initializeAsynchronousEngine(10);

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

/**
 * @brief WiFiClientSecure instance for TLS-encrypted MQTT connection.
 *
 * @note This replaces WiFiClient used in plain MQTT examples. WiFiClientSecure
 *       inherits from Client, making it compatible with MqttGatewayClient.
 *       Memory overhead is approximately 4-6KB RAM compared to WiFiClient.
 */
static WiFiClientSecure secureNetworkClient;

static WiFiConnectivityDriver* transportDriver = nullptr;
static MqttGatewayClient* gatewayClient        = nullptr;
static MqttTlsDevice* tlsApplication           = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    /**
     * @brief Configure TLS security settings.
     *
     * @warning setInsecure() disables certificate verification — use only for testing.
     *          For production, use secureNetworkClient.setCACert(caCertificate) with
     *          the PEM-encoded certificate of your broker's certificate authority.
     *          Example:
     *          @code
     *          static const char caCert[] = R"EOF(
     *          -----BEGIN CERTIFICATE-----
     *          MIID... (base64-encoded certificate) ...
     *          -----END CERTIFICATE-----
     *          )EOF";
     *          secureNetworkClient.setCACert(caCert);
     *          @endcode
     */
    secureNetworkClient.setInsecure();

    transportDriver = new WiFiConnectivityDriver(WIRELESS_SSID, WIRELESS_PASSWORD);

    /**
     * @brief Construct MqttGatewayClient with WiFiClientSecure instead of WiFiClient.
     *
     * The broker port is 8883 (MQTT over TLS) instead of 1883 (plain MQTT).
     * All other parameters are identical to plain MQTT configuration.
     */
    gatewayClient = new MqttGatewayClient(
        *transportDriver,
        secureNetworkClient,
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
