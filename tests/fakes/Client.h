#ifndef FAKE_CLIENT_H
#define FAKE_CLIENT_H

#include <cstddef>
#include <cstdint>

class Client {
public:
    virtual ~Client() = default;

    virtual int connect(const char* host, uint16_t port) {
        (void)host;
        (void)port;
        return 1;
    }

    virtual size_t write(uint8_t b) {
        (void)b;
        return 1;
    }

    virtual size_t write(const uint8_t* buf, size_t size) {
        (void)buf;
        return size;
    }

    virtual int available() { return 0; }
    virtual int read() { return -1; }
    virtual int peek() { return -1; }
    virtual void flush() {}
    virtual void stop() {}
    virtual uint8_t connected() { return 1; }
    virtual operator bool() { return true; }
};

#endif // FAKE_CLIENT_H
