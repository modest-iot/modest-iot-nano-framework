#ifndef I2C_BUS_H
#define I2C_BUS_H

/**
 * @file I2cBus.h
 * @brief Declares the platform-neutral I2C bus configuration, result, and transaction contract.
 * @ingroup hardware_io_module
 */

#include <cstddef>
#include <cstdint>

/**
 * @brief Configuration used to start an I2C bus.
 *
 * Set both pins to -1 to request the platform defaults; otherwise use non-negative,
 * distinct SDA and SCL pins. The timeout applies to an individual
 * backend transfer; it does not define a bound for waiting to acquire a bus lock.
 * @ingroup hardware_io_module
 */
struct I2cBusConfiguration {
    int sdaPin; ///< SDA GPIO; set both pin fields to -1 to use platform defaults.
    int sclPin; ///< SCL GPIO; set both pin fields to -1 to use platform defaults.
    std::uint32_t frequencyInHertz; ///< Bus clock frequency; must be greater than zero.
    std::uint16_t timeoutInMilliseconds = 50; ///< Per-transfer timeout in milliseconds; defaults to 50 and must be non-zero.
};

/**
 * @brief Status reported by an I2C bus operation.
 * @ingroup hardware_io_module
 */
enum class I2cStatus : std::uint8_t {
    Ok, ///< The requested transfer completed successfully.
    NotStarted, ///< The bus has not been started or is not ready.
    InvalidArgument, ///< A configuration, address, pointer, or transfer length is invalid.
    AddressNack, ///< The addressed peripheral did not acknowledge its address.
    DataNack, ///< The peripheral did not acknowledge transmitted data.
    Timeout, ///< The backend reported a timeout; not all backends distinguish read timeouts.
    BusError, ///< The backend reported an initialization or other bus-level error.
    ShortRead ///< Fewer receive bytes were obtained than requested.
};

/**
 * @brief Status and byte counts for one I2C operation.
 *
 * For @ref I2cBus::write, counts describe the write payload. For
 * @ref I2cBus::writeRead, counts describe received bytes only; transmitted
 * register/address bytes are not included. A short read may therefore have a
 * non-zero @c transferred count while its status is @ref I2cStatus::ShortRead.
 * @ingroup hardware_io_module
 */
struct I2cResult {
    I2cStatus status; ///< Status of the operation.
    std::size_t transferred; ///< Payload bytes completed or received, as defined above.
    std::size_t expectedTransferred; ///< Requested write payload or receive byte count.

    /**
     * @brief Reports whether the backend status is successful.
     * @return @c true only when @c status is @ref I2cStatus::Ok.
     */
    bool ok() const { return status == I2cStatus::Ok; }

    /**
     * @brief Reports a failed status or incomplete byte count.
     * @return @c true when the status is not successful or counts differ.
     */
    bool isInvalidOrIncomplete() const {
        return !ok() || transferred != expectedTransferred;
    }
};

/**
 * @brief Platform-neutral contract for starting and using an I2C bus.
 *
 * Transactions execute synchronously in the caller's context. A combined
 * write/read keeps its transmitted prefix and receive phase within one bus
 * operation; implementations should not interleave another transaction between
 * those phases.
 * @ingroup hardware_io_module
 */
class I2cBus {
public:
    virtual ~I2cBus() = default;

    /**
     * @brief Starts the bus with the requested configuration.
     * @param configuration Pin, frequency, and per-transfer timeout settings.
     * @return @ref I2cStatus::Ok on success, otherwise the startup failure status.
     * @runs_on{caller}
     */
    virtual I2cStatus start(const I2cBusConfiguration& configuration) = 0;

    /**
     * @brief Reports whether the bus is ready for transactions.
     * @return @c true if the bus has started successfully.
     * @runs_on{caller}
     */
    virtual bool isReady() const = 0;

    /**
     * @brief Writes a payload to a 7-bit I2C address.
     * @param address Peripheral address in the range 0x00–0x7F.
     * @param bytes Non-null payload pointer.
     * @param length Number of payload bytes; must be greater than zero.
     * @return Status and completed-versus-expected payload byte counts.
     * @runs_on{caller}
     */
    virtual I2cResult write(std::uint8_t address, const std::uint8_t* bytes,
                            std::size_t length) = 0;

    /**
     * @brief Transmits an optional prefix and reads bytes as one combined transaction.
     * @param address Peripheral address in the range 0x00–0x7F.
     * @param txBytes Prefix to transmit before reading; may be null only when @p txLength is zero.
     * @param txLength Number of prefix bytes to transmit.
     * @param rxBytes Non-null buffer receiving the requested bytes.
     * @param rxLength Number of receive bytes; must be greater than zero.
     * @return Status and received-versus-expected byte counts; prefix bytes are not counted.
     * @runs_on{caller}
     */
    virtual I2cResult writeRead(std::uint8_t address,
                                const std::uint8_t* txBytes, std::size_t txLength,
                                std::uint8_t* rxBytes, std::size_t rxLength) = 0;
};

#endif // I2C_BUS_H
