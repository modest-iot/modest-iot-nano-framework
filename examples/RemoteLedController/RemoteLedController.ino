/**
 * @file RemoteLedController.ino
 * @brief MQTT-connected RGB smart lamp that changes color based on cloud commands.
 *
 * This example demonstrates a grounded real-world device: a smart lamp or indicator light
 * that receives color change commands from a home automation server (e.g., Home Assistant)
 * via MQTT. The cloud publishes a command to a subscribe topic, the device receives it
 * via MQTT subscribe callback, marshals it across tasks using enqueueCommand(), and
 * changes the RGB LED color accordingly.
 *
 * This is the primary example for the cloud-to-device command flow, which is the core
 * CQRS (Command Query Responsibility Segregation) pattern in the framework.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `enqueueCommand()` for cross-task command marshaling (network → sampler task)
 * - `MqttGatewayClient` with MQTT subscribe for cloud-to-device commands
 * - `RgbLed` actuator with PWM color control and predefined colors
 * - `setColor()` and `SET_COLOR_COMMAND` for buffered color changes
 * - `ADJUST_BRIGHTNESS_COMMAND` with payload for parameterized brightness adjustment
 * - `Command` value object with optional `int32_t payload` member
 * - `Command` handling via `handle(Command)` override
 * - `getStatistics()` for send success/failure tracking via `GatewayStatistics`
 * - Cross-task communication from network task to sampler task
 * - Loop-less, event-driven asynchronous execution via `initializeAsynchronousEngine()`
 *
 * @par MQTT Payload format:
 * Commands are sent as JSON: `{"command": 400, "payload": 128}`
 * The payload field is optional and defaults to 0 when omitted.
 *
 * @par Command identifiers used by this example:
 * - 400: Set LED to RED color
 * - 401: Set LED to GREEN color
 * - 402: Set LED to BLUE color
 * - 403: Set LED to ORANGE color
 * - 404: Set LED to WHITE color
 * - 304: Adjust LED brightness (payload 0-255 scales active color)
 * - 300: Turn LED ON with last applied color
 * - 301: Turn LED OFF
 *
 * @date 2026-11-12
 * @version 1.0
 */

/** @name Cloud-to-Device Command Identifiers */
//@{
static constexpr int SET_COLOR_RED_COMMAND_IDENTIFIER = 400;
static constexpr int SET_COLOR_GREEN_COMMAND_IDENTIFIER = 401;
static constexpr int SET_COLOR_BLUE_COMMAND_IDENTIFIER = 402;
static constexpr int SET_COLOR_ORANGE_COMMAND_IDENTIFIER = 403;
static constexpr int SET_COLOR_WHITE_COMMAND_IDENTIFIER = 404;
//@} // Cloud-to-Device Command Identifiers

/** @brief Sampler event queue length for remote LED control. */
#define SAMPLER_QUEUE_LENGTH 10

#include <Arduino.h>
#include "ModestIoT.h"

/** @brief Serial communication baud rate. */
static const unsigned long SERIAL_BAUD_RATE = 115200;

/** @brief Delay in the main loop when no work is pending. */
static const unsigned long MAIN_LOOP_DELAY_IN_MILLISECONDS = 60000;

/** @name Application Mediator Implementation */
//@{

/**
 * @brief MQTT-connected RGB smart lamp device.
 *
 * This class extends the `Device` base class to manage an RGB LED actuator and an MQTT
 * gateway client. Commands received from the cloud via MQTT subscribe are marshaled
 * across tasks using enqueueCommand() and processed in the sampler task context.
 */
class RemoteLedController : public Device {
private:
    RgbLed smartLamp;
    MqttGatewayClient& gatewayClient;

    /**
     * @brief Reports MQTT transmission statistics to Serial.
     */
    void reportStatistics() {
        GatewayStatistics stats = gatewayClient.getStatistics();
        Serial.printf("[RemoteLedController] MQTT stats -> succeeded: %lu, failed: %lu\n",
                      stats.sendsSucceeded, stats.sendsFailed);
    }

protected:
    void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const override {
        // This device does not send telemetry; it only receives commands.
        (void)rawQueueItemPayload;
    }

    void maintainNetworkServices() const override {
        gatewayClient.service();
    }

public:
    /**
     * @brief Constructs a new RemoteLedController application instance.
     *
     * @param redPin PWM GPIO pin for the red channel.
     * @param greenPin PWM GPIO pin for the green channel.
     * @param bluePin PWM GPIO pin for the blue channel.
     * @param client Reference to the MQTT gateway client.
     * @param samplingIntervalInMilliseconds Base sampling interval (ms).
     * @param timerGroupIndex Hardware timer group index (0-3).
     */
    RemoteLedController(int redPin, int greenPin, int bluePin, MqttGatewayClient& client,
                        unsigned long samplingIntervalInMilliseconds,
                        uint8_t timerGroupIndex = 0)
        : Device(samplingIntervalInMilliseconds, timerGroupIndex),
          smartLamp(redPin, greenPin, bluePin, false, this),
          gatewayClient(client)
    {
        initializeAsynchronousEngine(SAMPLER_QUEUE_LENGTH);
    }

    ~RemoteLedController() override = default;

    /**
     * @brief Handles events dispatched by the framework.
     * @param event The triggered event context payload.
     * @runs_on{sampler}
     */
    void on(Event event) override {
        // This device does not use sensors; events are not expected.
        (void)event;
    }

    /**
     * @brief Handles commands received from the cloud via MQTT subscribe.
     *
     * Commands arrive from the network task via enqueueCommand() and are processed
     * here in the sampler task context. Each command identifier maps to a specific
     * color or action for the RGB LED.
     *
     * @param command The command to execute.
     * @runs_on{sampler}
     */
    void handle(Command command) override {
        switch (command.identifier) {
            case SET_COLOR_RED_COMMAND_IDENTIFIER:
                smartLamp.setColor(RgbLed::RED);
                smartLamp.handle(RgbLed::SET_COLOR_COMMAND);
                Serial.println("[RemoteLedController] Cloud command -> LED set to RED");
                break;

            case SET_COLOR_GREEN_COMMAND_IDENTIFIER:
                smartLamp.setColor(RgbLed::GREEN);
                smartLamp.handle(RgbLed::SET_COLOR_COMMAND);
                Serial.println("[RemoteLedController] Cloud command -> LED set to GREEN");
                break;

            case SET_COLOR_BLUE_COMMAND_IDENTIFIER:
                smartLamp.setColor(RgbLed::BLUE);
                smartLamp.handle(RgbLed::SET_COLOR_COMMAND);
                Serial.println("[RemoteLedController] Cloud command -> LED set to BLUE");
                break;

            case SET_COLOR_ORANGE_COMMAND_IDENTIFIER:
                smartLamp.setColor(RgbLed::ORANGE);
                smartLamp.handle(RgbLed::SET_COLOR_COMMAND);
                Serial.println("[RemoteLedController] Cloud command -> LED set to ORANGE");
                break;

            case SET_COLOR_WHITE_COMMAND_IDENTIFIER:
                smartLamp.setColor(RgbLed::WHITE);
                smartLamp.handle(RgbLed::SET_COLOR_COMMAND);
                Serial.println("[RemoteLedController] Cloud command -> LED set to WHITE");
                break;

            case RgbLed::TURN_ON_COMMAND_IDENTIFIER:
                smartLamp.handle(RgbLed::TURN_ON_COMMAND);
                Serial.println("[RemoteLedController] Cloud command -> LED turned ON");
                break;

            case RgbLed::TURN_OFF_COMMAND_IDENTIFIER:
                smartLamp.handle(RgbLed::TURN_OFF_COMMAND);
                Serial.println("[RemoteLedController] Cloud command -> LED turned OFF");
                break;

            case RgbLed::ADJUST_BRIGHTNESS_COMMAND_IDENTIFIER:
                smartLamp.handle(Command{RgbLed::ADJUST_BRIGHTNESS_COMMAND_IDENTIFIER, command.payload});
                Serial.printf("[RemoteLedController] Cloud command -> LED brightness adjusted to %ld\n", command.payload);
                break;

            default:
                Serial.printf("[RemoteLedController] Unknown cloud command: %d\n", command.identifier);
                break;
        }
    }

    /**
     * @brief Prepares WiFi and MQTT connection, and configures the subscribe topic.
     *
     * @param transport The WiFi connectivity driver for network access.
     * @param subscribeTopic MQTT topic to subscribe for color change commands.
     * @param connectionTimeoutMs Maximum time to wait for WiFi connection.
     * @return True if all initialization steps succeeded.
     */
    bool start(WiFiConnectivityDriver& transport,
               const char* subscribeTopic,
               unsigned long connectionTimeoutMs)
    {
        // Connect to WiFi with timeout
        Serial.println("[RemoteLedController] Connecting to WiFi...");
        if (!transport.waitForConnection(connectionTimeoutMs)) {
            Serial.println("[RemoteLedController] WiFi connection timed out");
            return false;
        }

        Serial.printf("[RemoteLedController] Connected! IP: %s, RSSI: %ld dBm\n",
                      transport.getLocalIpAddress().toString().c_str(),
                      transport.getSignalStrengthInDbm());

        // Configure MQTT subscribe topic for cloud-to-device commands
        gatewayClient.setSubscribeTopic(subscribeTopic);
        gatewayClient.setSubscribeCallback(
            [this](const char* payload, size_t length) {
                // Parse JSON payload: {"command": 400, "payload": 128}
                // The payload field is optional and defaults to 0.
                (void)length;
                StaticJsonDocument<64> doc;
                DeserializationError err = deserializeJson(doc, payload, length);
                if (err == DeserializationError::Ok) {
                    int commandId = doc["command"] | 0;
                    int32_t commandPayload = doc["payload"] | 0;
                    if (commandId > 0) {
                        Command cmd{commandId, commandPayload};
                        enqueueCommand(cmd);
                        Serial.printf("[RemoteLedController] MQTT subscribe -> enqueued command %d (payload %ld)\n",
                                      commandId, commandPayload);
                    }
                } else {
                    Serial.printf("[RemoteLedController] Failed to parse MQTT payload: %s\n", err.c_str());
                }
            }
        );

        // Set initial color to orange
        smartLamp.setColor(RgbLed::ORANGE);
        smartLamp.handle(RgbLed::SET_COLOR_COMMAND);
        Serial.println("[RemoteLedController] Remote LED controller online. Awaiting cloud commands.");
        return true;
    }
};
//@} // Application Mediator Implementation

/** @name Top-Level Declarative Canvas */
//@{

static const int RGB_LED_RED_PIN = 25;
static const int RGB_LED_GREEN_PIN = 26;
static const int RGB_LED_BLUE_PIN = 27;

static const char* WIRELESS_SSID = "HomeAutomation";
static const char* WIRELESS_PASSWORD = "SmartHomePass2026";

static const char* MQTT_BROKER_ADDRESS = "192.168.1.100";
static const int MQTT_BROKER_PORT = 1883;
static const char* MQTT_DEVICE_ID = "smart-lamp-01";
static const char* MQTT_SUBSCRIBE_TOPIC = "home/lamp/01/color";

static const unsigned long SAMPLING_INTERVAL_IN_MILLISECONDS = 1000;
static const unsigned long CONNECTION_TIMEOUT_IN_MILLISECONDS = 15000;
static const uint8_t HARDWARE_TIMER_CHANNEL_INDEX = 0;

static WiFiConnectivityDriver* wifiTransport = nullptr;
static MqttGatewayClient* mqttGateway = nullptr;
static RemoteLedController* ledController = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);

    wifiTransport = new WiFiConnectivityDriver(WIRELESS_SSID, WIRELESS_PASSWORD);

    mqttGateway = new MqttGatewayClient(*wifiTransport, MQTT_BROKER_ADDRESS, MQTT_BROKER_PORT,
                                         MQTT_DEVICE_ID);

    ledController = new RemoteLedController(
        RGB_LED_RED_PIN,
        RGB_LED_GREEN_PIN,
        RGB_LED_BLUE_PIN,
        *mqttGateway,
        SAMPLING_INTERVAL_IN_MILLISECONDS,
        HARDWARE_TIMER_CHANNEL_INDEX
    );

    if (!ledController->start(*wifiTransport, MQTT_SUBSCRIBE_TOPIC,
                              CONNECTION_TIMEOUT_IN_MILLISECONDS)) {
        Serial.println("[RemoteLedController] Device startup failed.");
        return;
    }
}

void loop() {
    // The master loop is completely clean and empty.
    // The framework handles all timing, MQTT communication, and command processing.
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
//@} // Top-Level Declarative Canvas
