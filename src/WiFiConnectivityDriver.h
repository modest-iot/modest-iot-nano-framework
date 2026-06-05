#ifndef WIFI_CONNECTIVITY_DRIVER_H
#define WIFI_CONNECTIVITY_DRIVER_H

#include "ConnectivityDriver.h"
#include <WiFi.h>

class WiFiConnectivityDriver : public ConnectivityDriver {
private:
    const char* networkSsidIdentifier;
    const char* networkPasswordCredential;
    
    // Aligned Rule: Property variables hold state without 'is' prefixes
    bool provisioningRequested;

public:
    WiFiConnectivityDriver(const char* ssid, const char* password);
    virtual ~WiFiConnectivityDriver() override = default;

    void connect() override;
    bool isConnected() override;
    
    // Implements the purified capability query hook
    bool canTransmit() override;
};

#endif // WIFI_CONNECTIVITY_DRIVER_H