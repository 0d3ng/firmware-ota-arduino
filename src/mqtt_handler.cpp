#include "mqtt_handler.h"
#include "config.h"
#include "certificates.h"

MQTTHandler* MQTTHandler::_instance = nullptr;

MQTTHandler::MQTTHandler() : _mqttClient(_espClient) {
    _instance = this;
    _otaCallback = nullptr;
    _lastAttempt = 0;
    
#if FIRMWARE_TLS == 1
    // Configure TLS buffer sizes (reduce memory usage)
    _espClient.setBufferSizes(512, 512);
    
    // Setup TLS for MQTT with fingerprint verification (lightweight, ~3KB memory)
    // Fingerprint must be updated when server certificate is renewed
    _espClient.setFingerprint(MQTT_FINGERPRINT);
    
    Serial.println("[MQTT] TLS: Fingerprint verification enabled");
    Serial.printf("[MQTT] Fingerprint: %s\n", MQTT_FINGERPRINT);
#endif
}

void MQTTHandler::begin() {
    _mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    _mqttClient.setCallback(messageCallback);
#if FIRMWARE_TLS == 1
    Serial.printf("[MQTT] Configured MQTTS: %s:%d (Fingerprint)\n", MQTT_SERVER, MQTT_PORT);
#else
    Serial.printf("[MQTT] Configured MQTT server: %s:%d\n", MQTT_SERVER, MQTT_PORT);
#endif
}

void MQTTHandler::loop() {
    if (!_mqttClient.connected()) {
        reconnect();
    }
    _mqttClient.loop();
}

bool MQTTHandler::isConnected() {
    return _mqttClient.connected();
}

void MQTTHandler::publish(const char* topic, const char* payload) {
    // Try to reconnect if disconnected
    if (!_mqttClient.connected()) {
        reconnect();
    }
    
    // Publish with retry (3 attempts)
    if (_mqttClient.connected()) {
        for (int i = 0; i < 3; i++) {
            if (_mqttClient.publish(topic, payload, false)) {
                // Success
                return;
            }
            // Failed, wait and retry
            delay(100);
            yield();
        }
        Serial.printf("[MQTT] Failed to publish to %s after 3 attempts\n", topic);
    } else {
        Serial.printf("[MQTT] Not connected, cannot publish to %s\n", topic);
    }
}

void MQTTHandler::setOTACallback(void (*callback)()) {
    _otaCallback = callback;
}

void MQTTHandler::reconnect() {
    unsigned long now = millis();
    
    // Try to reconnect every MQTT_RECONNECT_INTERVAL
    if (now - _lastAttempt < MQTT_RECONNECT_INTERVAL) {
        return;
    }
    _lastAttempt = now;
    
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }
    
    Serial.print("[MQTT] Connecting...");
    
    // Create client ID
    String clientId = "ESP8266-";
    clientId += String(ESP.getChipId(), HEX);
    
    // Attempt to connect
    if (_mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
        Serial.println(" Connected!");
        
        // Subscribe to OTA topic
        if (_mqttClient.subscribe(MQTT_TOPIC_OTA, 1)) {
            Serial.printf("[MQTT] Subscribed to: %s\n", MQTT_TOPIC_OTA);
        } else {
            Serial.println("[MQTT] Subscription failed!");
        }
    } else {
        Serial.printf(" Failed, rc=%d\n", _mqttClient.state());
    }
}

void MQTTHandler::messageCallback(char* topic, byte* payload, unsigned int length) {
    Serial.printf("[MQTT] Message arrived [%s]: ", topic);
    
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println(message);
    
    // Check if OTA update trigger
    if (String(topic) == MQTT_TOPIC_OTA && message == "start") {
        Serial.println("[MQTT] OTA update triggered!");
        if (_instance && _instance->_otaCallback) {
            _instance->_otaCallback();
        }
    }
}
