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
 * @brief Connectivity adapter that manages Wi-Fi provisioning and socket access.
 * @ingroup connectivity_module
 */
class WiFiConnectivityDriver : public ConnectivityDriver {
private:
    const char* networkSsidIdentifier; ///< SSID of the network.
    const char* networkPasswordCredential; ///< Password for the network.

    // Aligned Naming Rule: Variables represent properties without 'is' prefixes
    volatile bool provisioningRequested; ///< Flag indicating if provisioning was requested.
    unsigned long lastConnectAttemptTimestamp; ///< Timestamp of the last connection attempt.
    unsigned long reconnectIntervalMilliseconds; ///< Cooldown interval between reconnection attempts in milliseconds.

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
     * @brief Evaluates whether outbound transmission is possible.
     * @note WiFiConnectivityDriver delegates actual payload transmission to socket-based GatewayClient instances.
     * @param target Target identifier (unused).
     * @param data Payload data string (unused).
     * @return `false` as Wi-Fi telemetry is managed via Client socket gateways.
     */
    bool transmit(const char* target, const char* data) override;

    /**
     * @brief Explicit Protocol Bridge: Exposes the internal socket interface
     * upcasted cleanly to the generic Arduino Client contract.
     * @return Reference to the underlying network socket.
     */
    [[nodiscard]] Client& getNetworkSocket() const;

    /**
     * @brief Sets the cooldown interval between Wi-Fi connection attempts.
     * @param intervalInMilliseconds Cooldown period in milliseconds (default: 10000ms).
     */
    void setReconnectInterval(unsigned long intervalInMilliseconds);

    /**
     * @brief Gets the configured cooldown interval between Wi-Fi connection attempts.
     * @return Cooldown period in milliseconds.
     */
    [[nodiscard]] unsigned long getReconnectInterval() const;

    /**
     * @brief Returns the detailed Wi-Fi connection status from the ESP32 SDK.
     * @return Raw wl_status_t value (WL_CONNECTED, WL_NO_SSID_AVAIL, etc.).
     */
    [[nodiscard]] wl_status_t getWiFiStatus() const;

    /**
     * @brief Disconnects from the current Wi-Fi access point.
     * @note Useful before entering deep sleep to conserve power.
     */
    void disconnect();

    /**
     * @brief Returns the IP address assigned by the access point.
     * @return The local IP address after a successful connection.
     */
    [[nodiscard]] IPAddress getLocalIpAddress() const;

    /**
     * @brief Returns the signal strength in dBm.
     * @return RSSI value (negative, e.g., -50 dBm for strong signal).
     */
    [[nodiscard]] int32_t getSignalStrengthInDbm() const;

    /**
     * @brief Blocks until Wi-Fi connects or the timeout expires.
     * @param timeoutMilliseconds Maximum time to wait in milliseconds.
     * @return `true` if connected within the timeout, `false` otherwise.
     */
    bool waitForConnection(unsigned long timeoutMilliseconds);
};

#endif // WIFI_CONNECTIVITY_DRIVER_H