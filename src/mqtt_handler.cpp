#include "mqtt_handler.h"
#include "config.h"
#include "certificates.h"

#if FIRMWARE_TLS == 1
#include <CertStoreBearSSL.h>

// CA Certificate loaded from certificates.h
BearSSL::X509List caCert(MQTT_CA_CERT);

#endif

MQTTHandler* MQTTHandler::_instance = nullptr;

MQTTHandler::MQTTHandler() : _mqttClient(_espClient) {
    _instance = this;
    _otaCallback = nullptr;
    _lastAttempt = 0;
    
#if FIRMWARE_TLS == 1
    // Setup TLS for MQTT with certificate verification
    // Option 1: Use CA certificate (recommended - similar to esp_crt_bundle_attach)
    _espClient.setTrustAnchors(&caCert);
    
    // Option 2: Use certificate fingerprint (alternative)
    // _espClient.setFingerprint("AA BB CC DD EE FF 00 11 22 33 44 55 66 77 88 99 AA BB CC DD");
    
    // Option 3: Skip verification (NOT RECOMMENDED - insecure!)
    // _espClient.setInsecure();
    
    Serial.println("[MQTT] TLS mode enabled with CA verification");
#endif
}

void MQTTHandler::begin() {
    _mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    _mqttClient.setCallback(messageCallback);
#if FIRMWARE_TLS == 1
    Serial.printf("[MQTT] Configured MQTTS server: %s:%d\n", MQTT_SERVER, MQTT_PORT);
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
    if (_mqttClient.connected()) {
        _mqttClient.publish(topic, payload, false);
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
