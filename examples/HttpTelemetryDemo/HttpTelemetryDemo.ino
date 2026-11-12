/**
 * @file HttpTelemetryDemo.ino
 * @brief Demonstrates HTTP telemetry with Bearer token authentication flow.
 *
 * This example uses a DHT22 sensor to read temperature and humidity, then sends
 * the readings to an HTTP REST API endpoint using HttpGatewayClient. It demonstrates:
 * - Bearer token acquisition from an authentication endpoint via sendRequest()
 * - Response body parsing using getResponseBody()
 * - Dynamic authorization header configuration
 * - HTTP POST telemetry with Bearer token authentication
 * - Statistics tracking (success/failure counts) via getStatistics()
 * - Response status code reporting via getLastResponseStatusCode()
 *
 * The example follows the Modest-IoT Nano-Framework's loopless architecture:
 * all sensor sampling and telemetry processing is managed by the Device's
 * asynchronous engine (FreeRTOS tasks). The main loop is intentionally empty.
 *
 * @date 2026-11-11
 * @version 1.0
 */

#define JSON_KEY_TEMPERATURE_IN_CELSIUS "temperatureInCelsius"
#define JSON_KEY_HUMIDITY_IN_PERCENTAGE "humidityInPercentage"
#define JSON_KEY_TOKEN "token"
#define JSON_TOKEN_VALUE_PREFIX ": \""
#define JSON_TOKEN_VALUE_DELIMITER '"'
#define TOKEN_VALUE_BUFFER_SIZE 128
#define AUTHORIZATION_HEADER_BUFFER_SIZE 150
#define TOKEN_PREVIEW_LENGTH 20
#define TOKEN_VALUE_OFFSET 3

/** @brief Sampler event queue length for HTTP telemetry operation. */
#define SAMPLER_QUEUE_LENGTH 10

#include <Arduino.h>
#include <cstring>
#include <memory>
#include "ModestIoT.h"

/** @name Concrete Application Data Schema Specification */
//@{

class HttpTelemetryPackage : public TelemetryPackage {
private:
    float temperatureInCelsius;
    float humidityInPercentage;

public:
    HttpTelemetryPackage(float temperature, float humidity)
        : temperatureInCelsius(temperature),
          humidityInPercentage(humidity) {}

    ~HttpTelemetryPackage() override = default;

    void serialize(JsonDocument& serializationDestination) const override {
        serializationDestination[JSON_KEY_TEMPERATURE_IN_CELSIUS] = temperatureInCelsius;
        serializationDestination[JSON_KEY_HUMIDITY_IN_PERCENTAGE] = humidityInPercentage;
    }
};
//@} // Concrete Application Data Schema Specification

/** @name Application Mediator Implementation */
//@{

class HttpTelemetryDevice : public Device {
private:
    DhtSensor climateSensor;
    HttpGatewayClient& gatewayClient;

    void packageAndQueueTelemetrySnapshot() {
        auto packet = std::make_unique<HttpTelemetryPackage>(
            climateSensor.getTemperatureInCelsius(),
            climateSensor.getRelativeHumidityInPercentage()
        );
        enqueueTelemetryPayload(std::move(packet));
    }

    /**
     * @brief Acquires a Bearer token from the authentication endpoint.
     * @return True if the token was successfully retrieved and configured.
     */
    bool acquireBearerToken() {
        Serial.println("[HttpTelemetryDemo] Requesting Bearer token...");

        bool tokenReceived = gatewayClient.sendRequest(
            HttpGatewayClient::HttpMethod::Get, nullptr);

        if (!tokenReceived) {
            Serial.println("[HttpTelemetryDemo] Failed to obtain Bearer token");
            return false;
        }

        int statusCode = gatewayClient.getLastResponseStatusCode();
        Serial.printf("[HttpTelemetryDemo] Token endpoint responded: %d\n", statusCode);

        const char* responseBody = gatewayClient.getResponseBody();
        if (responseBody == nullptr) {
            Serial.println("[HttpTelemetryDemo] Empty response from token endpoint");
            return false;
        }

        Serial.printf("[HttpTelemetryDemo] Response body: %s\n", responseBody);

        const char* tokenStart = strstr(responseBody, JSON_KEY_TOKEN);
        if (tokenStart != nullptr) {
            tokenStart = strstr(tokenStart, JSON_TOKEN_VALUE_PREFIX);
            if (tokenStart != nullptr) {
                tokenStart += TOKEN_VALUE_OFFSET;
                const char* tokenEnd = strchr(tokenStart, JSON_TOKEN_VALUE_DELIMITER);
                if (tokenEnd != nullptr) {
                    size_t tokenLength = tokenEnd - tokenStart;
                    char tokenValue[TOKEN_VALUE_BUFFER_SIZE];
                    if (tokenLength < sizeof(tokenValue)) {
                        memcpy(tokenValue, tokenStart, tokenLength);
                        tokenValue[tokenLength] = '\0';

                        char authorizationHeader[AUTHORIZATION_HEADER_BUFFER_SIZE];
                        snprintf(authorizationHeader, sizeof(authorizationHeader),
                                 "Bearer %s", tokenValue);
                        gatewayClient.setAuthorizationHeader(authorizationHeader);

                        Serial.printf("[HttpTelemetryDemo] Bearer token configured: %.*s...\n",
                                      TOKEN_PREVIEW_LENGTH, tokenValue);
                        return true;
                    }
                }
            }
        }

        Serial.println("[HttpTelemetryDemo] Could not parse token from response");
        return false;
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
    HttpTelemetryDevice(int climatePin, HttpGatewayClient& client,
                        unsigned long samplingIntervalInMilliseconds,
                        uint8_t timerGroupIndex = 0)
        : Device(samplingIntervalInMilliseconds, timerGroupIndex),
          climateSensor(climatePin, DhtSensor::SENSOR_MODEL_DHT22, this),
          gatewayClient(client)
    {
        initializeAsynchronousEngine(SAMPLER_QUEUE_LENGTH);

        appendSensorToScheduler(&climateSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    }

    ~HttpTelemetryDevice() override = default;

    void on(Event event) override {
        if (event.identifier == Sensor::DATA_READ_EVENT_IDENTIFIER) {
            packageAndQueueTelemetrySnapshot();
        }
    }

    void handle(Command command) override {}

    /**
     * @brief Prepares network communication, acquires authentication token,
     * and starts telemetry operations.
     * This method encapsulates all setup logic so the sketch's setup()
     * is only responsible for instance creation.
     * @param transport The Wi-Fi connectivity driver for network access.
     * @param authEndpointUrl The authentication endpoint URL for token acquisition.
     * @param telemetryEndpointUrl The telemetry endpoint URL for POST requests.
     * @param connectionTimeoutMs Maximum time to wait for Wi-Fi connection.
     * @return True if all initialization steps succeeded.
     */
    bool start(WiFiConnectivityDriver& transport,
               const char* authEndpointUrl,
               const char* telemetryEndpointUrl,
               unsigned long connectionTimeoutMs)
    {
        // Connect to Wi-Fi with timeout
        Serial.println("[HttpTelemetryDemo] Connecting to Wi-Fi...");
        if (!transport.waitForConnection(connectionTimeoutMs)) {
            Serial.println("[HttpTelemetryDemo] Wi-Fi connection timed out");
            return false;
        }

        Serial.printf("[HttpTelemetryDemo] Connected! IP: %s, RSSI: %ld dBm\n",
                      transport.getLocalIpAddress().toString().c_str(),
                      transport.getSignalStrengthInDbm());

        // Configure gateway client for authentication
        gatewayClient.setServiceEndpointUrl(authEndpointUrl);
        gatewayClient.setHttpMethod(HttpGatewayClient::HttpMethod::Get);

        // Acquire Bearer token
        if (!acquireBearerToken()) {
            Serial.println("[HttpTelemetryDemo] Authentication failed, aborting");
            return false;
        }

        // Switch to telemetry endpoint with POST method
        gatewayClient.setServiceEndpointUrl(telemetryEndpointUrl);
        gatewayClient.setHttpMethod(HttpGatewayClient::HttpMethod::Post);

        Serial.println("[HttpTelemetryDemo] HTTP telemetry online with Bearer token authentication.");
        return true;
    }
};

#define SERIAL_BAUD_RATE 115200
#define MAIN_LOOP_DELAY_IN_MILLISECONDS 60000
//@} // Application Mediator Implementation

/** @name Top-Level Declarative Canvas */
//@{

static const int CLIMATE_ONE_WIRE_BUS_PIN = 4;

static const char* WIRELESS_SSID = "HttpTelemetry_Secure_Mesh";
static const char* WIRELESS_PASSWORD = "TelemetryNodePass2026";
static const char* AUTH_ENDPOINT_URL = "http://192.168.1.100:8080/api/auth/token";
static const char* TELEMETRY_ENDPOINT_URL = "http://192.168.1.100:8080/api/telemetry";

static const unsigned long TELEMETRY_POLLING_INTERVAL_IN_MILLISECONDS = 1000;
static const unsigned long CONNECTION_TIMEOUT_IN_MILLISECONDS = 15000;
static const uint8_t HARDWARE_TIMER_CHANNEL_INDEX = 0;

static WiFiConnectivityDriver* transportDriver = nullptr;
static HttpGatewayClient* gatewayClient = nullptr;
static HttpTelemetryDevice* httpTelemetryApplication = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    transportDriver = new WiFiConnectivityDriver(WIRELESS_SSID, WIRELESS_PASSWORD);

    gatewayClient = new HttpGatewayClient(*transportDriver, AUTH_ENDPOINT_URL,
                                           HttpGatewayClient::HttpMethod::Get);

    httpTelemetryApplication = new HttpTelemetryDevice(
        CLIMATE_ONE_WIRE_BUS_PIN,
        *gatewayClient,
        TELEMETRY_POLLING_INTERVAL_IN_MILLISECONDS,
        HARDWARE_TIMER_CHANNEL_INDEX
    );

    if (!httpTelemetryApplication->start(*transportDriver, AUTH_ENDPOINT_URL,
                                         TELEMETRY_ENDPOINT_URL,
                                         CONNECTION_TIMEOUT_IN_MILLISECONDS)) {
        Serial.println("[HttpTelemetryDemo] Device startup failed.");
        return;
    }
}

void loop() {
    // The master loop is completely clean and empty.
    // The framework handles all timing, sensor sampling, and telemetry publishing.
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
