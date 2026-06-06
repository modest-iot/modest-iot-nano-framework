#ifndef EDGE_GATEWAY_CLIENT_H
#define EDGE_GATEWAY_CLIENT_H

/**
 * @file EdgeGatewayClient.h
 * @brief Declares a generic telemetry gateway that delegates transport details.
 * @ingroup gateway_module
 */

#include "ConnectivityDriver.h"
#include "TelemetryPackage.h"

/**
 * @name Gateway: Generic Abstraction
 * @ingroup gateway_module
 * @{
 */

/**
 * @brief Generic gateway client that serializes telemetry and sends it through a connectivity driver.
 * @ingroup gateway_module
 */
class EdgeGatewayClient {
private:
    ConnectivityDriver& communicationTransportDriver;
    const char* serviceEndpointUrl;

public:
    EdgeGatewayClient(ConnectivityDriver& transportDriver, const char* targetEndpoint)
        : communicationTransportDriver(transportDriver), serviceEndpointUrl(targetEndpoint) {}

    /**
     * @brief Polymorphically serializes any payload inheriting from TelemetryPackage and ships it.
     * @param abstractTelemetryPayload Polymorphic payload to serialize and transmit.
     * @return `true` when payload transmission succeeds, otherwise `false`.
     */
    bool sendTelemetryRecord(const TelemetryPackage& abstractTelemetryPayload) {
        if (!communicationTransportDriver.isConnected()) {
            communicationTransportDriver.connect();
            return false;
        }

        JsonDocument serializationDocument;
        abstractTelemetryPayload.serialize(serializationDocument);

        String serializedPayloadOutput;
        serializeJson(serializationDocument, serializedPayloadOutput);

        return communicationTransportDriver.transmit(serviceEndpointUrl, serializedPayloadOutput.c_str());
    }
};

/** @} */ // End of Gateway: Generic Abstraction

#endif // EDGE_GATEWAY_CLIENT_H