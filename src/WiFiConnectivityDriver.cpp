#include "WiFiConnectivityDriver.h"

WiFiConnectivityDriver::WiFiConnectivityDriver(const char* ssid, const char* password)
    : networkSsidIdentifier(ssid), 
      networkPasswordCredential(password),
      provisioningRequested(false) {}

void WiFiConnectivityDriver::connect() {
    if (WiFi.status() != WL_CONNECTED && !provisioningRequested) {
        provisioningRequested = true;
        WiFi.begin(networkSsidIdentifier, networkPasswordCredential);
    }
}

bool WiFiConnectivityDriver::isConnected() {
    bool currentLinkStatus = (WiFi.status() == WL_CONNECTED);
    if (currentLinkStatus) {
        provisioningRequested = false; 
    }
    return currentLinkStatus;
}

bool WiFiConnectivityDriver::canTransmit() {
    // The driver returns true only when the physical and data-link layers 
    // are completely stable and ready to carry socket transactions.
    return isConnected();
}