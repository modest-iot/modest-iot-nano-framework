/**
 * @file ClimateDataUploader.ino
 * @brief Posts climate readings to a REST API with offline buffering support.
 *
 * This example demonstrates a grounded real-world device: a climate data uploader
 * that reads temperature and humidity from a DHT22 sensor and posts the readings
 * to an HTTP REST API endpoint. It handles authentication by acquiring a Bearer
 * token before sending telemetry data. When the network is unavailable, failed
 * sends are buffered in LittleFS via TelemetryBuffer and replayed when connectivity
 * is restored.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `TelemetryPackage` subclass for custom data schema
 * - `HttpGatewayClient` for HTTP-based telemetry transmission
 * - `sendRequest()` for arbitrary HTTP requests (GET, POST, etc.)
 * - `getResponseBody()` and `getLastResponseStatusCode()` for response handling
 * - `setAuthorizationHeader()` for Bearer token authentication
 * - `getStatistics()` for send success/failure tracking
 * - `TelemetryBuffer` for offline persistence with FIFO eviction and replay
 * - `Device::setFirmwareVersion()` for fleet management in telemetry
 * - `DhtSensor` for temperature and humidity measurement
 * - `WiFiConnectivityDriver` with `waitForConnection()` timeout
 * - Loop-less, event-driven asynchronous execution via `initializeAsynchronousEngine()`
 *
 * @date 2026-11-15
 * @version 1.1
 */

#include <Arduino.h>
#include <cstring>
#include <memory>
#include "ModestIoT.h"

/** @name Hardware Configuration */
//@{
/** @brief GPIO pin for the DHT22 sensor data line. */
static const int CLIMATE_ONE_WIRE_BUS_PIN = 4;
/** @brief Hardware timer channel for the sampling engine. */
static const uint8_t HARDWARE_TIMER_CHANNEL_INDEX = 0;
//@} // Hardware Configuration

/** @name Network Configuration */
//@{
/** @brief Wi-Fi SSID for the device to connect to. */
static const char* WIRELESS_SSID = "HttpTelemetry_Secure_Mesh";
/** @brief Wi-Fi password for authentication. */
static const char* WIRELESS_PASSWORD = "TelemetryNodePass2026";
/** @brief Maximum time to wait for Wi-Fi connection before timing out. */
static const unsigned long CONNECTION_TIMEOUT_IN_MILLISECONDS = 15000;
//@} // Network Configuration

/** @name API Endpoints */
//@{
/** @brief REST API endpoint for acquiring a Bearer authentication token. */
static const char* AUTHENTICATION_ENDPOINT_URL = "http://192.168.1.100:8080/api/v1/tokens";
/** @brief REST API endpoint for posting telemetry data. */
static const char* TELEMETRY_ENDPOINT_URL = "http://192.168.1.100:8080/api/v1/telemetry-records";
//@} // API Endpoints

/** @name Timing Configuration */
//@{
/** @brief Interval between climate sensor readings. */
static const unsigned long TELEMETRY_POLLING_INTERVAL_IN_MILLISECONDS = 1000;
/** @brief Delay in the main loop when no work is pending. */
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;
//@} // Timing Configuration

/** @name Framework Configuration */
//@{
/** @brief Sampler event queue length for HTTP telemetry operation. */
static const int SAMPLER_QUEUE_LENGTH = 10;
/** @brief Serial communication baud rate. */
static const unsigned long SERIAL_BAUD_RATE = 115200;
//@} // Framework Configuration

/** @name Telemetry JSON Keys */
//@{
/** @brief JSON key for temperature in Celsius. */
#define JSON_KEY_TEMPERATURE_IN_CELSIUS "temperatureInCelsius"
/** @brief JSON key for humidity in percentage. */
#define JSON_KEY_HUMIDITY_IN_PERCENTAGE "humidityInPercentage"
//@} // Telemetry JSON Keys

/** @name Bearer Token Parsing Configuration */
//@{
/** @brief JSON key for the authentication token in the response body. */
static const char* BEARER_TOKEN_JSON_KEY = "token";
/** @brief Prefix string before the token value in the JSON response. */
static const char* BEARER_TOKEN_VALUE_PREFIX = ": \"";
/** @brief Delimiter character that ends the token value in the JSON response. */
static const char BEARER_TOKEN_VALUE_DELIMITER = '"';
/** @brief The "Bearer " prefix including the trailing space. */
static const char* BEARER_AUTHORIZATION_PREFIX = "Bearer ";
/** @brief Length of the "Bearer " prefix plus null terminator. */
static const size_t BEARER_AUTHORIZATION_PREFIX_LENGTH_WITH_NULL = 8;
/** @brief Buffer size for extracted token values. */
static const size_t BEARER_TOKEN_MAX_LENGTH = 128;
/** @brief Buffer size for the Authorization header string. */
static const size_t BEARER_AUTHORIZATION_HEADER_MAX_LENGTH = 150;
/** @brief Number of characters to preview in the token log message. */
static const size_t BEARER_TOKEN_PREVIEW_LENGTH = 20;
//@} // Bearer Token Parsing Configuration

/** @name Serial Output Configuration */
//@{
/** @brief Log prefix for all ClimateDataUploader serial output. */
static const char* CLIMATE_DATA_UPLOADER_LOG_PREFIX = "[ClimateDataUploader]";
//@} // Serial Output Configuration

/**
 * @brief Extracts a Bearer token from a JSON response body and builds
 * the complete authorization header.
 *
 * Parses a JSON response such as `{"token": "abc123"}` and returns a
 * ready-to-use "Bearer abc123" header string. This utility isolates all
 * token parsing and header formatting logic from the application mediator.
 */
class BearerTokenParser {
public:
    /**
     * @brief Extracts the token from a JSON response and builds the
     * complete Bearer authorization header in a single call.
     * @param responseBody The raw JSON response string.
     * @param outAuthorizationHeader Buffer to receive the formatted header
     * (must be at least BEARER_AUTHORIZATION_HEADER_MAX_LENGTH bytes).
     * @param maxHeaderLength Maximum length of the header buffer.
     * @param outTokenPreview Optional buffer to receive a truncated token
     * preview for logging (must be at least BEARER_TOKEN_MAX_LENGTH bytes).
     * Pass nullptr if not needed.
     * @return True if the authorization header was successfully built.
     */
    static bool buildAuthorizationHeader(const char* responseBody,
                                          char* outAuthorizationHeader,
                                          size_t maxHeaderLength,
                                          char* outTokenPreview) {
        if (responseBody == nullptr || outAuthorizationHeader == nullptr || maxHeaderLength == 0) {
            return false;
        }

        const char* keyPos = strstr(responseBody, BEARER_TOKEN_JSON_KEY);
        if (keyPos == nullptr) {
            return false;
        }

        const char* valuePrefixPos = strstr(keyPos, BEARER_TOKEN_VALUE_PREFIX);
        if (valuePrefixPos == nullptr) {
            return false;
        }

        // Skip past the prefix `": "` to reach the actual token value
        const char* tokenStart = valuePrefixPos + strlen(BEARER_TOKEN_VALUE_PREFIX);
        const char* tokenEnd = strchr(tokenStart, BEARER_TOKEN_VALUE_DELIMITER);
        if (tokenEnd == nullptr) {
            return false;
        }

        size_t tokenLength = tokenEnd - tokenStart;
        if (tokenLength >= maxHeaderLength - BEARER_AUTHORIZATION_PREFIX_LENGTH_WITH_NULL) {
            return false;
        }

        // Copy token for optional preview before building the header
        if (outTokenPreview != nullptr) {
            size_t previewLength = (tokenLength < BEARER_TOKEN_PREVIEW_LENGTH)
                                        ? tokenLength
                                        : BEARER_TOKEN_PREVIEW_LENGTH;
            memcpy(outTokenPreview, tokenStart, previewLength);
            outTokenPreview[previewLength] = '\0';
        }

        // Build the complete "Bearer <token>" authorization header
        snprintf(outAuthorizationHeader, maxHeaderLength, "%s%.*s",
                 BEARER_AUTHORIZATION_PREFIX, static_cast<int>(tokenLength), tokenStart);
        return true;
    }
};

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

    void write(TelemetryWriter& writer) const override {
        writer.field(JSON_KEY_TEMPERATURE_IN_CELSIUS, temperatureInCelsius);
        writer.field(JSON_KEY_HUMIDITY_IN_PERCENTAGE, humidityInPercentage);
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
     * @brief Replays any buffered packets from a previous session or failed sends.
     * Called after successful network reconnection to drain the offline buffer.
     */
    void replayBufferedPackets() {
        String bufferedPacket;
        while (TelemetryBuffer::retrieveNext(bufferedPacket)) {
            Serial.printf("%s Replaying buffered packet: %s\n", CLIMATE_DATA_UPLOADER_LOG_PREFIX, bufferedPacket.c_str());
            gatewayClient.sendRequest(HttpGatewayClient::HttpMethod::Post, bufferedPacket.c_str());
        }
        Serial.println((String(CLIMATE_DATA_UPLOADER_LOG_PREFIX) + " Buffered packet replay complete.").c_str());
    }

    /**
     * @brief Acquires a Bearer token from the authentication endpoint.
     * @return True if the token was successfully retrieved and configured.
     */
    bool acquireBearerToken() {
        Serial.println((String(CLIMATE_DATA_UPLOADER_LOG_PREFIX) + " Requesting Bearer token...").c_str());

        bool tokenReceived = gatewayClient.sendRequest(
            HttpGatewayClient::HttpMethod::Get, nullptr);

        if (!tokenReceived) {
            Serial.println((String(CLIMATE_DATA_UPLOADER_LOG_PREFIX) + " Failed to obtain Bearer token").c_str());
            return false;
        }

        int statusCode = gatewayClient.getLastResponseStatusCode();
        Serial.printf("%s Token endpoint responded: %d\n", CLIMATE_DATA_UPLOADER_LOG_PREFIX, statusCode);

        const char* responseBody = gatewayClient.getResponseBody();
        if (responseBody == nullptr) {
            Serial.println((String(CLIMATE_DATA_UPLOADER_LOG_PREFIX) + " Empty response from token endpoint").c_str());
            return false;
        }

        Serial.printf("%s Response body: %s\n", CLIMATE_DATA_UPLOADER_LOG_PREFIX, responseBody);

        char authorizationHeader[BEARER_AUTHORIZATION_HEADER_MAX_LENGTH];
        char tokenPreview[BEARER_TOKEN_MAX_LENGTH];
        if (!BearerTokenParser::buildAuthorizationHeader(responseBody, authorizationHeader,
                                                         sizeof(authorizationHeader), tokenPreview)) {
            Serial.println((String(CLIMATE_DATA_UPLOADER_LOG_PREFIX) + " Could not parse token from response").c_str());
            return false;
        }

        gatewayClient.setAuthorizationHeader(authorizationHeader);

        Serial.printf("%s Bearer token configured: %s...\n", CLIMATE_DATA_UPLOADER_LOG_PREFIX, tokenPreview);
        return true;
    }

protected:
    void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const override {
        if (rawQueueItemPayload != nullptr) {
            bool sent = gatewayClient.sendTelemetryRecord(*rawQueueItemPayload);
            if (!sent) {
                // Serialize and buffer the payload for later replay when connectivity is restored
                String failedPayload = rawQueueItemPayload->toJsonString();
                if (TelemetryBuffer::store(failedPayload)) {
                    Serial.println((String(CLIMATE_DATA_UPLOADER_LOG_PREFIX) + " Send failed — payload buffered for offline replay.").c_str());
                } else {
                    Serial.println((String(CLIMATE_DATA_UPLOADER_LOG_PREFIX) + " Send failed and buffer is full — payload lost.").c_str());
                }
            }
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
        // IMPORTANT: Registration must occur before initializeAsynchronousEngine(), because
        // the engine start guard rejects any registration after the engine is running.
        appendSensorToScheduler(&climateSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);

        if (!initializeAsynchronousEngine(SAMPLER_QUEUE_LENGTH)) {
            return;
        }
    }

    ~ClimateDataUploader() override {
        stopAsynchronousEngine();
    }

    /**
     * @brief Handles sensor data-ready events to package and queue telemetry.
     * @param event The triggered event context payload.
     * @runs_on{sampler}
     */
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
     * Each step cleans up partial state on failure, leaving the device
     * in a predictable state regardless of where initialization fails.
     * @param transport The Wi-Fi connectivity driver for network access.
     * @param authenticationEndpointUrl The authentication endpoint URL for token acquisition.
     * @param telemetryEndpointUrl The telemetry endpoint URL for POST requests.
     * @param connectionTimeoutInMilliseconds Maximum time to wait for Wi-Fi connection.
     * @return True if all initialization steps succeeded.
     */
    bool start(WiFiConnectivityDriver& transport,
                const char* authenticationEndpointUrl,
                const char* telemetryEndpointUrl,
                unsigned long connectionTimeoutInMilliseconds)
    {
        if (!isAsynchronousEngineStarted()) {
            return false;
        }
        // Set firmware version for fleet management in telemetry
        Device::setFirmwareVersion("CLD-100 FW 1.1.0");

        // Initialize TelemetryBuffer for offline persistence
        if (!TelemetryBuffer::begin()) {
            Serial.println((String(CLIMATE_DATA_UPLOADER_LOG_PREFIX) + " Failed to initialize TelemetryBuffer").c_str());
            return false;
        }
        TelemetryBuffer::onEviction([]() {
            Serial.println((String(CLIMATE_DATA_UPLOADER_LOG_PREFIX) + " WARNING: Oldest buffered packet evicted due to capacity.").c_str());
        });

        // Replay any packets buffered from a previous session
        if (TelemetryBuffer::hasBufferedPackets()) {
            int bufferedCount = TelemetryBuffer::getBufferedPacketCount();
            Serial.printf("%s Found %d buffered packets from previous session.\n", CLIMATE_DATA_UPLOADER_LOG_PREFIX, bufferedCount);
        }

        // Connect to Wi-Fi with timeout
        Serial.println((String(CLIMATE_DATA_UPLOADER_LOG_PREFIX) + " Connecting to Wi-Fi...").c_str());
        if (!transport.waitForConnection(connectionTimeoutInMilliseconds)) {
            Serial.println((String(CLIMATE_DATA_UPLOADER_LOG_PREFIX) + " Wi-Fi connection timed out").c_str());
            return false;
        }

        Serial.printf("%s Connected! IP: %s, RSSI: %ld dBm\n",
                      CLIMATE_DATA_UPLOADER_LOG_PREFIX,
                      transport.getLocalIpAddress().toString().c_str(),
                      transport.getSignalStrengthInDbm());

        // Replay buffered packets now that we have connectivity
        if (TelemetryBuffer::hasBufferedPackets()) {
            replayBufferedPackets();
        }

        // Configure gateway client for authentication
        gatewayClient.setServiceEndpointUrl(authenticationEndpointUrl);
        gatewayClient.setHttpMethod(HttpGatewayClient::HttpMethod::Get);

        // Acquire Bearer token
        if (!acquireBearerToken()) {
            Serial.println((String(CLIMATE_DATA_UPLOADER_LOG_PREFIX) + " Authentication failed, aborting").c_str());
            // Reset gateway client to clean state after failed authentication
            gatewayClient.setServiceEndpointUrl(nullptr);
            gatewayClient.setAuthorizationHeader(nullptr);
            return false;
        }

        // Switch to telemetry endpoint with POST method
        gatewayClient.setServiceEndpointUrl(telemetryEndpointUrl);
        gatewayClient.setHttpMethod(HttpGatewayClient::HttpMethod::Post);

        Serial.println((String(CLIMATE_DATA_UPLOADER_LOG_PREFIX) + " HTTP telemetry online with Bearer token authentication.").c_str());
        return true;
    }
};
//@} // Application Mediator Implementation

/** @name Top-Level Declarative Canvas */
//@{

static WiFiConnectivityDriver* transportDriver = nullptr;
static HttpGatewayClient* gatewayClient = nullptr;
static ClimateDataUploader* climateUploaderApplication = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    transportDriver = new WiFiConnectivityDriver(WIRELESS_SSID, WIRELESS_PASSWORD);

    gatewayClient = new HttpGatewayClient(*transportDriver, AUTHENTICATION_ENDPOINT_URL,
                                           HttpGatewayClient::HttpMethod::Get);

    climateUploaderApplication = new ClimateDataUploader(
        CLIMATE_ONE_WIRE_BUS_PIN,
        *gatewayClient,
        TELEMETRY_POLLING_INTERVAL_IN_MILLISECONDS,
        HARDWARE_TIMER_CHANNEL_INDEX
    );

    if (!climateUploaderApplication->start(*transportDriver, AUTHENTICATION_ENDPOINT_URL,
                                         TELEMETRY_ENDPOINT_URL,
                                         CONNECTION_TIMEOUT_IN_MILLISECONDS)) {
        Serial.println((String(CLIMATE_DATA_UPLOADER_LOG_PREFIX) + " Device startup failed.").c_str());
        return;
    }
}

void loop() {
    // The master loop is completely clean and empty.
    // The framework handles all timing, sensor sampling, and telemetry publishing.
    // vTaskDelay clarifies that this loop yields to FreeRTOS — the device is loopless.
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
