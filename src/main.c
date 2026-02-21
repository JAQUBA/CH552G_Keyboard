// CH552G Keyboard - Main
// Blink example for testing the build

#include <Arduino.h>

// CH552G LED pin - adjust to your hardware
#define LED_PIN 33  // P3.3

void setup() {
    pinMode(LED_PIN, OUTPUT);
}

void loop() {
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
}
