/**
 * @file HttpEdgeGatewayClient.cpp
 * @brief Implements HTTP POST transport for serialized telemetry packages.
 * @date 2026-06-06
 * @version 1.0
 */

#include "HttpEdgeGatewayClient.h"
#include <Arduino.h>
#include <HTTPClient.h> 
#include <ArduinoJson.h>

HttpEdgeGatewayClient::HttpEdgeGatewayClient(ConnectivityDriver& transportDriver, const char* targetEndpoint)
    : communicationTransportDriver(transportDriver), 
      serviceEndpointUrl(targetEndpoint),
      contentTypeHeader("application/json") {}

bool HttpEdgeGatewayClient::sendTelemetryRecord(const TelemetryPackage& abstractTelemetryPayload) {
    // 1. Guard capability health checks first before allocating memory
    if (!communicationTransportDriver.canTransmit()) {
        communicationTransportDriver.connect(); 
        return false;
    }

    // 2. Perform polymorphic package data serialization mapping
    JsonDocument serializationDocument;
    abstractTelemetryPayload.serialize(serializationDocument);

    String serializedPayloadOutput;
    serializeJson(serializationDocument, serializedPayloadOutput);

    // 3. Execute HTTP Layer transactions safely within the NetworkWorker thread context
    HTTPClient httpClientEndpoint;
    httpClientEndpoint.begin(serviceEndpointUrl);
    httpClientEndpoint.addHeader("Content-Type", contentTypeHeader);

    int httpResponseStatusCode = httpClientEndpoint.POST(serializedPayloadOutput);
    httpClientEndpoint.end();

    // Standard HTTP Success Range definition (200 OK through 299)
    return (httpResponseStatusCode >= 200 && httpResponseStatusCode < 300);
}