#ifndef EDGE_GATEWAY_CLIENT_H
#define EDGE_GATEWAY_CLIENT_H

#include "ConnectivityDriver.h"
#include "TelemetryPackage.h"

class EdgeGatewayClient {
private:
    ConnectivityDriver& communicationTransportDriver;
    const char* serviceEndpointUrl;

public:
    EdgeGatewayClient(ConnectivityDriver& transportDriver, const char* targetEndpoint)
        : communicationTransportDriver(transportDriver), serviceEndpointUrl(targetEndpoint) {}

    /**
     * @brief Polymorphically serializes any payload inheriting from TelemetryPackage and ships it.
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

#endif // EDGE_GATEWAY_CLIENT_H