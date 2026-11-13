#ifndef FAKE_MQTT_H
#define FAKE_MQTT_H

#include <time.h>
#include "Client.h"
#include <cstdint>
#include <string>
#include <cstring>

#define MQTT_CONNECTION_TIMEOUT     -4
#define MQTT_CONNECTION_LOST        -3
#define MQTT_CONNECT_FAILED         -2
#define MQTT_DISCONNECTED           -1
#define MQTT_CONNECTED               0
#define MQTT_CONNECT_BAD_PROTOCOL    1
#define MQTT_CONNECT_BAD_CLIENT_ID   2
#define MQTT_CONNECT_UNAVAILABLE     3
#define MQTT_CONNECT_BAD_CREDENTIALS 4
#define MQTT_CONNECT_UNAUTHORIZED    5

// MQTT Things callback signatures (uses Arduino String from Arduino.h)
typedef void (*MQTTClientCallbackSimple)(String& topic, String& payload);

class MQTTClient {
private:
    [[maybe_unused]] Client* networkClient;
    std::string serverHost;
    int serverPort;
    bool isBrokerConnected;
    int lastErrorCode;
    MQTTClientCallbackSimple messageCallback;

public:
    // Simulation controls for testing
    static inline bool simulateConnectFailure = false;
    static inline bool simulatePublishFailure = false;
    static inline int connectAttemptCount = 0;
    static inline int publishCount = 0;
    static inline int loopCount = 0;
    static inline std::string lastTopic = "";
    static inline std::string lastPayload = "";
    static inline std::string lastClientId = "";

    // Subscribe simulation controls
    static inline int subscribeCount = 0;
    static inline std::string lastSubscribeTopic = "";
    static inline int lastSubscribeQos = 0;
    static inline bool simulateSubscribeFailure = false;
    static inline std::string pendingMessageTopic = "";
    static inline std::string pendingMessagePayload = "";
    static inline bool hasPendingMessage = false;

    static void resetMockState() {
        simulateConnectFailure = false;
        simulatePublishFailure = false;
        connectAttemptCount = 0;
        publishCount = 0;
        loopCount = 0;
        lastTopic.clear();
        lastPayload.clear();
        lastClientId.clear();
        subscribeCount = 0;
        lastSubscribeTopic.clear();
        lastSubscribeQos = 0;
        simulateSubscribeFailure = false;
        pendingMessageTopic.clear();
        pendingMessagePayload.clear();
        hasPendingMessage = false;
    }

    /**
     * @brief Constructor — MQTT Things takes optional buffer sizes, not Client&.
     * The Client is passed via begin().
     */
    explicit MQTTClient(int /*readBufSize*/ = 128, int /*writeBufSize*/ = 128)
        : networkClient(nullptr), serverPort(1883), isBrokerConnected(false), 
          lastErrorCode(MQTT_DISCONNECTED), messageCallback(nullptr) {}

    /**
     * @brief Initialize with Client only.
     */
    void begin(Client& client) {
        networkClient = &client;
    }

    /**
     * @brief Initialize with hostname, port, and Client.
     * MQTT Things signature: begin(hostname, port, client)
     */
    void begin(const char* host, int port, Client& client) {
        begin(client);
        serverHost = host ? host : "";
        serverPort = port;
    }

    /**
     * @brief Connect to the broker with the given client ID.
     * MQTT Things signature: connect(clientId)
     */
    bool connect(const char* id) {
        connectAttemptCount++;
        lastClientId = id ? id : "";
        if (simulateConnectFailure) {
            isBrokerConnected = false;
            lastErrorCode = MQTT_CONNECT_FAILED;
            return false;
        }
        isBrokerConnected = true;
        lastErrorCode = MQTT_CONNECTED;
        return true;
    }

    void disconnect() {
        isBrokerConnected = false;
        lastErrorCode = MQTT_DISCONNECTED;
    }

    /**
     * @brief Publish a message to a topic.
     * MQTT Things signature: publish(topic, payload, length)
     */
    bool publish(const char* topic, const char* payload, int length) {
        if (!isBrokerConnected) {
            return false;
        }
        if (simulatePublishFailure) {
            return false;
        }
        publishCount++;
        lastTopic = topic ? topic : "";
        lastPayload = std::string(payload ? payload : "", length);
        return true;
    }

    /**
     * @brief Publish with null-terminated payload (MQTT Things also provides this overload).
     */
    bool publish(const char* topic, const char* payload) {
        if (!isBrokerConnected) {
            return false;
        }
        if (simulatePublishFailure) {
            return false;
        }
        publishCount++;
        lastTopic = topic ? topic : "";
        lastPayload = payload ? payload : "";
        return true;
    }

    /**
     * @brief MQTT Things loop() — processes pending MQTT packets.
     */
    bool loop() {
        loopCount++;
        processPendingMessages();
        return isBrokerConnected;
    }

    [[nodiscard]] bool connected() const {
        return isBrokerConnected;
    }

    /**
     * @brief Returns the last error code (replaces PubSubClient::state()).
     */
    [[nodiscard]] int lastError() const {
        return lastErrorCode;
    }

    void setMockConnected(bool connected) {
        isBrokerConnected = connected;
        lastErrorCode = connected ? MQTT_CONNECTED : MQTT_DISCONNECTED;
    }

    /**
     * @brief Subscribe to a topic.
     * MQTT Things signature: subscribe(topic, qos)
     */
    bool subscribe(const char* topic, int qos = 0) {
        if (!isBrokerConnected) {
            return false;
        }
        if (simulateSubscribeFailure) {
            return false;
        }
        subscribeCount++;
        lastSubscribeTopic = topic ? topic : "";
        lastSubscribeQos = qos;
        return true;
    }

    /**
     * @brief Register a callback for incoming messages.
     * MQTT Things signature: onMessage(void(*)(String& topic, String& payload))
     */
    void onMessage(MQTTClientCallbackSimple callback) {
        messageCallback = callback;
    }

    /**
     * @brief Simulates an incoming MQTT message on a subscribed topic.
     * Triggers the callback if one is registered.
     */
    static void simulateIncomingMessage(const char* topic, const char* payload) {
        pendingMessageTopic = topic ? topic : "";
        pendingMessagePayload = payload ? payload : "";
        hasPendingMessage = true;
    }

    /**
     * @brief Processes pending messages by invoking the callback.
     * Called by loop() to simulate message delivery.
     */
    void processPendingMessages() {
        if (hasPendingMessage && messageCallback != nullptr) {
            String topicStr(pendingMessageTopic.c_str());
            String payloadStr(pendingMessagePayload.c_str());
            messageCallback(topicStr, payloadStr);
            hasPendingMessage = false;
        }
    }
};

#endif // FAKE_MQTT_H
