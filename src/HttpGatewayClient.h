#ifndef HTTP_GATEWAY_CLIENT_H
#define HTTP_GATEWAY_CLIENT_H

/**
 * @file HttpGatewayClient.h
 * @brief Declares an HTTP client for sending serialized telemetry payloads with authentication support.
 * @ingroup gateway_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-06-06
 * @version 1.0
 */

#include "GatewayClient.h"
#include "TelemetryPackage.h"
#include <HTTPClient.h>

/**
 * @brief Gateway adapter that sends `TelemetryPackage` payloads to an HTTP endpoint
 *        using configurable HTTP methods (GET, POST, PUT, PATCH, DELETE).
 * @ingroup gateway_module
 */
class HttpGatewayClient : public GatewayClient {
public:
    /**
     * @brief Strongly-typed HTTP method enumeration for switch-based dispatch.
     */
    enum class HttpMethod : uint8_t {
        Get,    ///< HTTP GET method for retrieving resources.
        Post,   ///< HTTP POST method for creating resources.
        Put,    ///< HTTP PUT method for replacing resources.
        Patch,  ///< HTTP PATCH method for partial updates.
        Delete  ///< HTTP DELETE method for removing resources.
    };

private:
    const char* contentTypeHeader; ///< Content-Type header for the HTTP request.
    const char* authorizationHeader; ///< Optional Authorization header (e.g., "Bearer <token>").
    HttpMethod httpMethod; ///< HTTP method for requests (GET, POST, PUT, PATCH, DELETE).
    int lastResponseStatusCode; ///< Last HTTP response status code from the server.
    uint32_t sendsSucceededCount; ///< Successful telemetry transmissions.
    uint32_t sendsFailedCount; ///< Failed telemetry transmissions.
    HTTPClient httpClientEndpoint; ///< HTTP client instance for sending requests.

    static constexpr size_t SERIALIZATION_BUFFER_SIZE = 512; ///< Maximum serialized payload size in bytes.
    char serializationBuffer[SERIALIZATION_BUFFER_SIZE]; ///< Fixed-size buffer for JSON serialization.
    static constexpr size_t RESPONSE_BUFFER_SIZE = 512; ///< Maximum response body size in bytes.
    char responseBuffer[RESPONSE_BUFFER_SIZE]; ///< Fixed-size buffer for HTTP response bodies.
    size_t responseBodyLength; ///< Length of the last response body received.

    static constexpr auto HTTP_HEADER_CONTENT_TYPE = "Content-Type"; ///< HTTP Content-Type header name.
    static constexpr auto HTTP_HEADER_AUTHORIZATION = "Authorization"; ///< HTTP Authorization header name.
    static constexpr auto DEFAULT_CONTENT_TYPE = "application/json"; ///< Default Content-Type for JSON payloads.

    /**
     * @brief Executes an HTTP request with the specified method and payload.
     * @param method HTTP method enum value.
     * @param payload Optional JSON payload body.
     * @return True if the server responded with a valid success status code (2xx).
     */
    bool executeHttpRequest(HttpMethod method, const char* payload);

public:
    /**
     * @param transportDriver Abstract reference handling physical link status.
     * @param targetEndpoint The RESTful API URL destination.
     * @param method HTTP method for requests. Default: Post.
     */
    HttpGatewayClient(ConnectivityDriver& transportDriver, const char* targetEndpoint,
                      HttpMethod method = HttpMethod::Post);
    ~HttpGatewayClient() override = default;

    /**
     * @brief Sets the Authorization header for subsequent HTTP requests.
     * @param authorizationHeaderValue Authorization header value (e.g., "Bearer <token>" or "ApiKey <key>").
     *        Pass nullptr to remove the authorization header.
     */
    void setAuthorizationHeader(const char* authorizationHeaderValue);

    /**
     * @brief Sets the HTTP method for subsequent requests.
     * @param method HTTP method enum value (Get, Post, Put, Patch, Delete).
     */
    void setHttpMethod(HttpMethod method);

    /**
     * @brief Returns the last HTTP response status code from the server.
     * @return HTTP status code (e.g., 200, 201, 400, 401, 403, 429, 500), or 0 if no request was made.
     */
    [[nodiscard]] int getLastResponseStatusCode() const;

    /**
     * @brief Returns the response body from the last HTTP request.
     * @return Null-terminated response body, or nullptr if no response was received.
     *         The buffer is overwritten on each request; copy the data if it must persist.
     */
    [[nodiscard]] const char* getResponseBody() const;

    /**
     * @brief Returns the size of the response body from the last HTTP request.
     * @return Response body length in bytes, or 0 if no response was received.
     */
    [[nodiscard]] size_t getResponseBodyLength() const;

    /**
     * @brief Inquires whether the underlying transport driver is connected.
     * @return `true` if the transport driver reports a connected state.
     */
    [[nodiscard]] bool isConnected() override;

    /**
     * @brief Returns send success/failure counters for diagnostics aggregation.
     * @return Statistics struct with cumulative send counts.
     */
    [[nodiscard]] GatewayStatistics getStatistics() const override;

    /**
     * @brief Sends a general HTTP request with an optional payload, independent of TelemetryPackage.
     *
     * Enables common patterns like fetching configuration (GET), sending commands (POST),
     * or clearing resources (DELETE) without requiring a TelemetryPackage. Uses the same
     * authentication, content-type, and statistics tracking as sendTelemetryRecord().
     * Must be called from the network task context (e.g., processQueuedTelemetryData or
     * maintainNetworkServices) for thread safety.
     *
     * @param method HTTP method enum value (Get, Post, Put, Patch, Delete).
     * @param payload Optional JSON payload body. Pass nullptr for GET/DELETE requests.
     * @return True if the server responded with a valid success status code (2xx).
     * @runs_on{network}
     */
    bool sendRequest(HttpMethod method, const char* payload = nullptr);

    /**
     * @brief Polling contract hook: Serializes and ships the payload via HTTP.
     * @param abstractTelemetryPayload Polymorphic payload to serialize and transmit.
     * @return True if the server responded with a valid success status code (2xx).
     */
    bool sendTelemetryRecord(const TelemetryPackage& abstractTelemetryPayload) override;
};

#endif // HTTP_GATEWAY_CLIENT_H