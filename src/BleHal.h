#ifndef BLE_HAL_H
#define BLE_HAL_H

/**
 * @file BleHal.h
 * @brief BLE Hardware Abstraction Layer for ESP32 Arduino core 3.x and 4.0.0+.
 *
 * This HAL normalizes the API differences between ESP32 Arduino core versions:
 * - Core 4.0.0+ uses a unified BLE.h with value-based handles (shared_ptr wrapped),
 *   lambda callbacks, and a global BLE singleton.
 * - Core 3.x uses separate headers (BLEDevice.h, BLEServer.h, etc.), raw-pointer
 *   ownership, callback classes, and BLEDevice static API.
 *
 * The HAL dispatches at compile time via __has_include, ensuring zero runtime overhead.
 * The classes use the PIMPL pattern so the header contains no core-specific types.
 *
 * @ingroup connectivity_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-23
 * @version 1.0
 */

#include <cstddef>
#include <cstdint>
#include <functional>

/**
 * @brief BLE characteristic property flags (core-agnostic).
 *
 * These values match the GATT property bits and are compatible with both
 * core 3.x and 4.0.0+ APIs.
 */
enum class BleHalProperty : uint32_t {
    Broadcast       = 0x01,
    Read            = 0x02,
    WriteWithoutRsp = 0x04,
    Write           = 0x08,
    Notify          = 0x10,
    Indicate        = 0x20,
    SignedWrite     = 0x40,
    ExtendedProps   = 0x80
};

/**
 * @brief Bitwise OR operator for BleHalProperty.
 */
inline BleHalProperty operator|(BleHalProperty a, BleHalProperty b) {
    return static_cast<BleHalProperty>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/**
 * @brief BLE characteristic permission flags (core-agnostic).
 */
enum class BleHalPermission : uint16_t {
    None          = 0x0000,
    ReadOpen      = 0x0001,
    WriteOpen     = 0x0010,
    ReadWriteOpen = 0x0011
};

/**
 * @brief Bitwise OR operator for BleHalPermission.
 */
inline BleHalPermission operator|(BleHalPermission a, BleHalPermission b) {
    return static_cast<BleHalPermission>(
        static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}

// Forward declarations for PIMPL
struct BleHalCharacteristicImpl;
struct BleHalServiceImpl;
struct BleHalServerImpl;
struct BleHalAdvertisingImpl;

/**
 * @brief HAL wrapper for a BLE Characteristic.
 *
 * Provides a unified interface for characteristic operations regardless
 * of the underlying ESP32 core version.
 */
class BleHalCharacteristic {
public:
    BleHalCharacteristic();
    ~BleHalCharacteristic();

    // Disable copy — the underlying BLE object has specific ownership semantics
    BleHalCharacteristic(const BleHalCharacteristic&) = delete;
    BleHalCharacteristic& operator=(const BleHalCharacteristic&) = delete;

    // Allow move
    BleHalCharacteristic(BleHalCharacteristic&& other) noexcept;
    BleHalCharacteristic& operator=(BleHalCharacteristic&& other) noexcept;

    /**
     * @brief Registers a callback invoked when a central writes to this characteristic.
     * @param callback Function receiving the written data and its length.
     */
    void onWrite(std::function<void(const uint8_t* data, size_t length)> callback);

    /**
     * @brief Sets the characteristic value and prepares it for notification.
     * @param data Pointer to the data buffer.
     * @param length Number of bytes to set.
     */
    void setValue(const uint8_t* data, size_t length);

    /**
     * @brief Retrieves the current characteristic value.
     * @param len Optional pointer to receive the value length.
     * @return Pointer to the value data.
     */
    const uint8_t* getValue(size_t* len = nullptr) const;

    /**
     * @brief Sends a notification to the connected central with the current value.
     */
    void notify();

private:
    BleHalCharacteristicImpl* pImpl;
    friend class BleHalService;
};

/**
 * @brief HAL wrapper for a BLE Service.
 *
 * Provides a unified interface for service operations regardless
 * of the underlying ESP32 core version.
 */
class BleHalService {
public:
    BleHalService();
    ~BleHalService();

    // Disable copy
    BleHalService(const BleHalService&) = delete;
    BleHalService& operator=(const BleHalService&) = delete;

    // Allow move
    BleHalService(BleHalService&& other) noexcept;
    BleHalService& operator=(BleHalService&& other) noexcept;

    /**
     * @brief Creates a new characteristic within this service.
     * @param uuid 128-bit UUID string for the characteristic.
     * @param props Bitwise OR of BleHalProperty flags.
     * @param perms Bitwise OR of BleHalPermission flags.
     * @return A BleHalCharacteristic handle for the new characteristic.
     */
    BleHalCharacteristic createCharacteristic(
        const char* uuid, uint32_t props, uint16_t perms);

private:
    BleHalServiceImpl* pImpl;
    friend class BleHalServer;
};

/**
 * @brief HAL wrapper for a BLE Server (GATT server).
 *
 * Provides a unified interface for server operations regardless
 * of the underlying ESP32 core version.
 */
class BleHalServer {
public:
    BleHalServer();
    ~BleHalServer();

    // Disable copy
    BleHalServer(const BleHalServer&) = delete;
    BleHalServer& operator=(const BleHalServer&) = delete;

    // Allow move
    BleHalServer(BleHalServer&& other) noexcept;
    BleHalServer& operator=(BleHalServer&& other) noexcept;

    /**
     * @brief Registers a callback invoked when a central connects.
     * @param callback Function to call on connection.
     */
    void onConnect(std::function<void()> callback);

    /**
     * @brief Registers a callback invoked when a central disconnects.
     * @param callback Function receiving the disconnect reason code.
     */
    void onDisconnect(std::function<void(uint8_t reason)> callback);

    /**
     * @brief Creates a new GATT service.
     * @param uuid 128-bit UUID string for the service.
     * @return A BleHalService handle for the new service.
     */
    BleHalService createService(const char* uuid);

    /**
     * @brief Starts the GATT server, making services available to centrals.
     */
    void start();

private:
    BleHalServerImpl* pImpl;
    friend BleHalServer bleHalCreateServer();
};

/**
 * @brief HAL wrapper for BLE Advertising.
 *
 * Provides a unified interface for advertising operations regardless
 * of the underlying ESP32 core version.
 */
class BleHalAdvertising {
public:
    BleHalAdvertising();
    ~BleHalAdvertising();

    // Disable copy
    BleHalAdvertising(const BleHalAdvertising&) = delete;
    BleHalAdvertising& operator=(const BleHalAdvertising&) = delete;

    // Allow move
    BleHalAdvertising(BleHalAdvertising&& other) noexcept;
    BleHalAdvertising& operator=(BleHalAdvertising&& other) noexcept;

    /**
     * @brief Adds a service UUID to the advertising data.
     * @param uuid 128-bit UUID string to advertise.
     */
    void addServiceUUID(const char* uuid);

    /**
     * @brief Enables or disables scan response data.
     * @param enable True to include service UUIDs in scan response.
     */
    void setScanResponse(bool enable);

    /**
     * @brief Starts broadcasting advertising packets.
     */
    void start();

    /**
     * @brief Stops broadcasting advertising packets.
     */
    void stop();

private:
    BleHalAdvertisingImpl* pImpl;
    friend BleHalAdvertising bleHalGetAdvertising();
};

/**
 * @brief Initializes the BLE stack with a local device name.
 * @param name The name broadcast during advertising.
 */
void bleHalBegin(const char* name);

/**
 * @brief Creates a new BLE GATT server instance.
 * @return A BleHalServer handle.
 */
BleHalServer bleHalCreateServer();

/**
 * @brief Retrieves the global BLE advertising instance.
 * @return A BleHalAdvertising handle.
 */
BleHalAdvertising bleHalGetAdvertising();

#endif // BLE_HAL_H
