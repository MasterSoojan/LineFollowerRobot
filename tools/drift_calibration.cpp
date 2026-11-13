#include <Arduino.h>

// ==========================================
// MOTOR DRIFT CALIBRATION TEST
// ==========================================

// --- PIN DEFINITIONS ---
#define PWMA 25   
#define AIN1 26   
#define AIN2 27   
#define PWMB 16   
#define BIN1 17   
#define BIN2 18   

// --- CALIBRATION KNOBS ---
// If the robot naturally drifts to the RIGHT, your left motor is too strong. Change left to 0.95.
// If the robot naturally drifts to the LEFT, your right motor is too strong. Change right to 0.95.
float leftCalibration = 1.0; 
float rightCalibration = 0.95; 

void setMotorSpeeds(int left, int right);

void setup() {
    Serial.begin(115200);

    // Initialize Motor Pins
    pinMode(PWMA, OUTPUT); pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
    pinMode(PWMB, OUTPUT); pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);

    Serial.println("Starting in 1 seconds... Put the robot on the floor!");
    delay(1000); 
}

void loop() {
    // Blast both motors forward at 110 indefinitely.
    // No sensors, no math. Just raw voltage.
    setMotorSpeeds(110, 110);
}

// ==========================================
// HARDWARE DRIVER FUNCTIONS
// ==========================================
void setMotorSpeeds(int left, int right) {
    
    static int currentLeft = -999;
    static int currentRight = -999;
    if (left == currentLeft && right == currentRight) return;
    
    currentLeft = left;
    currentRight = right;

    if (left == 0 && right == 0) {
        digitalWrite(AIN1, HIGH); digitalWrite(AIN2, HIGH); 
        digitalWrite(BIN1, HIGH); digitalWrite(BIN2, HIGH); 
        analogWrite(PWMA, 0);
        analogWrite(PWMB, 0);
        return;
    }

    // Apply the calibration weights
    left = left * leftCalibration;
    right = right * rightCalibration;

    if (left >= 0) {
        digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH); 
    } else {
        digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW); 
        left = -left; 
    }

    if (right >= 0) {
        digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH); 
    } else {
        digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW); 
        right = -right; 
    }

    left = constrain(left, 0, 255);
    right = constrain(right, 0, 255);

    analogWrite(PWMA, left);
    analogWrite(PWMB, right);
}