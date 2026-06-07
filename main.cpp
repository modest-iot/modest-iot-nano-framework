#include <Arduino.h>

// This replaces your main() initialization phase
void setup() {
    // Open the hardware UART Serial line at a standard baud rate speed
    Serial.begin(115200);
    delay(1000); // Give the serial connection a brief moment to settle

    Serial.println("Hello, World from ESP32-WROOM-DA!");
}

// This runs continuously on Core 1 after setup finishes
void loop() {
    // Keep this empty if you only want it to print once on boot,
    // or add recurring logic here.
}