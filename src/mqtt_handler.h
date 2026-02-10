#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "config.h"

#if FIRMWARE_TLS == 1
#include <WiFiClientSecure.h>
#endif

class MQTTHandler {
public:
    MQTTHandler();
    void begin();
    void loop();
    bool isConnected();
    void publish(const char* topic, const char* payload);
    void setOTACallback(void (*callback)());
    
private:
#if FIRMWARE_TLS == 1
    WiFiClientSecure _espClient;
#else
    WiFiClient _espClient;
#endif
    PubSubClient _mqttClient;
    void (*_otaCallback)();
    unsigned long _lastAttempt;
    
    void reconnect();
    static void messageCallback(char* topic, byte* payload, unsigned int length);
    static MQTTHandler* _instance;
};

#endif // MQTT_HANDLER_H
