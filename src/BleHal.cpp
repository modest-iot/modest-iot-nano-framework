/**
 * @file BleHal.cpp
 * @brief BLE Hardware Abstraction Layer implementation.
 *
 * Implements the BLE HAL with compile-time dispatch between ESP32 Arduino
 * core 3.x and 4.0.0+ APIs using __has_include(<BLE.h>).
 *
 * Uses the PIMPL pattern: each HAL class has an Impl struct that contains
 * the core-specific types. The header only exposes core-agnostic interfaces.
 *
 * @ingroup connectivity_module
 * @copyright (c) 2025-2026 Angel Velasquez
 * @date 2026-11-23
 * @version 1.0
 */

#include "BleHal.h"
#include <cstdlib>
#include <cstring>

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_arduino_version.h>
#endif

#if defined(TEST_MOCK_BLE) || __has_include(<BLE.h>)
// ============================================================================
// ESP32 Arduino core 4.0.0+ implementation (also used for TEST_MOCK_BLE)
// ============================================================================
#if defined(TEST_MOCK_BLE)
#include "BLE.h"
#else
#include <BLE.h>
#endif

// ---------- PIMPL structs ----------

struct BleHalCharacteristicImpl {
    BLECharacteristic characteristic;
    std::function<void(const uint8_t* data, size_t length)> onWriteCb;

    BleHalCharacteristicImpl() = default;
    explicit BleHalCharacteristicImpl(BLECharacteristic chr)
        : characteristic(std::move(chr)) {}
};

struct BleHalServiceImpl {
    BLEService service;

    BleHalServiceImpl() = default;
    explicit BleHalServiceImpl(BLEService svc)
        : service(std::move(svc)) {}
};

struct BleHalServerImpl {
    BLEServer server;
    std::function<void()> onConnectCb;
    std::function<void(uint8_t)> onDisconnectCb;

    BleHalServerImpl() = default;
    explicit BleHalServerImpl(BLEServer srv)
        : server(std::move(srv)) {}
};

struct BleHalAdvertisingImpl {
    BLEAdvertising* advertising = nullptr;

    BleHalAdvertisingImpl() = default;
};

// ---------- BleHalCharacteristic ----------

BleHalCharacteristic::BleHalCharacteristic()
    : pImpl(new BleHalCharacteristicImpl()) {}

BleHalCharacteristic::~BleHalCharacteristic() {
    delete pImpl;
}

BleHalCharacteristic::BleHalCharacteristic(BleHalCharacteristic&& other) noexcept
    : pImpl(other.pImpl) {
    other.pImpl = nullptr;
}

BleHalCharacteristic& BleHalCharacteristic::operator=(BleHalCharacteristic&& other) noexcept {
    if (this != &other) {
        delete pImpl;
        pImpl = other.pImpl;
        other.pImpl = nullptr;
    }
    return *this;
}

void BleHalCharacteristic::onWrite(std::function<void(const uint8_t* data, size_t length)> callback) {
    pImpl->onWriteCb = std::move(callback);
    pImpl->characteristic.onWrite([this](BLECharacteristic &chr, const BLEConnInfo &) {
        if (pImpl->onWriteCb) {
            size_t len = 0;
            const uint8_t* data = chr.getValue(&len);
            pImpl->onWriteCb(data, len);
        }
    });
}

void BleHalCharacteristic::setValue(const uint8_t* data, size_t length) {
    pImpl->characteristic.setValue(data, length);
}

const uint8_t* BleHalCharacteristic::getValue(size_t* len) const {
    return pImpl->characteristic.getValue(len);
}

void BleHalCharacteristic::notify() {
    pImpl->characteristic.notify();
}

// ---------- BleHalService ----------

BleHalService::BleHalService()
    : pImpl(new BleHalServiceImpl()) {}

BleHalService::~BleHalService() {
    delete pImpl;
}

BleHalService::BleHalService(BleHalService&& other) noexcept
    : pImpl(other.pImpl) {
    other.pImpl = nullptr;
}

BleHalService& BleHalService::operator=(BleHalService&& other) noexcept {
    if (this != &other) {
        delete pImpl;
        pImpl = other.pImpl;
        other.pImpl = nullptr;
    }
    return *this;
}

BleHalCharacteristic BleHalService::createCharacteristic(
    const char* uuid, uint32_t props, uint16_t perms) {

    // Map HAL property flags to core 4.0.0 BLEProperty values
    uint32_t ble_props = 0;
    if (props & static_cast<uint32_t>(BleHalProperty::Broadcast))       ble_props |= 0x20;
    if (props & static_cast<uint32_t>(BleHalProperty::Read))            ble_props |= 0x01;
    if (props & static_cast<uint32_t>(BleHalProperty::WriteWithoutRsp)) ble_props |= 0x10;
    if (props & static_cast<uint32_t>(BleHalProperty::Write))           ble_props |= 0x02;
    if (props & static_cast<uint32_t>(BleHalProperty::Notify))          ble_props |= 0x04;
    if (props & static_cast<uint32_t>(BleHalProperty::Indicate))        ble_props |= 0x08;
    if (props & static_cast<uint32_t>(BleHalProperty::SignedWrite))     ble_props |= 0x40;
    if (props & static_cast<uint32_t>(BleHalProperty::ExtendedProps))   ble_props |= 0x80;

    BLECharacteristic chr = pImpl->service.createCharacteristic(
        uuid,
        static_cast<BLEProperty>(ble_props),
        static_cast<BLEPermission>(perms));

    BleHalCharacteristic result;
    delete result.pImpl;
    result.pImpl = new BleHalCharacteristicImpl(std::move(chr));
    return result;
}

// ---------- BleHalServer ----------

BleHalServer::BleHalServer()
    : pImpl(new BleHalServerImpl()) {}

BleHalServer::~BleHalServer() {
    delete pImpl;
}

BleHalServer::BleHalServer(BleHalServer&& other) noexcept
    : pImpl(other.pImpl) {
    other.pImpl = nullptr;
}

BleHalServer& BleHalServer::operator=(BleHalServer&& other) noexcept {
    if (this != &other) {
        delete pImpl;
        pImpl = other.pImpl;
        other.pImpl = nullptr;
    }
    return *this;
}

void BleHalServer::onConnect(std::function<void()> callback) {
    pImpl->onConnectCb = std::move(callback);
    pImpl->server.onConnect([this](BLEServer &, const BLEConnInfo &) {
        if (pImpl->onConnectCb) pImpl->onConnectCb();
    });
}

void BleHalServer::onDisconnect(std::function<void(uint8_t reason)> callback) {
    pImpl->onDisconnectCb = std::move(callback);
    pImpl->server.onDisconnect([this](BLEServer &, const BLEConnInfo &, uint8_t reason) {
        if (pImpl->onDisconnectCb) pImpl->onDisconnectCb(reason);
    });
}

BleHalService BleHalServer::createService(const char* uuid) {
    BLEService svc = pImpl->server.createService(uuid);
    BleHalService result;
    delete result.pImpl;
    result.pImpl = new BleHalServiceImpl(std::move(svc));
    return result;
}

void BleHalServer::start() {
    pImpl->server.start();
}

// ---------- BleHalAdvertising ----------

BleHalAdvertising::BleHalAdvertising()
    : pImpl(new BleHalAdvertisingImpl()) {}

BleHalAdvertising::~BleHalAdvertising() {
    delete pImpl;
}

BleHalAdvertising::BleHalAdvertising(BleHalAdvertising&& other) noexcept
    : pImpl(other.pImpl) {
    other.pImpl = nullptr;
}

BleHalAdvertising& BleHalAdvertising::operator=(BleHalAdvertising&& other) noexcept {
    if (this != &other) {
        delete pImpl;
        pImpl = other.pImpl;
        other.pImpl = nullptr;
    }
    return *this;
}

void BleHalAdvertising::addServiceUUID(const char* uuid) {
    BLEAdvertising adv = BLE.getAdvertising();
    adv.addServiceUUID(uuid);
}

void BleHalAdvertising::setScanResponse(bool enable) {
    BLEAdvertising adv = BLE.getAdvertising();
    adv.setScanResponse(enable);
}

void BleHalAdvertising::start() {
    BLEAdvertising adv = BLE.getAdvertising();
    adv.start();
}

void BleHalAdvertising::stop() {
    BLEAdvertising adv = BLE.getAdvertising();
    adv.stop();
}

// ---------- Global functions ----------

void bleHalBegin(const char* name) {
    BLE.begin(name);
}

BleHalServer bleHalCreateServer() {
    BLEServer srv = BLE.createServer();
    BleHalServer server;
    delete server.pImpl;
    server.pImpl = new BleHalServerImpl(std::move(srv));
    return server;
}

BleHalAdvertising bleHalGetAdvertising() {
    BleHalAdvertising adv;
    delete adv.pImpl;
    adv.pImpl = new BleHalAdvertisingImpl();
    return adv;
}

#elif __has_include(<BLEDevice.h>)
// ============================================================================
// ESP32 Arduino core 3.x implementation
// ============================================================================
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEService.h>
#include <BLECharacteristic.h>
#include <BLEAdvertising.h>
#include <BLEUtils.h>
#if defined(CONFIG_BLUEDROID_ENABLED)
#include <BLE2902.h>
#endif

// ---------- Callback adapters ----------

class HalServerCallbacks : public BLEServerCallbacks {
public:
    std::function<void()> onConnectCb;
    std::function<void(uint8_t)> onDisconnectCb;

    void onConnect(BLEServer* server) override {
        (void)server;
        if (onConnectCb) onConnectCb();
    }

    void onDisconnect(BLEServer* server) override {
        (void)server;
        if (onDisconnectCb) onDisconnectCb(0);
    }
};

class HalCharCallbacks : public BLECharacteristicCallbacks {
public:
    std::function<void(const uint8_t* data, size_t length)> onWriteCb;

    void onWrite(BLECharacteristic* chr) override {
        if (onWriteCb && chr) {
            String value = chr->getValue();
            onWriteCb(reinterpret_cast<const uint8_t*>(value.c_str()), value.length());
        }
    }
};

// ---------- PIMPL structs ----------

struct BleHalCharacteristicImpl {
    BLECharacteristic* characteristic = nullptr;
    HalCharCallbacks* callbacks = nullptr;
    std::function<void(const uint8_t* data, size_t length)> onWriteCb;
    // Buffer for getValue — core 3.x returns std::string, we need a stable pointer
    mutable std::string valueBuffer;

    BleHalCharacteristicImpl() : callbacks(new HalCharCallbacks()) {}
    ~BleHalCharacteristicImpl() { delete callbacks; }
};

struct BleHalServiceImpl {
    BLEService* service = nullptr;
};

struct BleHalServerImpl {
    BLEServer* server = nullptr;
    HalServerCallbacks* callbacks = nullptr;
    std::function<void()> onConnectCb;
    std::function<void(uint8_t)> onDisconnectCb;

    BleHalServerImpl() : callbacks(new HalServerCallbacks()) {}
    ~BleHalServerImpl() { delete callbacks; }
};

struct BleHalAdvertisingImpl {
    BLEAdvertising* advertising = nullptr;
};

// ---------- BleHalCharacteristic ----------

BleHalCharacteristic::BleHalCharacteristic()
    : pImpl(new BleHalCharacteristicImpl()) {}

BleHalCharacteristic::~BleHalCharacteristic() {
    delete pImpl;
}

BleHalCharacteristic::BleHalCharacteristic(BleHalCharacteristic&& other) noexcept
    : pImpl(other.pImpl) {
    other.pImpl = nullptr;
}

BleHalCharacteristic& BleHalCharacteristic::operator=(BleHalCharacteristic&& other) noexcept {
    if (this != &other) {
        delete pImpl;
        pImpl = other.pImpl;
        other.pImpl = nullptr;
    }
    return *this;
}

void BleHalCharacteristic::onWrite(std::function<void(const uint8_t* data, size_t length)> callback) {
    pImpl->onWriteCb = std::move(callback);
    pImpl->callbacks->onWriteCb = pImpl->onWriteCb;
}

void BleHalCharacteristic::setValue(const uint8_t* data, size_t length) {
    if (pImpl->characteristic) {
        pImpl->characteristic->setValue(data, length);
    }
}

const uint8_t* BleHalCharacteristic::getValue(size_t* len) const {
    if (pImpl->characteristic) {
        String value = pImpl->characteristic->getValue();
        pImpl->valueBuffer = std::string(value.c_str(), value.length());
        if (len) *len = pImpl->valueBuffer.length();
        return reinterpret_cast<const uint8_t*>(pImpl->valueBuffer.data());
    }
    if (len) *len = 0;
    return nullptr;
}

void BleHalCharacteristic::notify() {
    if (pImpl->characteristic) {
        pImpl->characteristic->notify();
    }
}

// ---------- BleHalService ----------

BleHalService::BleHalService()
    : pImpl(new BleHalServiceImpl()) {}

BleHalService::~BleHalService() {
    delete pImpl;
}

BleHalService::BleHalService(BleHalService&& other) noexcept
    : pImpl(other.pImpl) {
    other.pImpl = nullptr;
}

BleHalService& BleHalService::operator=(BleHalService&& other) noexcept {
    if (this != &other) {
        delete pImpl;
        pImpl = other.pImpl;
        other.pImpl = nullptr;
    }
    return *this;
}

BleHalCharacteristic BleHalService::createCharacteristic(
    const char* uuid, uint32_t props, uint16_t perms) {

    // Map HAL property flags to core 3.x GATT properties
    uint32_t ble_props = 0;
    if (props & static_cast<uint32_t>(BleHalProperty::Broadcast))       ble_props |= 0x01;
    if (props & static_cast<uint32_t>(BleHalProperty::Read))            ble_props |= 0x02;
    if (props & static_cast<uint32_t>(BleHalProperty::WriteWithoutRsp)) ble_props |= 0x04;
    if (props & static_cast<uint32_t>(BleHalProperty::Write))           ble_props |= 0x08;
    if (props & static_cast<uint32_t>(BleHalProperty::Notify))          ble_props |= 0x10;
    if (props & static_cast<uint32_t>(BleHalProperty::Indicate))        ble_props |= 0x20;
    if (props & static_cast<uint32_t>(BleHalProperty::SignedWrite))     ble_props |= 0x40;
    if (props & static_cast<uint32_t>(BleHalProperty::ExtendedProps))   ble_props |= 0x80;

    BLECharacteristic* chr = pImpl->service->createCharacteristic(
        uuid, ble_props);

    // Set permissions
    uint32_t access_perms = 0;
    if (perms & static_cast<uint16_t>(BleHalPermission::ReadOpen)) {
        access_perms |= ESP_GATT_PERM_READ;
    }
    if (perms & static_cast<uint16_t>(BleHalPermission::WriteOpen)) {
        access_perms |= ESP_GATT_PERM_WRITE;
    }
    if (access_perms) {
        chr->setAccessPermissions(access_perms);
    }

    // Add Client Characteristic Configuration descriptor for Notify/Indicate
    // Note: NimBLE auto-adds 2902 descriptors when Notify/Indicate properties are set,
    // so manual addition is only needed for Bluedroid. Guard with type check to avoid
    // compilation errors when BLE2902 is unavailable.
#if defined(CONFIG_BLUEDROID_ENABLED)
    if (props & (static_cast<uint32_t>(BleHalProperty::Notify) |
                 static_cast<uint32_t>(BleHalProperty::Indicate))) {
        BLE2902* desc = new BLE2902();
        desc->setNotifications(true);
        chr->addDescriptor(desc);
    }
#endif

    BleHalCharacteristic result;
    result.pImpl->characteristic = chr;
    return result;
}

// ---------- BleHalServer ----------

BleHalServer::BleHalServer()
    : pImpl(new BleHalServerImpl()) {}

BleHalServer::~BleHalServer() {
    delete pImpl;
}

BleHalServer::BleHalServer(BleHalServer&& other) noexcept
    : pImpl(other.pImpl) {
    other.pImpl = nullptr;
}

BleHalServer& BleHalServer::operator=(BleHalServer&& other) noexcept {
    if (this != &other) {
        delete pImpl;
        pImpl = other.pImpl;
        other.pImpl = nullptr;
    }
    return *this;
}

void BleHalServer::onConnect(std::function<void()> callback) {
    pImpl->onConnectCb = std::move(callback);
    pImpl->callbacks->onConnectCb = pImpl->onConnectCb;
    if (pImpl->server) {
        pImpl->server->setCallbacks(pImpl->callbacks);
    }
}

void BleHalServer::onDisconnect(std::function<void(uint8_t reason)> callback) {
    pImpl->onDisconnectCb = std::move(callback);
    pImpl->callbacks->onDisconnectCb = pImpl->onDisconnectCb;
    if (pImpl->server) {
        pImpl->server->setCallbacks(pImpl->callbacks);
    }
}

BleHalService BleHalServer::createService(const char* uuid) {
    BleHalService result;
    if (pImpl->server) {
        result.pImpl->service = pImpl->server->createService(BLEUUID(uuid));
    }
    return result;
}

void BleHalServer::start() {
    if (pImpl->server) {
        pImpl->server->start();
    }
}

// ---------- BleHalAdvertising ----------

BleHalAdvertising::BleHalAdvertising()
    : pImpl(new BleHalAdvertisingImpl()) {}

BleHalAdvertising::~BleHalAdvertising() {
    delete pImpl;
}

BleHalAdvertising::BleHalAdvertising(BleHalAdvertising&& other) noexcept
    : pImpl(other.pImpl) {
    other.pImpl = nullptr;
}

BleHalAdvertising& BleHalAdvertising::operator=(BleHalAdvertising&& other) noexcept {
    if (this != &other) {
        delete pImpl;
        pImpl = other.pImpl;
        other.pImpl = nullptr;
    }
    return *this;
}

void BleHalAdvertising::addServiceUUID(const char* uuid) {
    BLEAdvertising* adv = BLEDevice::getAdvertising();
    if (adv) adv->addServiceUUID(BLEUUID(uuid));
}

void BleHalAdvertising::setScanResponse(bool enable) {
    BLEAdvertising* adv = BLEDevice::getAdvertising();
    if (adv) adv->setScanResponse(enable);
}

void BleHalAdvertising::start() {
    BLEAdvertising* adv = BLEDevice::getAdvertising();
    if (adv) adv->start();
}

void BleHalAdvertising::stop() {
    BLEAdvertising* adv = BLEDevice::getAdvertising();
    if (adv) adv->stop();
}

// ---------- Global functions ----------

void bleHalBegin(const char* name) {
    BLEDevice::init(name);
}

BleHalServer bleHalCreateServer() {
    BleHalServer server;
    server.pImpl->server = BLEDevice::createServer();
    server.pImpl->server->setCallbacks(server.pImpl->callbacks);
    return server;
}

BleHalAdvertising bleHalGetAdvertising() {
    BleHalAdvertising adv;
    adv.pImpl->advertising = BLEDevice::getAdvertising();
    return adv;
}

#elif defined(ARDUINO_ARCH_ESP32)
// ============================================================================
// ESP32 Arduino core fallback (Arduino CLI __has_include workaround)
// ============================================================================
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(4, 0, 0)
#include <BLE.h>
#else
#include <BLEDevice.h>
#endif
#else
#error "BLE HAL: No supported ESP32 BLE API found. Requires ESP32 Arduino core 3.x or 4.0.0+"
#endif
