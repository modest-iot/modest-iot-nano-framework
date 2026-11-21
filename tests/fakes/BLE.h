#ifndef FAKE_BLE_H
#define FAKE_BLE_H

#include <cstdint>
#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

// ESP32 4.0.0+ unified BLE API fakes for host tests.
// Mirrors the single BLE.h header structure from the ESP32 4.0.0 Arduino core.
// All BLE types (BLEServer, BLEService, BLECharacteristic, BLEAdvertising, etc.)
// and the BLE namespace are defined here, matching the production BLE.h layout.

// ---------------------------------------------------------------------------
// BLEProperty enum (ESP32 4.0.0+)
// ---------------------------------------------------------------------------
enum class BLEProperty : uint32_t {
    Read = 0x01,
    Write = 0x02,
    Notify = 0x04,
    Indicate = 0x08,
    WriteNR = 0x10,
    Broadcast = 0x20,
    SignedWrite = 0x40,
    ExtendedProps = 0x80
};

// Allow bitwise OR on BLEProperty enum
inline BLEProperty operator|(BLEProperty a, BLEProperty b) {
    return static_cast<BLEProperty>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

// ---------------------------------------------------------------------------
// BLEPermission enum (ESP32 4.0.0+)
// ---------------------------------------------------------------------------
enum class BLEPermission : uint16_t {
    None = 0x0000,
    ReadOpen = 0x0001,
    WriteOpen = 0x0010,
    ReadWriteOpen = 0x0011
};

// Allow bitwise OR on BLEPermission enum
inline BLEPermission operator|(BLEPermission a, BLEPermission b) {
    return static_cast<BLEPermission>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}

// ---------------------------------------------------------------------------
// BLEConnInfo type (ESP32 4.0.0+) - simplified for host tests
// ---------------------------------------------------------------------------
using BLEConnInfo = std::string;

// ---------------------------------------------------------------------------
// BTStatus return type (ESP32 4.0.0+)
// ---------------------------------------------------------------------------
class BTStatus {
public:
    explicit BTStatus(bool ok = true) : ok_(ok) {}
    explicit BTStatus(int code) : ok_(code == 0) {}
    [[nodiscard]] explicit operator bool() const { return ok_; }
    [[nodiscard]] bool ok() const { return ok_; }
private:
    bool ok_;
};

// ---------------------------------------------------------------------------
// BLECharacteristic (ESP32 4.0.0+)
// ---------------------------------------------------------------------------
class BLECharacteristic {
public:
    std::string uuid;
    uint32_t properties;
    // Use shared pointers so copies share the same state
    static inline std::shared_ptr<std::string> value = std::make_shared<std::string>();
    static inline std::shared_ptr<size_t> notifyCount = std::make_shared<size_t>(0);

    static inline std::shared_ptr<std::function<void(BLECharacteristic &, const BLEConnInfo &)>> onWriteCallback = std::make_shared<std::function<void(BLECharacteristic &, const BLEConnInfo &)>>();

    BLECharacteristic() = default;
    BLECharacteristic(std::string chUuid, const uint32_t props)
        : uuid(std::move(chUuid)), properties(props) {}

    ~BLECharacteristic() = default;

    void onWrite(std::function<void(BLECharacteristic &, const BLEConnInfo &)> callback) {
        *onWriteCallback = std::move(callback);
    }

    void setValue(const uint8_t* data, const size_t size) {
        *value = std::string(reinterpret_cast<const char*>(data), size);
    }

    void setValue(uint8_t* data, const size_t size) {
        *value = std::string(reinterpret_cast<char*>(data), size);
    }

    void setValue(const std::string& val) {
        *value = val;
    }

    [[nodiscard]] std::string getStringValue() const {
        return *value;
    }

    const uint8_t* getValue(size_t* length = nullptr) const {
        if (length != nullptr) {
            *length = value->size();
        }
        return reinterpret_cast<const uint8_t*>(value->data());
    }

    const uint8_t* getData() const {
        return reinterpret_cast<const uint8_t*>(value->data());
    }

    [[nodiscard]] size_t getLength() const {
        return value->size();
    }

    void notify() {
        (*notifyCount)++;
    }
};

// ---------------------------------------------------------------------------
// BLEService (ESP32 4.0.0+)
// ---------------------------------------------------------------------------
class BLEService {
public:
    std::string uuid;
    // Use shared pointer so copies share the same characteristics
    static inline std::shared_ptr<std::vector<BLECharacteristic>> characteristics = std::make_shared<std::vector<BLECharacteristic>>();
    static inline std::shared_ptr<bool> started = std::make_shared<bool>(false);

    BLEService() = default;
    explicit BLEService(std::string serviceUuid) : uuid(std::move(serviceUuid)) {}

    BLECharacteristic createCharacteristic(const std::string& chUuid, BLEProperty props, BLEPermission /*permissions*/) {
        if (characteristics->capacity() == characteristics->size()) {
            characteristics->reserve(characteristics->capacity() + 2);
        }
        characteristics->emplace_back(chUuid, static_cast<uint32_t>(props));
        return characteristics->back();
    }

    void start() {
        *started = true;
    }
};

// ---------------------------------------------------------------------------
// BLEServer (ESP32 4.0.0+)
// ---------------------------------------------------------------------------
class BLEServer {
public:
    // Use shared pointers so copies share the same state
    static inline std::shared_ptr<std::vector<BLEService>> services = std::make_shared<std::vector<BLEService>>();
    static inline std::shared_ptr<std::function<void(BLEServer &, const BLEConnInfo &)>> onConnectCallback = std::make_shared<std::function<void(BLEServer &, const BLEConnInfo &)>>();
    static inline std::shared_ptr<std::function<void(BLEServer &, const BLEConnInfo &, uint8_t)>> onDisconnectCallback = std::make_shared<std::function<void(BLEServer &, const BLEConnInfo &, uint8_t)>>();

    BLEServer() = default;

    void onConnect(std::function<void(BLEServer &, const BLEConnInfo &)> callback) {
        *onConnectCallback = std::move(callback);
    }

    void onDisconnect(std::function<void(BLEServer &, const BLEConnInfo &, uint8_t)> callback) {
        *onDisconnectCallback = std::move(callback);
    }

    BLEService createService(const std::string& serviceUuid) {
        if (services->capacity() == services->size()) {
            services->reserve(services->capacity() + 2);
        }
        services->emplace_back(std::move(serviceUuid));
        return services->back();
    }

    void start() {
        for (auto& svc : *services) {
            svc.start();
        }
    }
};

// ---------------------------------------------------------------------------
// BLEAdvertising (ESP32 4.0.0+)
// ---------------------------------------------------------------------------
class BLEAdvertising {
public:
    // Use shared pointers so copies share the same state
    static inline std::shared_ptr<std::vector<std::string>> serviceUuids = std::make_shared<std::vector<std::string>>();
    static inline std::shared_ptr<bool> scanResponse = std::make_shared<bool>(false);
    static inline std::shared_ptr<bool> running = std::make_shared<bool>(false);

    void addServiceUUID(const std::string& uuid) {
        serviceUuids->push_back(uuid);
    }

    void setScanResponse(const bool set) {
        *scanResponse = set;
    }

    void start() {
        *running = true;
    }

    void stop() {
        *running = false;
    }

    [[nodiscard]] bool isRunning() const {
        return *running;
    }
};

// ---------------------------------------------------------------------------
// BLEClass (ESP32 4.0.0+) - mirrors the global BLE singleton object
// ---------------------------------------------------------------------------
class BLEClass {
public:
    std::string deviceName;
    BLEServer serverInstance;
    BLEAdvertising advertisingInstance;

    BTStatus begin(const std::string& name) {
        deviceName = name;
        return BTStatus(true);
    }

    BTStatus end(bool release = false) {
        (void)release;
        deviceName.clear();
        return BTStatus(true);
    }

    BLEServer createServer() {
        return serverInstance;
    }

    BLEAdvertising& getAdvertising() {
        return advertisingInstance;
    }

    const std::string& getInitName() const {
        return deviceName;
    }
};

// Global BLE singleton object (ESP32 4.0.0+)
inline BLEClass BLE;

// Test utility - resets all singleton state between tests
inline void BLE_reset() {
    BLE.serverInstance = BLEServer();
    BLE.advertisingInstance = BLEAdvertising();
    BLE.deviceName.clear();
    *BLEAdvertising::running = false;
    *BLEAdvertising::serviceUuids = std::vector<std::string>();
    *BLEAdvertising::scanResponse = false;
    *BLEServer::services = std::vector<BLEService>();
    *BLEService::characteristics = std::vector<BLECharacteristic>();
    *BLEService::started = false;
    *BLECharacteristic::value = std::string();
    *BLECharacteristic::notifyCount = 0;
}

#endif // FAKE_BLE_DEVICE_H
