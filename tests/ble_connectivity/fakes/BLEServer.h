#ifndef FAKE_BLESERVER_H
#define FAKE_BLESERVER_H

#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

class BLEServer;
class BLECharacteristic;

class BLEServerCallbacks {
public:
    virtual ~BLEServerCallbacks() = default;
    virtual void onConnect(BLEServer* pServer) {}
    virtual void onDisconnect(BLEServer* pServer) {}
};

class BLECharacteristicCallbacks {
public:
    virtual ~BLECharacteristicCallbacks() = default;
    virtual void onWrite(BLECharacteristic* pCharacteristic) {}
    virtual void onRead(BLECharacteristic* pCharacteristic) {}
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

    BLECharacteristic(const std::string& uuid, uint32_t props)
        : uuid(uuid), properties(props) {}

    ~BLECharacteristic() = default;

    void setCallbacks(BLECharacteristicCallbacks* cb) {
        callbacks = cb;
    }

    void setValue(const uint8_t* data, size_t size) {
        value = std::string(reinterpret_cast<const char*>(data), size);
    }

    void setValue(uint8_t* data, size_t size) {
        value = std::string(reinterpret_cast<char*>(data), size);
    }

    void setValue(const std::string& val) {
        value = val;
    }

    std::string getValue() const {
        return value;
    }

    uint8_t* getData() {
        return reinterpret_cast<uint8_t*>(const_cast<char*>(value.data()));
    }

    size_t getLength() const {
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

    explicit BLEService(const std::string& uuid) : uuid(uuid) {}

    ~BLEService() {
        for (auto* ch : characteristics) {
            delete ch;
        }
        characteristics.clear();
    }

    BLECharacteristic* createCharacteristic(const std::string& chUuid, uint32_t properties) {
        auto* ch = new BLECharacteristic(chUuid, properties);
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
        for (auto* s : services) {
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

#endif // FAKE_BLESERVER_H
