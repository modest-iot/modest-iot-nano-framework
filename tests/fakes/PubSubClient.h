#ifndef FAKE_PUB_SUB_CLIENT_H
#define FAKE_PUB_SUB_CLIENT_H

#include "Client.h"
#include <cstdint>
#include <string>
#include <functional>

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

// Forward declaration for callback signature
typedef void (*MqttCallback)(const char* topic, const byte* payload, unsigned int length);

class PubSubClient {
private:
    [[maybe_unused]] Client* networkClient;
    std::string serverHost;
    uint16_t serverPort;
    bool isBrokerConnected;
    int clientState;
    MqttCallback messageCallback; ///< W15: Callback for subscribed messages

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
    
    // W15: Subscribe simulation controls
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

    explicit PubSubClient(Client& client)
        : networkClient(&client), serverPort(1883), isBrokerConnected(false), clientState(MQTT_DISCONNECTED), messageCallback(nullptr) {}

    PubSubClient& setServer(const char* ip, uint16_t port) {
        serverHost = ip != nullptr ? ip : "";
        serverPort = port;
        return *this;
    }

    bool connect(const char* id) {
        connectAttemptCount++;
        lastClientId = id != nullptr ? id : "";
        if (simulateConnectFailure) {
            isBrokerConnected = false;
            clientState = MQTT_CONNECT_FAILED;
            return false;
        }
        isBrokerConnected = true;
        clientState = MQTT_CONNECTED;
        return true;
    }

    bool connect(const char* id, const char* user, const char* pass) {
        (void)user;
        (void)pass;
        return connect(id);
    }

    void disconnect() {
        isBrokerConnected = false;
        clientState = MQTT_DISCONNECTED;
    }

    bool publish(const char* topic, const char* payload) {
        if (!isBrokerConnected) {
            return false;
        }
        if (simulatePublishFailure) {
            return false;
        }
        publishCount++;
        lastTopic = topic != nullptr ? topic : "";
        lastPayload = payload != nullptr ? payload : "";
        return true;
    }

    bool loop() {
        loopCount++;
        processPendingMessages();
        return isBrokerConnected;
    }

    [[nodiscard]] bool connected() const {
        return isBrokerConnected;
    }

    [[nodiscard]] int state() const {
        return clientState;
    }

    void setMockConnected(bool connected) {
        isBrokerConnected = connected;
        clientState = connected ? MQTT_CONNECTED : MQTT_DISCONNECTED;
    }
    
    // W15: Subscribe support
    bool subscribe(const char* topic, uint8_t qos = 0) {
        if (!isBrokerConnected) {
            return false;
        }
        if (simulateSubscribeFailure) {
            return false;
        }
        subscribeCount++;
        lastSubscribeTopic = topic != nullptr ? topic : "";
        lastSubscribeQos = qos;
        return true;
    }
    
    void setCallback(MqttCallback callback) {
        messageCallback = callback;
    }
    
    /**
     * @brief Simulates an incoming MQTT message on a subscribed topic.
     * Triggers the callback if one is registered.
     */
    static void simulateIncomingMessage(const char* topic, const char* payload) {
        pendingMessageTopic = topic != nullptr ? topic : "";
        pendingMessagePayload = payload != nullptr ? payload : "";
        hasPendingMessage = true;
    }
    
    /**
     * @brief Processes pending messages by invoking the callback.
     * Called by loop() to simulate message delivery.
     */
    void processPendingMessages() {
        if (hasPendingMessage && messageCallback != nullptr) {
            const byte* payloadBytes = reinterpret_cast<const byte*>(pendingMessagePayload.c_str());
            messageCallback(pendingMessageTopic.c_str(), 
                          payloadBytes, pendingMessagePayload.length());
            hasPendingMessage = false;
        }
    }
};

#endif // FAKE_PUB_SUB_CLIENT_H
