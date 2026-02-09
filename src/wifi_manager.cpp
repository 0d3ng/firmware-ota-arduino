#include "wifi_manager.h"
#include "config.h"
#include <ESP8266WiFi.h>

WiFiManager::WiFiManager() {
    _ssid = WIFI_SSID;
    _password = WIFI_PASSWORD;
    _timeout = WIFI_CONNECT_TIMEOUT;
}

bool WiFiManager::connect() {
    Serial.printf("Connecting to WiFi: %s ", _ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);
    
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < _timeout) {
        delay(500);
        Serial.print(".");
        retries++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" Connected!");
        Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
        return true;
    } else {
        Serial.println(" Failed!");
        return false;
    }
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String WiFiManager::getIPAddress() {
    return WiFi.localIP().toString();
}
