#ifndef WIFI_CONNECTIVITY_DRIVER_H
#define WIFI_CONNECTIVITY_DRIVER_H

#include "ConnectivityDriver.h"

// Forward declarations notify the compiler of types without forcing
// heavy downstream library include leaks into the application layer.
class WiFiClient;
class Client;

class WiFiConnectivityDriver : public ConnectivityDriver {
private:
    const char* networkSsidIdentifier;
    const char* networkPasswordCredential;

    // Aligned Naming Rule: Variables represent properties without 'is' prefixes
    bool provisioningRequested;

    // Encapsulated concrete socket footprint hidden from outer layers
    WiFiClient* hardwareSocket;

public:
    /**
     * @param ssid Wireless local network access point name.
     * @param password Authentication credential passphrase.
     */
    WiFiConnectivityDriver(const char* ssid, const char* password);

    virtual ~WiFiConnectivityDriver() override;

    /**
     * @brief Asynchronously initiates connection sequences to the wireless access point.
     */
    void connect() override;

    /**
     * @brief Inquires whether the local interface is fully authenticated with an IP allocation.
     */
    bool isConnected() override;

    /**
     * @brief Intention-Revealing Query: Verifies whether the link layer is clear and ready.
     */
    bool canTransmit() override;

    /**
     * @brief Explicit Protocol Bridge: Exposes the internal socket interface
     * upcasted cleanly to the generic Arduino Client contract.
     */
    Client& getNetworkSocket();
};

#endif // WIFI_CONNECTIVITY_DRIVER_H