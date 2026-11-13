/**
 * ESP32Wifi.h - WiFi & OTA (Over-The-Air) Update Module
 * 
 * Provides interface for WiFi connectivity and wireless firmware updates.
 * Prevents motor runaway during updates with automatic emergency brake.
 */

#ifndef ESP32WIFI_H
#define ESP32WIFI_H

/**
 * setupOTA()
 * Initialize WiFi connection and OTA update listener.
 * Call once in setup() if WiFi is enabled.
 * Automatically brakes motors when update starts (safety feature).
 */
void setupOTA();

/**
 * handleOTA()
 * Non-blocking handler for OTA updates.
 * Call frequently in loop() to maintain WiFi connection.
 * Keeps connection alive and checks for incoming firmware updates.
 */
void handleOTA();

#endif