#include <Arduino.h> 
#include "ESP32Wifi.h"
// ==========================================
// PURE SENSOR DIAGNOSTIC
// ==========================================

const bool WIFI_ENABLED = false; 
const int sensorPins[8] = {36, 39, 34, 35, 32, 33, 22, 23};
int LINE = 1; // RLS-08 reads 1 on black

void setup() {
    Serial.begin(115200);
    for (int i = 0; i < 8; i++) {
        pinMode(sensorPins[i], INPUT);
    }

    if (WIFI_ENABLED) setupOTA(); 

    Serial.println("--- SENSOR DIAGNOSTIC STARTED ---");
}

void loop() {
    if (WIFI_ENABLED) handleOTA();
    Serial.print("Sensors (L->R): ");
    for (int i = 0; i < 8; i++) {
        Serial.print(digitalRead(sensorPins[i]));
        Serial.print(" ");
    }
    Serial.println(); // Next line
    delay(100);       // Slow it down so humans can read it
}