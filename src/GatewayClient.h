#ifndef GATEWAY_CLIENT_H
#define GATEWAY_CLIENT_H

/**
 * @file GatewayClient.h
 * @brief Declares a generic telemetry gateway that delegates transport details.
 * @ingroup gateway_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-06-06
 * @version 1.0
 */

#include "ConnectivityDriver.h"
#include "TelemetryPackage.h"

/**
 * @brief Send success/failure counters for gateway diagnostics.
 * @ingroup gateway_module
 */
struct GatewayStatistics {
    uint32_t sendsSucceeded = 0; ///< Successful telemetry transmissions.
    uint32_t sendsFailed = 0; ///< Failed telemetry transmissions.
};

/**
 * @brief Generic gateway client that serializes telemetry and sends it through a connectivity driver.
 * @ingroup gateway_module
 */
class GatewayClient {
protected:
    ConnectivityDriver& communicationTransportDriver; ///< Reference to the connectivity driver.
    const char* serviceEndpointUrl; ///< The endpoint URL.

public:
    /**
     * @brief Constructs a generic gateway client.
     * @param transportDriver Reference to the connectivity driver.
     * @param targetEndpoint The target endpoint URL.
     */
    GatewayClient(ConnectivityDriver& transportDriver, const char* targetEndpoint)
        : communicationTransportDriver(transportDriver), serviceEndpointUrl(targetEndpoint) {}

    virtual ~GatewayClient() = default;

    /**
     * @brief Polymorphically serializes any payload inheriting from TelemetryPackage and ships it.
     * @param abstractTelemetryPayload Polymorphic payload to serialize and transmit.
     * @return `true` when payload transmission succeeds, otherwise `false`.
     * @runs_on{network}
     */
    virtual bool sendTelemetryRecord(const TelemetryPackage& abstractTelemetryPayload) = 0;

    /**
     * @brief Periodic background upkeep routine for network and session state (e.g. keep-alive ping).
     * Subclasses override this method to perform non-blocking connection maintenance.
     * @runs_on{network}
     */
    virtual void service() {}

    /**
     * @brief Ergonomic alias for service() to maintain parity with Arduino idioms.
     */
    virtual void loop() {
        service();
    }

    /**
     * @brief Inquires whether the gateway and underlying transport are connected.
     * @return `true` if connected and capable of transmitting data.
     */
    [[nodiscard]] virtual bool isConnected() {
        return communicationTransportDriver.isConnected();
    }

    /**
     * @brief Returns send success/failure counters for diagnostics aggregation.
     * @return Statistics struct with cumulative send counts.
     */
    [[nodiscard]] virtual GatewayStatistics getStatistics() const {
        return {}; // Default: no tracking
    }

    /**
     * @brief Updates the target endpoint URL for subsequent requests.
     * @param targetEndpoint The new endpoint URL.
     * Useful when switching between authentication and telemetry endpoints.
     */
    void setServiceEndpointUrl(const char* targetEndpoint) {
        serviceEndpointUrl = targetEndpoint;
    }
};

#endif // GATEWAY_CLIENT_H