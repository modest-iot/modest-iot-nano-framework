#pragma once
#include <cstdint>
#define OUTPUT 1
#define HIGH 1
#define LOW 0
inline int fakePwm[64] = {};
inline void pinMode(int, int) {}
inline void analogWrite(int pin, int value) { fakePwm[pin] = value; }
