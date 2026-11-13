/**
 * ESP32 WiFi & OTA Update Module
 * ========================================
 * Provides:
 * - WiFi connection to home network
 * - ArduinoOTA (Over-The-Air) firmware updates via WiFi
 * - Emergency motor shutdown when OTA starts
 * 
 * Usage:
 * - Call setupOTA() once in setup() if WiFi is enabled
 * - Call handleOTA() frequently in loop() to maintain connection & check for updates
 */

#include "ESP32Wifi.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// ========== WiFi CREDENTIALS ==========
// IMPORTANT: Change these to match your network!
const char* ssid = "Soojan_Laptop";
const char* password = "Robotics123";
bool wifiConnected = false;  // Flag: tracks successful WiFi connection 

/**
 * setupOTA() - Initialize WiFi connection and OTA update handler
 * 
 * Steps:
 * 1. Start WiFi in station mode (client, not access point)
 * 2. Attempt to connect to configured SSID with timeout
 * 3. If successful, configure ArduinoOTA callbacks and start listening
 * 4. If failed, log error and continue in offline mode
 */
void setupOTA() {
    // Print startup message
    Serial.println("\nBooting WiFi...");
    
    // Set WiFi to Station mode (connect to existing network, not broadcast)
    WiFi.mode(WIFI_STA);
    
    // Attempt to connect with credentials
    WiFi.begin(ssid, password);
    
    // Wait for connection with timeout (10 seconds max: 20 attempts × 500ms)
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print("."  );  // Print dots to show waiting
        attempts++;
    }

    // Check connection result
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\n[!] WiFi Failed. OFFLINE MODE.");
        wifiConnected = false;
        return;  // Continue without WiFi
    }

    // WiFi connection successful!
    wifiConnected = true;
    // === OTA SAFETY FEATURE: EMERGENCY MOTOR BRAKE ===
    // When firmware update starts, immediately shut off motors to prevent runaway
    ArduinoOTA.onStart([]() { 
        Serial.println("Start updating");
        // Emergency stop: force both motor PWM pins to 0 (full brake)
        // This prevents the robot from running away during upload
        analogWrite(25, 0);  // PWMA = 0
        analogWrite(16, 0);  // PWMB = 0
    });
    
    // Called when OTA update completes successfully
    ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
    
    // Called periodically during upload to show progress
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    
    // Called if OTA update fails
    ArduinoOTA.onError([](ota_error_t error) { Serial.printf("Error[%u]: ", error); });

    // Configure OTA settings
    ArduinoOTA.setHostname("robot-v1");  // Identify this device on the network
    ArduinoOTA.setPassword(password);      // Require password for updates (security)
    ArduinoOTA.begin();                    // Start listening for OTA updates
    
    // Print connection success message and IP address
    Serial.println("\n[+] WIFI SUCCESS!");
    Serial.print("[+] IP address: ");
    Serial.println(WiFi.localIP());
}

/**
 * handleOTA() - Non-blocking OTA handler
 * 
 * Call this frequently (ideally every loop cycle) to:
 * - Keep WiFi connection alive
 * - Check for incoming firmware update requests
 * - Process any OTA transfers
 * 
 * IMPORTANT: This is non-blocking and returns immediately
 */
void handleOTA() {
    // Only process OTA if WiFi is connected
    if (wifiConnected) ArduinoOTA.handle(); 
}