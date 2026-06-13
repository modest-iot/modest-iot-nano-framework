#ifndef WIFI_CONNECTIVITY_DRIVER_H
#define WIFI_CONNECTIVITY_DRIVER_H

/**
 * @file WiFiConnectivityDriver.h
 * @brief Declares an ESP32 Wi-Fi implementation of `ConnectivityDriver`.
 * @ingroup connectivity_module
 * @date 2026-06-06
 * @version 1.0
 */

#include <WiFi.h>
#include "ConnectivityDriver.h"

// Forward declarations notify the compiler of types without forcing
// heavy downstream library include leaks into the application layer.
class Client;

/**
 * @name Connectivity: Concrete Drivers
 * @ingroup connectivity_module
 * @{
 */

/**
 * @brief Connectivity adapter that manages Wi-Fi provisioning and socket access.
 * @ingroup connectivity_module
 */
class WiFiConnectivityDriver : public ConnectivityDriver {
private:
    const char* networkSsidIdentifier; ///< SSID of the network.
    const char* networkPasswordCredential; ///< Password for the network.

    // Aligned Naming Rule: Variables represent properties without 'is' prefixes
    volatile bool provisioningRequested; ///< Flag indicating if provisioning was requested.

    // Encapsulated concrete socket footprint hidden from outer layers
    WiFiClient* hardwareSocket{}; ///< Pointer to the hardware socket.

public:
    /**
     * @brief Constructs a Wi-Fi connectivity adapter.
     * @param ssid Wireless local network access point name.
     * @param password Authentication credential passphrase.
     */
    WiFiConnectivityDriver(const char* ssid, const char* password);

    ~WiFiConnectivityDriver() override;

    /**
     * @brief Asynchronously initiates connection sequences to the wireless access point.
     */
    void connect() override;

    /**
     * @brief Inquires whether the local interface is fully authenticated with an IP allocation.
     * @return `true` if Wi-Fi association and IP allocation are complete.
     */
    [[nodiscard]] bool isConnected() override;

    /**
     * @brief Intention-Revealing Query: Verifies whether the link layer is clear and ready.
     * @return `true` if the adapter can transmit outbound payloads.
     */
    [[nodiscard]] bool canTransmit() override;

    /**
     * @brief Explicit Protocol Bridge: Exposes the internal socket interface
     * upcasted cleanly to the generic Arduino Client contract.
     * @return Reference to the underlying network socket.
     */
    [[nodiscard]] Client& getNetworkSocket();
};

/** @} */ // End of Connectivity: Concrete Drivers

#endif // WIFI_CONNECTIVITY_DRIVER_H