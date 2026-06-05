#ifndef BUTTON_H
#define BUTTON_H

#include "Sensor.h"

class Button : public Sensor {
private:
    unsigned long lastInterruptTimestamp;
    const unsigned long debounceDelayInMilliseconds;

protected:
    void processEvent(Event& event) override;

public:
    static const int BUTTON_PRESSED_EVENT_IDENTIFIER = 10;
    static const Event BUTTON_PRESSED_EVENT;

    Button(int signalGpioPin, unsigned long debounceDelay = 200, EventHandler* parentHandler = nullptr);
    void handleHardwareInterrupt();
};

#endif // BUTTON_H