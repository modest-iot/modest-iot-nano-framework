#ifndef CONNECTIVITY_DRIVER_H
#define CONNECTIVITY_DRIVER_H

class ConnectivityDriver {
public:
    virtual ~ConnectivityDriver() = default;

    /**
     * @brief Establishes connections to the physical network environment.
     */
    virtual void connect() = 0;

    /**
     * @brief Evaluates whether the underlying transceiver link is fully authenticated.
     */
    virtual bool isConnected() = 0;

    /**
     * @brief INTENTION-REVEALING QUERY: Evaluates whether the underlying radio
     * link layer is clear, healthy, and ready to stream payload byte chunks.
     */
    virtual bool canTransmit() = 0;
};

#endif // CONNECTIVITY_DRIVER_H