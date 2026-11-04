#ifndef FAKE_BLE_DEVICE_H
#define FAKE_BLE_DEVICE_H

#include <cstdint>
#include <string>
#include <vector>
#include "BLEServer.h"

class BLEAdvertising {
public:
    std::vector<std::string> serviceUuids;
    bool scanResponse = false;
    uint32_t minPreferred = 0;
    bool running = false;

    void addServiceUUID(const std::string& uuid) {
        serviceUuids.push_back(uuid);
    }

    void setScanResponse(const bool set) {
        scanResponse = set;
    }

    void setMinPreferred(const uint32_t pref) {
        minPreferred = pref;
    }

    void start() {
        running = true;
    }

    void stop() {
        running = false;
    }
};

class BLEDevice {
private:
    static inline std::string deviceName;
    static inline BLEServer* serverInstance = nullptr;
    static inline BLEAdvertising advertisingInstance;

public:
    static void init(const std::string& name) {
        deviceName = name;
    }

    static BLEServer* createServer() {
        if (!serverInstance) {
            serverInstance = new BLEServer();
        }
        return serverInstance;
    }

    static BLEAdvertising* getAdvertising() {
        return &advertisingInstance;
    }

    static void startAdvertising() {
        advertisingInstance.start();
    }

    static void reset() {
        delete serverInstance;
        serverInstance = nullptr;
        advertisingInstance = BLEAdvertising();
        deviceName.clear();
    }

    static const std::string& getInitName() {
        return deviceName;
    }

    static BLEServer* getServer() {
        return serverInstance;
    }
};

#endif // FAKE_BLE_DEVICE_H
