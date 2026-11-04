#ifndef FAKE_BLESERVER_H
#define FAKE_BLESERVER_H

#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
#include "BLE2902.h"

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
    std::vector<BLEDescriptor*> descriptors;
    size_t notifyCount = 0;

    BLECharacteristic(const std::string& uuid, uint32_t props)
        : uuid(uuid), properties(props) {}

    ~BLECharacteristic() {
        for (auto* desc : descriptors) {
            delete desc;
        }
        descriptors.clear();
    }

    void setCallbacks(BLECharacteristicCallbacks* cb) {
        callbacks = cb;
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

    void notify() {
        notifyCount++;
    }

    void addDescriptor(BLEDescriptor* descriptor) {
        if (descriptor) {
            descriptors.push_back(descriptor);
        }
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
