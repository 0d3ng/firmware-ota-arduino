#include "mqtt_handler.h"
#include "config.h"

MQTTHandler* MQTTHandler::_instance = nullptr;

MQTTHandler::MQTTHandler() : _mqttClient(_espClient) {
    _instance = this;
    _otaCallback = nullptr;
    _lastAttempt = 0;
}

void MQTTHandler::begin() {
    _mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    _mqttClient.setCallback(messageCallback);
    Serial.printf("[MQTT] Configured server: %s:%d\n", MQTT_SERVER, MQTT_PORT);
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
