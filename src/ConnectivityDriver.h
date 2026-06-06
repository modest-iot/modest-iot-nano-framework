#ifndef CONNECTIVITY_DRIVER_H
#define CONNECTIVITY_DRIVER_H

/**
 * @file ConnectivityDriver.h
 * @brief Defines the abstraction for network transport provisioning and readiness checks.
 * @ingroup connectivity_module
 */

/**
 * @name Connectivity: Interface & Contracts
 * @ingroup connectivity_module
 * @{
 */

/**
 * @brief Strategy interface for physical/network link management.
 * @ingroup connectivity_module
 */
class ConnectivityDriver {
public:
    /** @brief Virtual destructor for safe polymorphic deletion. */
    virtual ~ConnectivityDriver() = default;

    /**
     * @brief Establishes connections to the physical network environment.
     */
    virtual void connect() = 0;

    /**
     * @brief Evaluates whether the underlying transceiver link is fully authenticated.
     * @return `true` if the transport is connected, otherwise `false`.
     */
    virtual bool isConnected() = 0;

    /**
     * @brief INTENTION-REVEALING QUERY: Evaluates whether the underlying radio
     * link layer is clear, healthy, and ready to stream payload byte chunks.
     * @return `true` if the link is able to transmit payloads, otherwise `false`.
     */
    virtual bool canTransmit() = 0;
};

/** @} */ // End of Connectivity: Interface & Contracts

#endif // CONNECTIVITY_DRIVER_H