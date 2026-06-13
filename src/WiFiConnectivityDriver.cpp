/**
 * @file WiFiConnectivityDriver.cpp
 * @brief Implements Wi-Fi provisioning, connection checks, and socket access.
 * @date 2026-06-06
 * @version 1.0
 */

#include <Arduino.h>
#include <WiFi.h>
#include "WiFiConnectivityDriver.h"


WiFiConnectivityDriver::WiFiConnectivityDriver(const char* ssid, const char* password)
    : networkSsidIdentifier(ssid),
      networkPasswordCredential(password),
      provisioningRequested(false)
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
    if (WiFi.status() != WL_CONNECTED && !provisioningRequested) {
        provisioningRequested = true;
        WiFi.begin(networkSsidIdentifier, networkPasswordCredential);
    }
}

bool WiFiConnectivityDriver::isConnected() {
    bool currentLinkStatus = (WiFi.status() == WL_CONNECTED);
    if (currentLinkStatus) {
        provisioningRequested = false; // Reset volatile transit flag upon verified link auth
    }
    return currentLinkStatus;
}

bool WiFiConnectivityDriver::canTransmit() {
    // Acts strictly as an agnostic Layer 1-4 validation checkpoint
    return isConnected();
}

Client& WiFiConnectivityDriver::getNetworkSocket() {
    // Returns the concrete reference upcasted implicitly to a generic Client reference interface.
    // Application layer components interact with it polymorphically without ever seeing <WiFi.h>.
    return *hardwareSocket;
}