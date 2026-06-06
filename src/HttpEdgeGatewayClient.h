#ifndef HTTP_EDGE_GATEWAY_CLIENT_H
#define HTTP_EDGE_GATEWAY_CLIENT_H

/**
 * @file HttpEdgeGatewayClient.h
 * @brief Declares an HTTP client for sending serialized telemetry payloads.
 */

#include "ConnectivityDriver.h"
#include "TelemetryPackage.h"

/**
 * @brief Gateway adapter that posts `TelemetryPackage` payloads to an HTTP endpoint.
 */
class HttpEdgeGatewayClient {
private:
    ConnectivityDriver& communicationTransportDriver;
    const char* serviceEndpointUrl;
    const char* contentTypeHeader;

public:
    /**
     * @param transportDriver Abstract reference handling physical link status.
     * @param targetEndpoint The RESTful API URL destination.
     */
    HttpEdgeGatewayClient(ConnectivityDriver& transportDriver, const char* targetEndpoint);
    ~HttpEdgeGatewayClient() = default;

    /**
     * @brief Polling contract hook: Serializes and ships the payload via HTTP POST.
     * @param abstractTelemetryPayload Polymorphic payload to serialize and transmit.
     * @return True if the server responded with a valid success status code.
     */
    bool sendTelemetryRecord(const TelemetryPackage& abstractTelemetryPayload);
};

#endif // HTTP_EDGE_GATEWAY_CLIENT_H