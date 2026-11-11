/**
 * @file HttpGatewayClient.cpp
 * @brief Implements HTTP transport for serialized telemetry packages with authentication support.
 * @date 2026-06-06
 * @version 1.0
 */

#include "HttpGatewayClient.h"
#include "ModestLog.h"
#include <Arduino.h>
#include <HTTPClient.h>
// ReSharper disable once CppUnusedIncludeDirective
#include <cstring>

HttpGatewayClient::HttpGatewayClient(ConnectivityDriver& transportDriver, const char* targetEndpoint,
                                     const char* method)
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

void HttpGatewayClient::setHttpMethod(const char* method) {
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
    const bool connectionStatus = GatewayClient::isConnected();
    MODEST_LOG_I("HTTP", "Transport connection status is %s", connectionStatus ? "connected" : "disconnected");
    return connectionStatus;
}

GatewayStatistics HttpGatewayClient::getStatistics() const {
    GatewayStatistics stats;
    stats.sendsSucceeded = sendsSucceededCount;
    stats.sendsFailed = sendsFailedCount;
    return stats;
}

bool HttpGatewayClient::sendRequest(const char* method, const char* payload) {
    // Check transport capability before processing
    if (!communicationTransportDriver.canTransmit()) {
        communicationTransportDriver.connect();
        sendsFailedCount++;
        return false;
    }

    // Execute HTTP request within the NetworkWorker thread context
    HTTPClient httpClientEndpoint;
    httpClientEndpoint.begin(serviceEndpointUrl);
    httpClientEndpoint.addHeader(HTTP_HEADER_CONTENT_TYPE, contentTypeHeader);

    // Add authorization header if configured
    if (authorizationHeader != nullptr) {
        httpClientEndpoint.addHeader(HTTP_HEADER_AUTHORIZATION, authorizationHeader);
    }

    // Send request using the specified HTTP method
    int httpResponseStatusCode;
    if (strcmp(method, HTTP_METHOD_GET) == 0) {
        httpResponseStatusCode = httpClientEndpoint.GET();
    } else if (strcmp(method, HTTP_METHOD_POST) == 0) {
        httpResponseStatusCode = httpClientEndpoint.POST(payload != nullptr ? payload : "");
    } else if (strcmp(method, HTTP_METHOD_PUT) == 0) {
        httpResponseStatusCode = httpClientEndpoint.PUT(payload != nullptr ? payload : "");
    } else if (strcmp(method, HTTP_METHOD_PATCH) == 0) {
        httpResponseStatusCode = httpClientEndpoint.sendRequest(HTTP_METHOD_PATCH, payload != nullptr ? payload : "");
    } else if (strcmp(method, HTTP_METHOD_DELETE) == 0) {
        httpResponseStatusCode = httpClientEndpoint.sendRequest(HTTP_METHOD_DELETE, "");
    } else {
        MODEST_LOG_W("HTTP", "Unsupported HTTP method: %s", method);
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
        MODEST_LOG_D("HTTP", "Sent request via %s to %s (status: %d)", method, serviceEndpointUrl, httpResponseStatusCode);
    } else {
        sendsFailedCount++;
        MODEST_LOG_W("HTTP", "Failed request via %s to %s (status: %d)", method, serviceEndpointUrl, httpResponseStatusCode);
    }

    return success;
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

    // Execute HTTP request within the NetworkWorker thread context
    HTTPClient httpClientEndpoint;
    httpClientEndpoint.begin(serviceEndpointUrl);
    httpClientEndpoint.addHeader(HTTP_HEADER_CONTENT_TYPE, contentTypeHeader);

    // Add authorization header if configured
    if (authorizationHeader != nullptr) {
        httpClientEndpoint.addHeader(HTTP_HEADER_AUTHORIZATION, authorizationHeader);
    }

    // Send request using the configured HTTP method
    int httpResponseStatusCode;
    if (strcmp(httpMethod, HTTP_METHOD_GET) == 0) {
        httpResponseStatusCode = httpClientEndpoint.GET();
    } else if (strcmp(httpMethod, HTTP_METHOD_POST) == 0) {
        httpResponseStatusCode = httpClientEndpoint.POST(serializationBuffer);
    } else if (strcmp(httpMethod, HTTP_METHOD_PUT) == 0) {
        httpResponseStatusCode = httpClientEndpoint.PUT(serializationBuffer);
    } else if (strcmp(httpMethod, HTTP_METHOD_PATCH) == 0) {
        httpResponseStatusCode = httpClientEndpoint.sendRequest(HTTP_METHOD_PATCH, serializationBuffer);
    } else if (strcmp(httpMethod, HTTP_METHOD_DELETE) == 0) {
        httpResponseStatusCode = httpClientEndpoint.sendRequest(HTTP_METHOD_DELETE, "");
    } else {
        MODEST_LOG_W("HTTP", "Unsupported HTTP method: %s", httpMethod);
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
        MODEST_LOG_D("HTTP", "Sent telemetry via %s to %s (status: %d)", httpMethod, serviceEndpointUrl, httpResponseStatusCode);
    } else {
        sendsFailedCount++;
        MODEST_LOG_W("HTTP", "Failed to send telemetry via %s to %s (status: %d)", httpMethod, serviceEndpointUrl, httpResponseStatusCode);
    }

    return success;
}
