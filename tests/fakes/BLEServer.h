#ifndef FAKE_BLE_SERVER_H
#define FAKE_BLE_SERVER_H

#include <time.h>
#include <cstdint>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class BLEServer;
class BLECharacteristic;

class BLEServerCallbacks {
public:
    virtual ~BLEServerCallbacks() = default;
    virtual void onConnect(BLEServer* pServer) { (void)pServer; }
    virtual void onDisconnect(BLEServer* pServer) { (void)pServer; }
};

class BLECharacteristicCallbacks {
public:
    virtual ~BLECharacteristicCallbacks() = default;
    virtual void onWrite(BLECharacteristic* pCharacteristic) { (void)pCharacteristic; }
    virtual void onRead(BLECharacteristic* pCharacteristic) { (void)pCharacteristic; }
};

class BLECharacteristic {
public:
    static constexpr uint32_t PROPERTY_READ     = 0x01;
    static constexpr uint32_t PROPERTY_WRITE    = 0x02;
    static constexpr uint32_t PROPERTY_NOTIFY   = 0x04;
    static constexpr uint32_t PROPERTY_WRITE_NR = 0x08;

    std::string uuid;
    uint32_t properties;
    std::string value;
    BLECharacteristicCallbacks* callbacks = nullptr;
    size_t notifyCount = 0;

    BLECharacteristic(std::string uuid, const uint32_t props)
        : uuid(std::move(uuid)), properties(props) {}

    ~BLECharacteristic() = default;

    void setCallbacks(BLECharacteristicCallbacks* cb) {
        callbacks = cb;
    }

    void setValue(const uint8_t* data, const size_t size) {
        value = std::string(reinterpret_cast<const char*>(data), size);
    }

    void setValue(uint8_t* data, const size_t size) {
        value = std::string(reinterpret_cast<char*>(data), size);
    }

    void setValue(const std::string& val) {
        value = val;
    }

    [[nodiscard]] std::string getValue() const {
        return value;
    }

    uint8_t* getData() {
        return reinterpret_cast<uint8_t*>(value.data());
    }

    [[nodiscard]] size_t getLength() const {
        return value.size();
    }

    void notify() {
        notifyCount++;
    }
};

class BLEService {
public:
    std::string uuid;
    std::vector<BLECharacteristic*> characteristics;
    bool started = false;

    explicit BLEService(std::string uuid) : uuid(std::move(uuid)) {}

    ~BLEService() {
        for (const auto* ch : characteristics) {
            delete ch;
        }
        characteristics.clear();
    }

    BLECharacteristic* createCharacteristic(const std::string& chUuid, const uint32_t props) {
        auto* ch = new BLECharacteristic(chUuid, props);
        characteristics.push_back(ch);
        return ch;
    }

    void start() {
        started = true;
    }
};

class BLEServer {
public:
    BLEServerCallbacks* callbacks = nullptr;
    std::vector<BLEService*> services;

    ~BLEServer() {
        for (const auto* s : services) {
            delete s;
        }
        services.clear();
    }

    void setCallbacks(BLEServerCallbacks* cb) {
        callbacks = cb;
    }

    BLEService* createService(const std::string& serviceUuid) {
        auto* s = new BLEService(serviceUuid);
        services.push_back(s);
        return s;
    }

};

#endif // FAKE_BLE_SERVER_H
