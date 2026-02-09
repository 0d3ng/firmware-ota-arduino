#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

class MQTTHandler {
public:
    MQTTHandler();
    void begin();
    void loop();
    bool isConnected();
    void publish(const char* topic, const char* payload);
    void setOTACallback(void (*callback)());
    
private:
    WiFiClient _espClient;
    PubSubClient _mqttClient;
    void (*_otaCallback)();
    unsigned long _lastAttempt;
    
    void reconnect();
    static void messageCallback(char* topic, byte* payload, unsigned int length);
    static MQTTHandler* _instance;
};

#endif // MQTT_HANDLER_H
