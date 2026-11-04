#pragma once
#include <cstdint>
#include <cstring>

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1

unsigned long millis();
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
void yield();
