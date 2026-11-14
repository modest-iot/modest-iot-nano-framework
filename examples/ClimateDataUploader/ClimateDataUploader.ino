/**
 * @file ClimateDataUploader.ino
 * @brief Posts climate readings to a REST API with Bearer token authentication.
 *
 * This example demonstrates a grounded real-world device: a climate data uploader
 * that reads temperature and humidity from a DHT22 sensor and posts the readings
 * to an HTTP REST API endpoint. It handles authentication by acquiring a Bearer
 * token before sending telemetry data.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `TelemetryPackage` subclass for custom data schema
 * - `HttpGatewayClient` for HTTP-based telemetry transmission
 * - `sendRequest()` for arbitrary HTTP requests (GET, POST, etc.)
 * - `getResponseBody()` and `getLastResponseStatusCode()` for response handling
 * - `setAuthorizationHeader()` for Bearer token authentication
 * - `getStatistics()` for send success/failure tracking
 * - `DhtSensor` for temperature and humidity measurement
 * - `WiFiConnectivityDriver` with `waitForConnection()` timeout
 * - Loop-less, event-driven asynchronous execution via `initializeAsynchronousEngine()`
 *
 * @date 2026-11-11
 * @version 1.0
 */

#define JSON_KEY_TEMPERATURE_IN_CELSIUS "temperatureInCelsius"
#define JSON_KEY_HUMIDITY_IN_PERCENTAGE "humidityInPercentage"
#define JSON_KEY_TOKEN "token"
#define JSON_TOKEN_VALUE_PREFIX ": \""
#define JSON_TOKEN_VALUE_DELIMITER '"'

/** @brief Buffer size for token value extraction. */
static const size_t TOKEN_VALUE_BUFFER_SIZE = 128;
/** @brief Buffer size for the Authorization header string. */
static const size_t AUTHORIZATION_HEADER_BUFFER_SIZE = 150;
/** @brief Number of characters to preview in the token log message. */
static const size_t TOKEN_PREVIEW_LENGTH = 20;
/** @brief Offset from the JSON token value prefix to the actual token start. */
static const size_t TOKEN_VALUE_OFFSET = 3;

/** @brief Sampler event queue length for HTTP telemetry operation. */
static const int SAMPLER_QUEUE_LENGTH = 10;

#include <Arduino.h>
#include <cstring>
#include <memory>
#include "ModestIoT.h"

/** @brief Serial communication baud rate. */
static const unsigned long SERIAL_BAUD_RATE = 115200;

/** @brief Delay in the main loop when no work is pending. */
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;

/** @name Concrete Application Data Schema Specification */
//@{

class ClimateTelemetryPackage : public TelemetryPackage {
private:
    float temperatureInCelsius;
    float humidityInPercentage;

public:
    ClimateTelemetryPackage(float temperature, float humidity)
        : temperatureInCelsius(temperature),
          humidityInPercentage(humidity) {}

    ~ClimateTelemetryPackage() override = default;

    void serialize(JsonDocument& serializationDestination) const override {
        serializationDestination[JSON_KEY_TEMPERATURE_IN_CELSIUS] = temperatureInCelsius;
        serializationDestination[JSON_KEY_HUMIDITY_IN_PERCENTAGE] = humidityInPercentage;
    }
};
//@} // Concrete Application Data Schema Specification

/** @name Application Mediator Implementation */
//@{

class ClimateDataUploader : public Device {
private:
    DhtSensor climateSensor;
    HttpGatewayClient& gatewayClient;

    void packageAndQueueTelemetrySnapshot() {
        auto packet = std::make_unique<ClimateTelemetryPackage>(
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
        Serial.println("[ClimateDataUploader] Requesting Bearer token...");

        bool tokenReceived = gatewayClient.sendRequest(
            HttpGatewayClient::HttpMethod::Get, nullptr);

        if (!tokenReceived) {
            Serial.println("[ClimateDataUploader] Failed to obtain Bearer token");
            return false;
        }

        int statusCode = gatewayClient.getLastResponseStatusCode();
        Serial.printf("[ClimateDataUploader] Token endpoint responded: %d\n", statusCode);

        const char* responseBody = gatewayClient.getResponseBody();
        if (responseBody == nullptr) {
            Serial.println("[ClimateDataUploader] Empty response from token endpoint");
            return false;
        }

        Serial.printf("[ClimateDataUploader] Response body: %s\n", responseBody);

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

                        Serial.printf("[ClimateDataUploader] Bearer token configured: %.*s...\n",
                                      TOKEN_PREVIEW_LENGTH, tokenValue);
                        return true;
                    }
                }
            }
        }

        Serial.println("[ClimateDataUploader] Could not parse token from response");
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
    ClimateDataUploader(int climatePin, HttpGatewayClient& client,
                        unsigned long samplingIntervalInMilliseconds,
                        uint8_t timerGroupIndex = 0)
        : Device(samplingIntervalInMilliseconds, timerGroupIndex),
          climateSensor(climatePin, DhtSensor::SENSOR_MODEL_DHT22, this),
          gatewayClient(client)
    {
        initializeAsynchronousEngine(SAMPLER_QUEUE_LENGTH);

        appendSensorToScheduler(&climateSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    }

    ~ClimateDataUploader() override = default;

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
        Serial.println("[ClimateDataUploader] Connecting to Wi-Fi...");
        if (!transport.waitForConnection(connectionTimeoutMs)) {
            Serial.println("[ClimateDataUploader] Wi-Fi connection timed out");
            return false;
        }

        Serial.printf("[ClimateDataUploader] Connected! IP: %s, RSSI: %ld dBm\n",
                      transport.getLocalIpAddress().toString().c_str(),
                      transport.getSignalStrengthInDbm());

        // Configure gateway client for authentication
        gatewayClient.setServiceEndpointUrl(authEndpointUrl);
        gatewayClient.setHttpMethod(HttpGatewayClient::HttpMethod::Get);

        // Acquire Bearer token
        if (!acquireBearerToken()) {
            Serial.println("[ClimateDataUploader] Authentication failed, aborting");
            return false;
        }

        // Switch to telemetry endpoint with POST method
        gatewayClient.setServiceEndpointUrl(telemetryEndpointUrl);
        gatewayClient.setHttpMethod(HttpGatewayClient::HttpMethod::Post);

        Serial.println("[ClimateDataUploader] HTTP telemetry online with Bearer token authentication.");
        return true;
    }
};
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
static ClimateDataUploader* climateUploaderApplication = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    transportDriver = new WiFiConnectivityDriver(WIRELESS_SSID, WIRELESS_PASSWORD);

    gatewayClient = new HttpGatewayClient(*transportDriver, AUTH_ENDPOINT_URL,
                                           HttpGatewayClient::HttpMethod::Get);

    climateUploaderApplication = new ClimateDataUploader(
        CLIMATE_ONE_WIRE_BUS_PIN,
        *gatewayClient,
        TELEMETRY_POLLING_INTERVAL_IN_MILLISECONDS,
        HARDWARE_TIMER_CHANNEL_INDEX
    );

    if (!climateUploaderApplication->start(*transportDriver, AUTH_ENDPOINT_URL,
                                         TELEMETRY_ENDPOINT_URL,
                                         CONNECTION_TIMEOUT_IN_MILLISECONDS)) {
        Serial.println("[ClimateDataUploader] Device startup failed.");
        return;
    }
}

void loop() {
    // The master loop is completely clean and empty.
    // The framework handles all timing, sensor sampling, and telemetry publishing.
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
