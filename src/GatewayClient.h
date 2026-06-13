#ifndef GATEWAY_CLIENT_H
#define GATEWAY_CLIENT_H

/**
 * @file GatewayClient.h
 * @brief Declares a generic telemetry gateway that delegates transport details.
 * @ingroup gateway_module
 * @date 2026-06-06
 * @version 1.0
 */

#include "ConnectivityDriver.h"
#include "TelemetryPackage.h"
#include <ArduinoJson.h>

/**
 * @name Gateway: Generic Abstraction
 * @ingroup gateway_module
 * @{
 */

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
     */
    virtual bool sendTelemetryRecord(const TelemetryPackage& abstractTelemetryPayload) = 0;
};

/** @} */ // End of Gateway: Generic Abstraction

#endif // GATEWAY_CLIENT_H