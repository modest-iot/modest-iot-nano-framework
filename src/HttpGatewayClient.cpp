/**
 * @file HttpGatewayClient.cpp
 * @brief Implements HTTP POST transport for serialized telemetry packages.
 * @date 2026-06-06
 * @version 1.0
 */

#include "HttpGatewayClient.h"
#include <Arduino.h>
#include <HTTPClient.h>

HttpGatewayClient::HttpGatewayClient(ConnectivityDriver& transportDriver, const char* targetEndpoint)
    : GatewayClient(transportDriver, targetEndpoint),
      contentTypeHeader("application/json") {}

// ReSharper disable once CppMemberFunctionMayBeConst
bool HttpGatewayClient::sendTelemetryRecord(const TelemetryPackage& abstractTelemetryPayload) {
    // Check transport capability before allocating memory
    if (!communicationTransportDriver.canTransmit()) {
        communicationTransportDriver.connect();
        return false;
    }

    // Serialize the telemetry package to JSON
    const String serializedPayloadOutput = abstractTelemetryPayload.toJsonString();

    // Execute HTTP POST within the NetworkWorker thread context
    HTTPClient httpClientEndpoint;
    httpClientEndpoint.begin(serviceEndpointUrl);
    httpClientEndpoint.addHeader("Content-Type", contentTypeHeader);

    const int httpResponseStatusCode = httpClientEndpoint.POST(serializedPayloadOutput);
    httpClientEndpoint.end();

    // Standard HTTP Success Range definition (200 OK through 299)
    return (httpResponseStatusCode >= 200 && httpResponseStatusCode < 300);
}
