#ifndef FAKE_ARDUINO_H
#define FAKE_ARDUINO_H

#include <iterator>
#include <string>

typedef std::uint8_t byte;
typedef bool boolean;

class String : public std::string {
public:
    using std::string::string;
    String(const std::string& s) : std::string(s) {}
    String(std::string&& s) noexcept : std::string(std::move(s)) {}
    String& operator=(const std::string& s) {
        std::string::operator=(s);
        return *this;
    }
    String& operator=(std::string&& s) noexcept {
        std::string::operator=(std::move(s));
        return *this;
    }
};

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1

// Mock timing and clock simulation
inline unsigned long fakeMillis = 0;
inline unsigned long millis() { return fakeMillis; }
inline void delay(const unsigned long ms) { fakeMillis += ms; }
inline void delayMicroseconds(const unsigned int us) { (void)us; }

// Mock GPIO pins and digital I/O
inline void pinMode(const std::uint8_t pin, const std::uint8_t mode) { (void)pin; (void)mode; }
inline void digitalWrite(const std::uint8_t pin, const std::uint8_t val) { (void)pin; (void)val; }
inline int digitalRead(const std::uint8_t pin) { (void)pin; return HIGH; }
inline void yield() {}

// Mock PWM analog output
inline int fakePwm[64] = {};
inline void analogWrite(const std::uint8_t pin, const int val) {
    if (pin < std::size(fakePwm)) {
        fakePwm[pin] = val;
    }
}

#endif // FAKE_ARDUINO_H
