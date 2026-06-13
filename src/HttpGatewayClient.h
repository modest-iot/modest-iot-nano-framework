#ifndef HTTP_GATEWAY_CLIENT_H
#define HTTP_GATEWAY_CLIENT_H

/**
 * @file HttpGatewayClient.h
 * @brief Declares an HTTP client for sending serialized telemetry payloads.
 * @ingroup gateway_module
 * @date 2026-06-06
 * @version 1.0
 */

#include "GatewayClient.h"
#include "TelemetryPackage.h"

/**
 * @name Gateway: Protocol-Specific Implementations
 * @ingroup gateway_module
 * @{
 */

/**
 * @brief Gateway adapter that posts `TelemetryPackage` payloads to an HTTP endpoint.
 * @ingroup gateway_module
 */
class HttpGatewayClient : public GatewayClient {
private:
    const char* contentTypeHeader; ///< Content-Type header for the HTTP request.

public:
    /**
     * @param transportDriver Abstract reference handling physical link status.
     * @param targetEndpoint The RESTful API URL destination.
     */
    HttpGatewayClient(ConnectivityDriver& transportDriver, const char* targetEndpoint);
    ~HttpGatewayClient() = default;

    /**
     * @brief Polling contract hook: Serializes and ships the payload via HTTP POST.
     * @param abstractTelemetryPayload Polymorphic payload to serialize and transmit.
     * @return True if the server responded with a valid success status code.
     */
    bool sendTelemetryRecord(const TelemetryPackage& abstractTelemetryPayload) override;
};

#endif // HTTP_GATEWAY_CLIENT_H