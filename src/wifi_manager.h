#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

class WiFiManager {
public:
    WiFiManager();
    bool connect();
    bool isConnected();
    String getIPAddress();
    
private:
    const char* _ssid;
    const char* _password;
    int _timeout;
};

#endif // WIFI_MANAGER_H
