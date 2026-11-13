#ifndef FAKE_WIFI_H
#define FAKE_WIFI_H

#include <time.h>
#include "Client.h"
#include <string>

typedef enum {
    WL_NO_SHIELD = 255,
    WL_IDLE_STATUS = 0,
    WL_NO_SSID_AVAIL = 1,
    WL_SCAN_COMPLETED = 2,
    WL_CONNECTED = 3,
    WL_CONNECT_FAILED = 4,
    WL_CONNECTION_LOST = 5,
    WL_DISCONNECTED = 6
} wl_status_t;

class WiFiClient : public Client {
public:
    WiFiClient() = default;
    ~WiFiClient() override = default;
};

class IPAddress {
private:
    uint32_t address;
public:
    IPAddress() : address(0) {}
    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
        : address(a | (b << 8) | (c << 16) | (d << 24)) {}
    [[nodiscard]] bool isSet() const { return address != 0; }
    [[nodiscard]] uint32_t value() const { return address; }
};

class WiFiClass {
private:
    wl_status_t currentStatus;
    std::string connectedSsid;
    std::string connectedPassword;
    bool isPersistent;
    int32_t signalStrengthInDbm;
    IPAddress localAddress;

public:
    static inline int beginCallCount = 0;
    static inline std::string lastBeginSsid = "";
    static inline std::string lastBeginPassword = "";

    static void resetMockState() {
        beginCallCount = 0;
        lastBeginSsid.clear();
        lastBeginPassword.clear();
    }

    WiFiClass()
        : currentStatus(WL_DISCONNECTED), isPersistent(false),
          signalStrengthInDbm(0), localAddress() {}

    void persistent(bool p) {
        isPersistent = p;
    }

    [[nodiscard]] bool getPersistent() const {
        return isPersistent;
    }

    wl_status_t status() const {
        return currentStatus;
    }

    void setStatus(wl_status_t s) {
        currentStatus = s;
    }

    wl_status_t begin(const char* ssid, const char* passphrase = nullptr) {
        beginCallCount++;
        lastBeginSsid = ssid != nullptr ? ssid : "";
        lastBeginPassword = passphrase != nullptr ? passphrase : "";
        connectedSsid = lastBeginSsid;
        connectedPassword = lastBeginPassword;
        return currentStatus;
    }

    void disconnect() {
        currentStatus = WL_DISCONNECTED;
    }

    [[nodiscard]] IPAddress localIP() const {
        return localAddress;
    }

    void setLocalIP(const IPAddress& ip) {
        localAddress = ip;
    }

    [[nodiscard]] int32_t RSSI() const {
        return signalStrengthInDbm;
    }

    void setSignalStrength(int32_t rssi) {
        signalStrengthInDbm = rssi;
    }
};

inline WiFiClass WiFi;

#endif // FAKE_WIFI_H
