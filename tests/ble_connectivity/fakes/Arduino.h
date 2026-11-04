#ifndef FAKE_ARDUINO_H
#define FAKE_ARDUINO_H

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>

#define HIGH 1
#define LOW 0

unsigned long millis();
void delay(unsigned long ms);

#endif // FAKE_ARDUINO_H
