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
    // 1. Guard capability health checks first before allocating memory
    if (!communicationTransportDriver.canTransmit()) {
        communicationTransportDriver.connect();
        return false;
    }
    
    // ... rest of the code ...
    // Wait, I need to fix `communicationTransportDriver`. 
    // It was in the private part of the base class. 
    // Since it's in the base class, and I made it `private`, `HttpGatewayClient` cannot access it!
    // I should make it `protected` in `GatewayClient`.

    // 2. Perform polymorphic package data serialization mapping
    String serializedPayloadOutput = abstractTelemetryPayload.serializeAsString();

    // 3. Execute HTTP Layer transactions safely within the NetworkWorker thread context
    HTTPClient httpClientEndpoint;
    httpClientEndpoint.begin(serviceEndpointUrl);
    httpClientEndpoint.addHeader("Content-Type", contentTypeHeader);

    int httpResponseStatusCode = httpClientEndpoint.POST(serializedPayloadOutput);
    httpClientEndpoint.end();

    // Standard HTTP Success Range definition (200 OK through 299)
    return (httpResponseStatusCode >= 200 && httpResponseStatusCode < 300);
}
