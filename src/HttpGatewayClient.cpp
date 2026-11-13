/**
 * @file HttpGatewayClient.cpp
 * @brief Implements HTTP transport for serialized telemetry packages with authentication support.
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-06-06
 * @version 1.0
 */

#include "HttpGatewayClient.h"
#include "ModestLog.h"
#include <Arduino.h>

HttpGatewayClient::HttpGatewayClient(ConnectivityDriver& transportDriver, const char* targetEndpoint,
                                     const HttpMethod method)
    : GatewayClient(transportDriver, targetEndpoint),
      contentTypeHeader(DEFAULT_CONTENT_TYPE),
      authorizationHeader(nullptr),
      httpMethod(method),
      lastResponseStatusCode(0),
      sendsSucceededCount(0),
      sendsFailedCount(0),
      serializationBuffer{},
      responseBuffer{},
      responseBodyLength(0)
{
}

void HttpGatewayClient::setAuthorizationHeader(const char* authorizationHeaderValue) {
    authorizationHeader = authorizationHeaderValue;
}

void HttpGatewayClient::setHttpMethod(const HttpMethod method) {
    httpMethod = method;
}

int HttpGatewayClient::getLastResponseStatusCode() const {
    return lastResponseStatusCode;
}

const char* HttpGatewayClient::getResponseBody() const {
    return responseBodyLength > 0 ? responseBuffer : nullptr;
}

size_t HttpGatewayClient::getResponseBodyLength() const {
    return responseBodyLength;
}

bool HttpGatewayClient::isConnected() {
    return communicationTransportDriver.isConnected() && httpClientEndpoint.connected();
}

GatewayStatistics HttpGatewayClient::getStatistics() const {
    GatewayStatistics stats;
    stats.sendsSucceeded = sendsSucceededCount;
    stats.sendsFailed = sendsFailedCount;
    return stats;
}

bool HttpGatewayClient::sendRequest(const HttpMethod method, const char* payload) {
    return executeHttpRequest(method, payload);
}

bool HttpGatewayClient::sendTelemetryRecord(const TelemetryPackage& abstractTelemetryPayload) {
    // Check transport capability before processing
    if (!communicationTransportDriver.canTransmit()) {
        communicationTransportDriver.connect();
        sendsFailedCount++;
        return false;
    }

    // Serialize telemetry payload into bounded buffer
    JsonDocument payload;
    abstractTelemetryPayload.addEnvelopeMetadata(payload);
    abstractTelemetryPayload.serialize(payload);
    if (const size_t serializedLength = serializeJson(payload, serializationBuffer, sizeof(serializationBuffer)); serializedLength == 0) {
        MODEST_LOG_W("HTTP", "Payload too large for serialization buffer (%u bytes)", SERIALIZATION_BUFFER_SIZE);
        sendsFailedCount++;
        return false;
    }

    return executeHttpRequest(httpMethod, serializationBuffer);
}

bool HttpGatewayClient::executeHttpRequest(const HttpMethod method, const char* payload) {
    // Check transport capability before processing
    if (!communicationTransportDriver.canTransmit()) {
        communicationTransportDriver.connect();
        sendsFailedCount++;
        return false;
    }

    // Execute HTTP request within the NetworkWorker thread context
    httpClientEndpoint.begin(serviceEndpointUrl);
    httpClientEndpoint.addHeader(HTTP_HEADER_CONTENT_TYPE, contentTypeHeader);

    // Add authorization header if configured
    if (authorizationHeader != nullptr) {
        httpClientEndpoint.addHeader(HTTP_HEADER_AUTHORIZATION, authorizationHeader);
    }

    // Send request using the specified HTTP method
    // ReSharper disable once CppEntityAssignedButNoRead
    const char* httpMethodString;
    int httpResponseStatusCode;
    switch (method) {
        case HttpMethod::Get:
            // ReSharper disable once CppAssignedValueIsNeverUsed
            httpMethodString = "GET";
            httpResponseStatusCode = httpClientEndpoint.GET();
            break;
        case HttpMethod::Post:
            // ReSharper disable once CppAssignedValueIsNeverUsed
            httpMethodString = "POST";
            httpResponseStatusCode = httpClientEndpoint.POST(payload != nullptr ? payload : "");
            break;
        case HttpMethod::Put:
            // ReSharper disable once CppAssignedValueIsNeverUsed
            httpMethodString = "PUT";
            httpResponseStatusCode = httpClientEndpoint.PUT(payload != nullptr ? payload : "");
            break;
        case HttpMethod::Patch:
            // ReSharper disable once CppAssignedValueIsNeverUsed
            httpMethodString = "PATCH";
            httpResponseStatusCode = httpClientEndpoint.sendRequest("PATCH", payload != nullptr ? payload : "");
            break;
        case HttpMethod::Delete:
            // ReSharper disable once CppAssignedValueIsNeverUsed
            httpMethodString = "DELETE";
            httpResponseStatusCode = httpClientEndpoint.sendRequest("DELETE", "");
            break;
        default:
            // ReSharper disable once CppAssignedValueIsNeverUsed
            httpMethodString = "UNKNOWN";
            MODEST_LOG_W("HTTP", "Unsupported HTTP method: %s", httpMethodString);
            sendsFailedCount++;
            lastResponseStatusCode = 0;
            httpClientEndpoint.end();
            return false;
    }

    lastResponseStatusCode = httpResponseStatusCode;

    // Read response body into fixed-size buffer using getString()
    responseBodyLength = 0;
    if (httpResponseStatusCode > 0) {
        const String responseString = httpClientEndpoint.getString();
        responseBodyLength = responseString.length();
        if (responseBodyLength > 0) {
            responseBodyLength = responseBodyLength < RESPONSE_BUFFER_SIZE ? responseBodyLength : RESPONSE_BUFFER_SIZE - 1;
            memcpy(responseBuffer, responseString.c_str(), responseBodyLength);
            responseBuffer[responseBodyLength] = '\0';
        }
    }

    httpClientEndpoint.end();

    // Standard HTTP success range (200 OK through 299)
    const bool success = httpResponseStatusCode >= 200 && httpResponseStatusCode < 300;
    if (success) {
        sendsSucceededCount++;
        MODEST_LOG_D("HTTP", "Sent request via %s to %s (status: %d)", httpMethodString, serviceEndpointUrl, httpResponseStatusCode);
    } else {
        sendsFailedCount++;
        MODEST_LOG_W("HTTP", "Failed request via %s to %s (status: %d)", httpMethodString, serviceEndpointUrl, httpResponseStatusCode);
    }

    return success;
}
