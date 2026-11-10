/**
 * @file WiFiConnectivityDriver.cpp
 * @brief Implements Wi-Fi provisioning, connection checks, and socket access.
 * @date 2026-06-06
 * @version 1.0
 */

#include <Arduino.h>
#include "WiFiConnectivityDriver.h"
#include "ModestLog.h"

WiFiConnectivityDriver::WiFiConnectivityDriver(const char* ssid, const char* password)
    : networkSsidIdentifier(ssid),
      networkPasswordCredential(password),
      provisioningRequested(false),
      lastConnectAttemptTimestamp(0),
      reconnectIntervalMilliseconds(10000)
{
    // Concrete allocation happens safely hidden away within the transport domain constructor
    hardwareSocket = new WiFiClient();
    // Prevents the underlying Espressif SDK from writing Wi-Fi credentials to the physical NVS flash sector
    // during every connection loop.
    WiFi.persistent(false);
}

WiFiConnectivityDriver::~WiFiConnectivityDriver() {
    if (hardwareSocket != nullptr) {
        delete hardwareSocket;
        hardwareSocket = nullptr;
    }
}

void WiFiConnectivityDriver::connect() {
    // Avoid re-triggering allocation requests if connection handshakes are actively executing
    // unless the cooldown period has expired to allow recovering from transient AP outages.
    if (WiFi.status() != WL_CONNECTED) {
        if (const unsigned long currentTimestamp = millis(); !provisioningRequested || lastConnectAttemptTimestamp == 0 ||
                                                       currentTimestamp - lastConnectAttemptTimestamp >= reconnectIntervalMilliseconds) {
            provisioningRequested = true;
            lastConnectAttemptTimestamp = currentTimestamp;
            MODEST_LOG_I("WiFi", "Initiating Wi-Fi connection to SSID: %s", networkSsidIdentifier);
            WiFi.begin(networkSsidIdentifier, networkPasswordCredential);
        }
    }
}

bool WiFiConnectivityDriver::isConnected() {
    const bool currentLinkStatus = WiFi.status() == WL_CONNECTED;
    if (currentLinkStatus) {
        provisioningRequested = false; // Reset volatile transit flag upon verified link auth
    }
    return currentLinkStatus;
}

bool WiFiConnectivityDriver::canTransmit() {
    // Acts strictly as an agnostic Layer 1-4 validation checkpoint
    return isConnected();
}

bool WiFiConnectivityDriver::transmit(const char* target, const char* data) {
    (void)target;
    (void)data;
    // Wi-Fi telemetry transmission is mediated via getNetworkSocket() and GatewayClient instances.
    return false;
}

Client& WiFiConnectivityDriver::getNetworkSocket() const {
    // Returns the concrete reference upcasted implicitly to a generic Client reference interface.
    // Application layer components interact with it polymorphically without ever seeing <WiFi.h>.
    return *hardwareSocket;
}

void WiFiConnectivityDriver::setReconnectInterval(const unsigned long intervalInMilliseconds) {
    reconnectIntervalMilliseconds = intervalInMilliseconds;
}

unsigned long WiFiConnectivityDriver::getReconnectInterval() const {
    return reconnectIntervalMilliseconds;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
wl_status_t WiFiConnectivityDriver::getWiFiStatus() const {
    return WiFi.status();
}

void WiFiConnectivityDriver::disconnect() {
    WiFi.disconnect();
    provisioningRequested = false;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
IPAddress WiFiConnectivityDriver::getLocalIpAddress() const {
    return WiFi.localIP();
}

// ReSharper disable once CppMemberFunctionMayBeStatic
int32_t WiFiConnectivityDriver::getSignalStrengthInDbm() const {
    return WiFi.RSSI();
}

bool WiFiConnectivityDriver::waitForConnection(unsigned long timeoutMilliseconds) {
    if (isConnected()) {
        return true;
    }
    connect();
    unsigned long startTimestamp = millis();
    while (millis() - startTimestamp < timeoutMilliseconds) {
        if (isConnected()) {
            return true;
        }
        delay(100);
    }
    return isConnected();
}