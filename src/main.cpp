#include <Arduino.h>
#include <FS.h>
#include "config.h"
#include "wifi_manager.h"
#include "ntp_sync.h"
#include "mqtt_handler.h"
#include "ota_updater.h"

// Global objects
WiFiManager wifiManager;
NTPSync ntpSync;
MQTTHandler mqttHandler;
OTAUpdater otaUpdater;

// OTA control flag
volatile bool ota_flag = false;

// OTA trigger callback
void onOTATrigger() {
    ota_flag = true;
}

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n\n=== ESP8266 OTA Firmware Updater ===");
    Serial.printf("Current Version: %s\n", FIRMWARE_VERSION);
    #if FIRMWARE_TLS == 1
    Serial.println("TLS: Enabled (Secure Connection)");
    #else
    Serial.println("TLS: Disabled (Insecure Connection)");
    #endif
    
    // Initialize SPIFFS
    if (!SPIFFS.begin()) {
        Serial.println("Failed to mount SPIFFS, formatting...");
        SPIFFS.format();
        SPIFFS.begin();
    }
    
    FSInfo fs_info;
    SPIFFS.info(fs_info);
    Serial.printf("SPIFFS: total=%d, used=%d bytes\n", fs_info.totalBytes, fs_info.usedBytes);
    
    // Connect to WiFi
    wifiManager.connect();
    
    // Initialize NTP
    ntpSync.initialize();
    
    // Setup MQTT
    mqttHandler.begin();
    mqttHandler.setOTACallback(onOTATrigger);
    
    // Setup OTA with MQTT handler for monitoring
    otaUpdater.setMQTTHandler(&mqttHandler);
    
    Serial.println("Setup complete. Waiting for MQTT trigger...");
}

void loop() {
    // Keep MQTT connection alive
    mqttHandler.loop();
    
    // Check for OTA updates
    if (ota_flag) {
        ota_flag = false;
        otaUpdater.checkForUpdates();
    }
    
    // Main application code here
    delay(100);
    
    // Periodic status update
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > STATUS_UPDATE_INTERVAL) {
        lastStatus = millis();
        Serial.println("[APP] Running...");
    }
}
