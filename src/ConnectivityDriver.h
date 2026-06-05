#ifndef CONNECTIVITY_DRIVER_H
#define CONNECTIVITY_DRIVER_H

class ConnectivityDriver {
public:
    virtual ~ConnectivityDriver() = default;

    virtual void connect() = 0;
    virtual bool isConnected() = 0;
    virtual bool transmit(const char* destinationTarget, const char* encodedPayloadZone) = 0;
};

#endif // CONNECTIVITY_DRIVER_H