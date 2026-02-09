#ifndef OTA_UPDATER_H
#define OTA_UPDATER_H

#include <Arduino.h>

class MQTTHandler;

class OTAUpdater {
public:
    OTAUpdater();
    void setMQTTHandler(MQTTHandler* mqtt);
    void checkForUpdates();
    
private:
    MQTTHandler* _mqttHandler;
    unsigned long _stageStartTime;
    
    bool downloadManifest(String& manifestData);
    bool parseManifest(const String& manifestData, String& version, String& hash, String& signature);
    int compareVersions(const String& currentVer, const String& newVer);
    int hexStringToBytes(const String& hexStr, uint8_t* output, size_t maxLen);
    bool verifySignature(const uint8_t* hash, size_t hashLen, const uint8_t* signature, size_t sigLen);
    void performOTA(const String& expectedHashHex, const String& signatureHex);
    
    // Monitoring functions
    void monitorStartStage();
    void monitorEndStage(const char* stageName);
};

#endif // OTA_UPDATER_H
