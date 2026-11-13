#include <time.h>
/**
 * @file test.cpp
 * @brief Host tests for HttpGatewayClient HTTP telemetry gateway.
 * @ingroup gateway_module
 * @date 2026-11-12
 * @version 1.0
 */

#include "HttpGatewayClient.h"
#include "WiFiConnectivityDriver.h"
#include "TelemetryPackage.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <cassert>
#include <cstring>
#include <string>

namespace {

/**
 * @brief Minimal telemetry payload for testing serialization.
 */
class TestTelemetryPayload : public TelemetryPackage {
protected:
    void serialize(JsonDocument& doc) const override {
        doc["temperature"] = 24.5f;
        doc["humidity"] = 65.0f;
    }
};

} // namespace

[[nodiscard]] int main() {
    // 1. Constructor initializes statistics to zero
    {
        WiFiConnectivityDriver driver("test_ssid", "test_password");
        HttpGatewayClient gatewayClient(driver, "http://example.com/api");

        auto stats = gatewayClient.getStatistics();
        assert(stats.sendsSucceeded == 0);
        assert(stats.sendsFailed == 0);
        assert(!gatewayClient.isConnected());
        assert(gatewayClient.getLastResponseStatusCode() == 0);
        assert(gatewayClient.getResponseBody() == nullptr);
        assert(gatewayClient.getResponseBodyLength() == 0);
    }

    // 2. sendTelemetryRecord fails when transport cannot transmit
    {
        WiFiConnectivityDriver driver("test_ssid", "test_password");
        HttpGatewayClient gatewayClient(driver, "http://example.com/api");

        TestTelemetryPayload payload;
        assert(!gatewayClient.sendTelemetryRecord(payload));

        auto stats = gatewayClient.getStatistics();
        assert(stats.sendsSucceeded == 0);
        assert(stats.sendsFailed == 1);
    }

    // 3. sendTelemetryRecord succeeds when transport is connected
    {
        HTTPClient::resetMockState();
        WiFiConnectivityDriver driver("test_ssid", "test_password");
        driver.connect();
        WiFi.setStatus(WL_CONNECTED);
        HTTPClient::simulateSuccessResponse(200, "{\"status\":\"ok\"}");

        HttpGatewayClient gatewayClient(driver, "http://example.com/api");

        TestTelemetryPayload payload;
        assert(gatewayClient.sendTelemetryRecord(payload));

        auto stats = gatewayClient.getStatistics();
        assert(stats.sendsSucceeded == 1);
        assert(stats.sendsFailed == 0);
        assert(gatewayClient.getLastResponseStatusCode() == 200);
        assert(gatewayClient.getResponseBody() != nullptr);
        assert(std::string(gatewayClient.getResponseBody()) == "{\"status\":\"ok\"}");
    }

    // 4. sendRequest with different HTTP methods
    {
        HTTPClient::resetMockState();
        WiFiConnectivityDriver driver("test_ssid", "test_password");
        driver.connect();
        WiFi.setStatus(WL_CONNECTED);

        HttpGatewayClient gatewayClient(driver, "http://example.com/api");

        // Test GET
        HTTPClient::simulateSuccessResponse(200);
        assert(gatewayClient.sendRequest(HttpGatewayClient::HttpMethod::Get));
        assert(HTTPClient::getCount == 1);

        // Test POST
        HTTPClient::simulateSuccessResponse(201, "{\"id\":123}");
        assert(gatewayClient.sendRequest(HttpGatewayClient::HttpMethod::Post, "{\"data\":\"test\"}"));
        assert(HTTPClient::postCount == 1);
        assert(gatewayClient.getLastResponseStatusCode() == 201);

        // Test PUT
        HTTPClient::simulateSuccessResponse(200);
        assert(gatewayClient.sendRequest(HttpGatewayClient::HttpMethod::Put, "{\"data\":\"updated\"}"));
        assert(HTTPClient::putCount == 1);

        // Test DELETE
        HTTPClient::simulateSuccessResponse(204);
        assert(gatewayClient.sendRequest(HttpGatewayClient::HttpMethod::Delete));
        assert(HTTPClient::deleteCount == 1);
    }

    // 5. setHttpMethod changes the HTTP method for sendTelemetryRecord
    {
        HTTPClient::resetMockState();
        WiFiConnectivityDriver driver("test_ssid", "test_password");
        driver.connect();
        WiFi.setStatus(WL_CONNECTED);
        HTTPClient::simulateSuccessResponse(200);

        HttpGatewayClient gatewayClient(driver, "http://example.com/api", HttpGatewayClient::HttpMethod::Get);
        gatewayClient.setHttpMethod(HttpGatewayClient::HttpMethod::Put);

        TestTelemetryPayload payload;
        assert(gatewayClient.sendTelemetryRecord(payload));
        assert(HTTPClient::putCount == 1);
    }

    // 6. setAuthorizationHeader sets the auth header
    {
        WiFiConnectivityDriver driver("test_ssid", "test_password");
        HttpGatewayClient gatewayClient(driver, "http://example.com/api");
        gatewayClient.setAuthorizationHeader("Bearer test_token_123");
        // Auth header is stored internally; verify it doesn't crash
    }

    // 7. Failed HTTP response increments failure count
    {
        HTTPClient::resetMockState();
        WiFiConnectivityDriver driver("test_ssid", "test_password");
        driver.connect();
        WiFi.setStatus(WL_CONNECTED);
        HTTPClient::setResponse(401, "{\"error\":\"unauthorized\"}");

        HttpGatewayClient gatewayClient(driver, "http://example.com/api");

        TestTelemetryPayload payload;
        assert(!gatewayClient.sendTelemetryRecord(payload));

        auto stats = gatewayClient.getStatistics();
        assert(stats.sendsSucceeded == 0);
        assert(stats.sendsFailed == 1);
        assert(gatewayClient.getLastResponseStatusCode() == 401);
    }

    // 8. Response body handling
    {
        HTTPClient::resetMockState();
        WiFiConnectivityDriver driver("test_ssid", "test_password");
        driver.connect();
        WiFi.setStatus(WL_CONNECTED);
        HTTPClient::simulateSuccessResponse(200, "{\"message\":\"success\",\"count\":5}");

        HttpGatewayClient gatewayClient(driver, "http://example.com/api");

        assert(gatewayClient.sendRequest(HttpGatewayClient::HttpMethod::Post, "{\"query\":\"data\"}"));
        const char* body = gatewayClient.getResponseBody();
        assert(body != nullptr);
        assert(std::string(body).find("\"message\"") != std::string::npos);
        assert(gatewayClient.getResponseBodyLength() > 0);
    }

    return 0;
}
