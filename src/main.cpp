/**
 * LINE FOLLOWER ROBOT - V4.1 (THE GOLDEN RESTORATION)
 * ==========================================
 * Features:
 * - Restored V4 "Wait for White" continuous movement logic (Zero Delays)
 * - Restored V4 Physics and PD Math
 * - Left-First Turn Bias
 * - Non-Blocking Checkpoint Beep
 */

#include <Arduino.h>
#include "ESP32Wifi.h"

const bool WIFI_ENABLED = true;  

// ========== MOTOR PIN DEFINITIONS ==========
#define PWMA 25   
#define AIN1 26   
#define AIN2 27   

#define PWMB 16   
#define BIN1 17   
#define BIN2 18   

#define BONUS_LED 4  
#define BUZZER_PIN 5  

// D1(Right) to D8(Left) mapping - Proven to work in V4
const int sensorPins[8] = {36, 39, 34, 35, 32, 33, 22, 23};

float leftCalibration = 1.0;   
float rightCalibration = 0.98; 

// Original V4 Weights
const int weights[8] = {-40, -30, -20, -10, 10, 20, 30, 40};

int lastError = 0;   
int turnMemory = 0;  
int LINE = 1;        
bool isRecovering = false;  

unsigned long beepUntil = 0; // Timer for non-blocking beep

// ========== CONTROL PARAMETERS ==========
int baseSpeed = 130;   
float Kp = 2.0;        
float Kd = 14.0;       

void setMotorSpeeds(int left, int right);

void setup() {
    Serial.begin(115200);

    pinMode(PWMA, OUTPUT); pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
    pinMode(PWMB, OUTPUT); pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);

    for (int i = 0; i < 8; i++) {
        pinMode(sensorPins[i], INPUT);
    }
    
    pinMode(BONUS_LED, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    digitalWrite(BONUS_LED, HIGH); digitalWrite(BUZZER_PIN, HIGH); delay(100);
    digitalWrite(BONUS_LED, LOW); digitalWrite(BUZZER_PIN, LOW); 

    if (WIFI_ENABLED) setupOTA(); 
}

void loop() {
    if (WIFI_ENABLED) handleOTA();

    int error = 0;                
    int sensorsOnLine = 0;         
    int leftSensorsActive = 0;     
    int rightSensorsActive = 0;    
    
    for (int i = 0; i < 8; i++) {
        if (digitalRead(sensorPins[i]) == LINE) {
            error += weights[i]; 
            sensorsOnLine++; 
            
            // Original V4 Mapping
            if (i < 4) rightSensorsActive++;  // Indices 0-3 (D1-D4)
            else leftSensorsActive++;         // Indices 4-7 (D5-D8)
        }
    }

    // =========================================================
    // NON-BLOCKING CHECKPOINT BEEP (No stopping!)
    // =========================================================
    if (sensorsOnLine >= 6) {
        beepUntil = millis() + 150; // Set timer for 150ms beep
    }
    
    if (millis() < beepUntil) {
        digitalWrite(BUZZER_PIN, HIGH); digitalWrite(BONUS_LED, HIGH);
    } else {
        digitalWrite(BUZZER_PIN, LOW); digitalWrite(BONUS_LED, LOW);
    }

    // =========================================================
    // RECOVERY MODE (Catch the line after a spin)
    // =========================================================
    if (isRecovering) {
        if (digitalRead(sensorPins[3]) == LINE || digitalRead(sensorPins[4]) == LINE) {
            isRecovering = false; 
            if (turnMemory == 50 || turnMemory == -50) turnMemory = 0; 
        } else {
            sensorsOnLine = 0; // Force it to keep spinning
        }
    }

    // =========================================================
    // THE VAULT: INTERSECTION LOCK (Left-First Bias)
    // =========================================================
    if (turnMemory != 50 && turnMemory != -50 && !isRecovering) {
        
        // HIERARCHY: If left triggers, ALWAYS lock left. 
        if (leftSensorsActive >= 3) {
            turnMemory = 50;  // Lock Left
        } 
        else if (rightSensorsActive >= 3) {
            turnMemory = -50; // Lock Right
        } 
        else if (sensorsOnLine > 0 && sensorsOnLine <= 2) {
            turnMemory = error; 
        }
    }

    // =========================================================
    // WHITE SPACE TRIGGER (Execute the Locked Turn)
    // =========================================================
    // This allows the robot to fluidly drift off sharp corners before turning
    if (sensorsOnLine == 0) {
        isRecovering = true;  
        int spinSpeed = 110;  

        if (turnMemory < 0) {
            setMotorSpeeds(spinSpeed, -spinSpeed); // Spin Right
        } else {
            setMotorSpeeds(-spinSpeed, spinSpeed); // Spin Left
        }
        return;  
    }

    // =========================================================
    // STANDARD PD STEERING 
    // =========================================================
    if (turnMemory == 50 || turnMemory == -50) {
        error = 0;  // Force straight driving while passing over an intersection
    }

    int P = error;              
    int D = error - lastError;  
    int correction = (Kp * P) + (Kd * D);
    lastError = error;  

    // Original V4 Math
    int leftSpeed = baseSpeed - correction;   
    int rightSpeed = baseSpeed + correction;  
    
    setMotorSpeeds(leftSpeed, rightSpeed);
}

// ========== HARDWARE DRIVER FUNCTIONS ==========
void setMotorSpeeds(int left, int right) {
    static int currentLeft = -999;
    static int currentRight = -999;
    if (left == currentLeft && right == currentRight) return;
    
    currentLeft = left;
    currentRight = right;

    if (left == 0 && right == 0) {
        digitalWrite(AIN1, HIGH); digitalWrite(AIN2, HIGH); 
        digitalWrite(BIN1, HIGH); digitalWrite(BIN2, HIGH); 
        analogWrite(PWMA, 0); analogWrite(PWMB, 0);
        return;
    }

    left = left * leftCalibration;
    right = right * rightCalibration;

    if (left >= 0) {
        digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH); 
    } else {
        digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW); left = -left; 
    }

    if (right >= 0) {
        digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH); 
    } else {
        digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW); right = -right; 
    }

    left = constrain(left, 0, 255); right = constrain(right, 0, 255);
    analogWrite(PWMA, left); analogWrite(PWMB, right);
}