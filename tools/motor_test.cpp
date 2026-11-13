#include <Arduino.h>
#include "ESP32Wifi.h"
// ==========================================
// PURE MOTOR DIAGNOSTIC (TB6612FNG)
// ==========================================

const bool WIFI_ENABLED = true;

#define PWMA 25   
#define AIN1 26   
#define AIN2 27   
#define PWMB 16   
#define BIN1 17   
#define BIN2 18   
#define BONUS_LED 4

int testSpeed = 150; 

void setup() {
    Serial.begin(115200);
    
    pinMode(PWMA, OUTPUT); pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
    pinMode(PWMB, OUTPUT); pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
    pinMode(BONUS_LED, OUTPUT);
    if (WIFI_ENABLED) setupOTA();

    Serial.println("=== MOTOR DIAGNOSTIC STARTING ===");
    delay(3000); 
    
    
}

void loop() {
    // --- TEST 1: BOTH MOTORS FORWARD ---
    if (WIFI_ENABLED) handleOTA();
    Serial.println("[TEST] Motors -> Forward");

    // digitalWrite(BONUS_LED, HIGH); // <--- TURNS LED ON
    digitalWrite(BONUS_LED, LOW); // <--- TURNS LED OFF
    
    // 1. Set Direction for both
    digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW); 
    digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
    
    // 2. Set Speed for BOTH
    analogWrite(PWMA, testSpeed); 
    analogWrite(PWMB, testSpeed); 
    delay(2000); 

    // --- BRAKE BOTH MOTORS ---
    Serial.println("[STOP] Brakes applied");
    analogWrite(PWMA, 0); 
    analogWrite(PWMB, 0); 
    delay(1000);

    // --- TEST 2: BOTH MOTORS BACKWARD ---
    Serial.println("[TEST] Motors -> Backward");
    
    // 1. Set Direction for both
    digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH);
    digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH); 
    
    // 2. Set Speed for BOTH
    analogWrite(PWMA, testSpeed); 
    analogWrite(PWMB, testSpeed); 
    delay(2000);

    // --- BRAKE BOTH MOTORS ---
    Serial.println("[STOP] Brakes applied");
    analogWrite(PWMA, 0); 
    analogWrite(PWMB, 0); 
    delay(1000);
}