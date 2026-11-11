#ifndef BLE_CONNECTIVITY_DRIVER_H
#define BLE_CONNECTIVITY_DRIVER_H

/**
 * @file BleConnectivityDriver.h
 * @brief Declares a Bluetooth Low Energy (BLE) Nordic UART Service (NUS) implementation of `ConnectivityDriver`.
 * @ingroup connectivity_module
 * @date 2026-06-06
 * @version 1.0
 */

#include "ConnectivityDriver.h"
#include <cstddef>
#include <cstdint>
#include <memory>

#if defined(CONFIG_BT_ENABLED) || defined(ARDUINO_ARCH_ESP32) || defined(TEST_MOCK_BLE)

// Forward declarations avoid leaking heavy BLE headers into application code
class BLEServer;
class BLEService;
class BLECharacteristic;
class BLEServerCallbacks;
class BLECharacteristicCallbacks;
class BLEAdvertising;

/**
 * @brief Bluetooth Low Energy (BLE) connectivity adapter implementing the Nordic UART Service (NUS).
 *
 * This driver provisions a standard 128-bit Nordic UART Service (NUS) GATT server, enabling
 * bidirectional wireless telemetry streaming between ESP32 devices and smartphones, web dashboards
 * (Web Bluetooth API), or edge gateways across all modern ESP32 SoC variants (ESP32, S3, C3, C6, H2).
 *
 * @ingroup connectivity_module
 */
class BleConnectivityDriver : public ConnectivityDriver {
public:
    /// @brief Callback signature for handling inbound data arriving on the RX characteristic.
    using DataReceivedCallback = void (*)(const char* data, size_t length);

    // Nordic Semiconductor UART Service (NUS) 128-bit standard UUID constants
    static constexpr auto NUS_SERVICE_UUID           = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
    static constexpr auto NUS_RX_CHARACTERISTIC_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
    static constexpr auto NUS_TX_CHARACTERISTIC_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

private:
    const char* localDeviceName;
    volatile bool deviceConnected;
    bool advertisingActive;
    bool initialized;

    BLEServer* bleServer;
    BLEService* bleService;
    BLECharacteristic* txCharacteristic;
    BLECharacteristic* rxCharacteristic;

    DataReceivedCallback onDataReceivedHandler;

    std::unique_ptr<BLEServerCallbacks> serverCallbacksHandler;
    std::unique_ptr<BLECharacteristicCallbacks> rxCallbacksHandler;

public:
    /**
     * @brief Constructs a BLE Nordic UART connectivity adapter.
     * @param deviceName Local Bluetooth advertising broadcast identifier.
     */
    explicit BleConnectivityDriver(const char* deviceName = "Modest-IoT-Device");

    ~BleConnectivityDriver() override;

    /**
     * @brief Initializes the BLE stack, configures the NUS GATT server, and starts advertising.
     */
    void connect() override;

    /**
     * @brief Inquires whether a central peer (phone/gateway) is actively connected.
     * @return `true` if a BLE central peer is connected, otherwise `false`.
     */
    [[nodiscard]] bool isConnected() override;

    /**
     * @brief Intention-Revealing Query: Verifies whether the BLE link is connected and ready to transmit.
     * @return `true` if the adapter can transmit outbound payloads.
     */
    [[nodiscard]] bool canTransmit() override;

    /**
     * @brief Transmits data over the BLE TX characteristic via GATT notification.
     * @param target Target identifier (unused in point-to-point BLE connection, can be nullptr).
     * @param data Payload data string to transmit.
     * @return `true` if successfully queued for notification, otherwise `false`.
     */
    bool transmit(const char* target, const char* data) override;

    /**
     * @brief Transmits a raw byte buffer over the BLE TX characteristic.
     * @param payload Pointer to raw data buffer.
     * @param length Number of bytes to transmit.
     * @return `true` if successfully sent, otherwise `false`.
     */
    bool transmitBytes(const uint8_t* payload, size_t length);

    /**
     * @brief Returns the local Bluetooth device name.
     * @return Device name string.
     */
    [[nodiscard]] const char* getDeviceName() const;

    /**
     * @brief Reports whether BLE advertising is currently active.
     * @return `true` if advertising, otherwise `false`.
     */
    [[nodiscard]] bool isAdvertising() const;

    /**
     * @brief Manually stops BLE advertising.
     */
    void stopAdvertising();

    /**
     * @brief Manually starts or restarts BLE advertising.
     */
    void startAdvertising();

    /**
     * @brief Registers a callback to be invoked when data is written to the RX characteristic by a central peer.
     * @param callback Function pointer to the received data handler.
     */
    void setReceivedDataCallback(DataReceivedCallback callback);

    /**
     * @brief Internal hook invoked when a central peer connects.
     */
    void handlePeerConnected();

    /**
     * @brief Internal hook invoked when a central peer disconnects.
     */
    void handlePeerDisconnected();

    /**
     * @brief Disconnects the currently connected BLE peer and stops advertising.
     * @note Useful before entering deep sleep to conserve power.
     */
    void disconnect();

    /**
     * @brief Returns the signal strength in dBm of the connected peer.
     * @return Always returns 0, as the ESP32 BLE server API does not expose RSSI.
     * @note RSSI is only available during scanning (BLEAdvertisedDevice::getRSSI())
     * or from a BLE client perspective (BLEClient::getRssi()).
     */
    [[nodiscard]] int32_t getSignalStrengthInDbm() const;

    /**
     * @brief Internal hook invoked when data is received on the RX characteristic.
     * @param data Received byte buffer.
     * @param length Number of received bytes.
     */
    void handleDataReceived(const uint8_t* data, size_t length) const;
};

#endif // CONFIG_BT_ENABLED || ARDUINO_ARCH_ESP32 || TEST_MOCK_BLE

#endif // BLE_CONNECTIVITY_DRIVER_H
