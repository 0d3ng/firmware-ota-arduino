#include "ota_updater.h"
#include "mqtt_handler.h"
#include "config.h"
#include "certificates.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <bearssl/bearssl_hash.h>
#include <Ed25519.h>
#include <time.h>

OTAUpdater::OTAUpdater() : _mqttHandler(nullptr), _stageStartTime(0) {
}

void OTAUpdater::setMQTTHandler(MQTTHandler* mqtt) {
    _mqttHandler = mqtt;
}

void OTAUpdater::checkForUpdates() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[OTA] WiFi not connected");
        return;
    }
    
    // Check if time is synced (required for TLS certificate verification)
    time_t now = time(nullptr);
    if (now < 1000000000) {
        Serial.println("[OTA] Time not synced! TLS will fail. Please wait for NTP sync.");
        return;
    }
    
    Serial.println("\n[OTA] Checking for updates...");
    Serial.printf("[OTA] Current time: %s", ctime(&now));
    Serial.printf("[OTA] Free heap: %d bytes\n", ESP.getFreeHeap());
    
    monitorStartStage();
    String manifestData;
    if (!downloadManifest(manifestData)) {
        Serial.println("[OTA] Failed to download manifest");
        return;
    }
    monitorEndStage("download_manifest");
    
    monitorStartStage();
    String newVersion, expectedHash, signature;
    if (!parseManifest(manifestData, newVersion, expectedHash, signature)) {
        Serial.println("[OTA] Failed to parse manifest");
        return;
    }
    monitorEndStage("parse_manifest");
    
    Serial.printf("[OTA] Current version: %s\n", FIRMWARE_VERSION);
    Serial.printf("[OTA] New version: %s\n", newVersion.c_str());
    
    int cmp = compareVersions(FIRMWARE_VERSION, newVersion);
    if (cmp <= 0) {
        Serial.println("[OTA] No update needed (current >= new)");
        return;
    }
    
    Serial.println("[OTA] Update available! Starting OTA...");
    performOTA(expectedHash, signature);
}

bool OTAUpdater::downloadManifest(String& manifestData) {
    HTTPClient http;
    WiFiClient client;
    
#if FIRMWARE_TLS == 1
    WiFiClientSecure secureClient;
    
    // Configure TLS buffer sizes (reduce memory usage)
    secureClient.setBufferSizes(512, 512);
    
    // Use fingerprint verification (lightweight, ~3KB memory vs ~20KB for CA cert)
    secureClient.setFingerprint(OTA_FINGERPRINT);
    
    Serial.println("[HTTPS] TLS: Fingerprint verification");
    Serial.printf("[HTTPS] Fingerprint: %s\n", OTA_FINGERPRINT);
    Serial.printf("[HTTPS] Free heap: %d bytes\n", ESP.getFreeHeap());
    
    if (!http.begin(secureClient, MANIFEST_URL)) {
        Serial.println("[HTTPS] ERROR: Failed to begin HTTPS connection");
        return false;
    }
#else
    if (!http.begin(client, MANIFEST_URL)) {
        Serial.println("[HTTP] ERROR: Failed to begin connection");
        return false;
    }
#endif
    
    http.addHeader("Accept-Encoding", "identity");
    http.addHeader("User-Agent", "ESP8266");
    
    Serial.println("[HTTP] Sending GET request...");
    int httpCode = http.GET();
    Serial.printf("[HTTP] Response code: %d\n", httpCode);
    
    if (httpCode == HTTP_CODE_OK) {
        manifestData = http.getString();
        Serial.printf("[HTTP] Manifest downloaded: %d bytes\n", manifestData.length());
        http.end();
        return true;
    } else {
        Serial.printf("[HTTP] GET failed, error: %s\n", http.errorToString(httpCode).c_str());
        
#if FIRMWARE_TLS == 1
        // Additional debugging for TLS errors
        if (httpCode == HTTPC_ERROR_CONNECTION_FAILED) {
            Serial.println("[HTTPS] Connection failed. Possible causes:");
            Serial.println("  - Certificate fingerprint mismatch");
            Serial.println("  - Server certificate changed (update FINGERPRINT)");
            Serial.println("  - Time not synchronized (check NTP)");
            Serial.printf("  - Free heap: %d bytes\n", ESP.getFreeHeap());
        }
#endif
        http.end();
        return false;
    }
}

bool OTAUpdater::parseManifest(const String& manifestData, String& version, String& hash, String& signature) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, manifestData);
    
    if (error) {
        Serial.printf("[JSON] Parse failed: %s\n", error.c_str());
        return false;
    }
    
    if (!doc.containsKey("version") || !doc.containsKey("hash") || !doc.containsKey("signature")) {
        Serial.println("[JSON] Missing required fields (version, hash, signature)");
        return false;
    }
    
    version = doc["version"].as<String>();
    hash = doc["hash"].as<String>();
    signature = doc["signature"].as<String>();
    
    Serial.printf("[Manifest] Version: %s\n", version.c_str());
    Serial.printf("[Manifest] Hash: %s\n", hash.c_str());
    Serial.printf("[Manifest] Signature: %s...\n", signature.substring(0, 32).c_str());
    
    return true;
}

int OTAUpdater::compareVersions(const String& currentVer, const String& newVer) {
    int idx1 = currentVer.indexOf('-');
    int idx2 = currentVer.indexOf('-', idx1 + 1);
    int idx3 = newVer.indexOf('-');
    int idx4 = newVer.indexOf('-', idx3 + 1);
    
    if (idx1 == -1 || idx2 == -1 || idx3 == -1 || idx4 == -1) {
        return 0;
    }
    
    String currentTS = currentVer.substring(idx1 + 1, idx2);
    String newTS = newVer.substring(idx3 + 1, idx4);
    
    if (currentVer.endsWith("-local") && newVer.indexOf("-build") > 0) {
        return 1;
    }
    
    int cmp = newTS.compareTo(currentTS);
    if (cmp > 0) return 1;
    if (cmp < 0) return -1;
    return 0;
}

int OTAUpdater::hexStringToBytes(const String& hexStr, uint8_t* output, size_t maxLen) {
    size_t hexLen = hexStr.length();
    if (hexLen % 2 != 0) {
        Serial.println("[HEX] Odd length hex string");
        return -1;
    }
    
    size_t byteLen = hexLen / 2;
    if (byteLen > maxLen) {
        Serial.printf("[HEX] Buffer too small: %d > %d\n", byteLen, maxLen);
        return -1;
    }
    
    for (size_t i = 0; i < byteLen; i++) {
        String byteStr = hexStr.substring(i * 2, i * 2 + 2);
        output[i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
    }
    
    return byteLen;
}

bool OTAUpdater::verifySignature(const uint8_t* hash, size_t hashLen, const uint8_t* signature, size_t sigLen) {
    Serial.println("[OTA] Verifying ED25519 signature...");
    
    if (sigLen != 64) {
        Serial.printf("[OTA] Invalid signature length: %d (expected 64)\n", sigLen);
        return false;
    }
    
    if (hashLen != 32) {
        Serial.printf("[OTA] Invalid hash length: %d (expected 32)\n", hashLen);
        return false;
    }
    
    uint8_t publicKey[32];
    String pubKeyHex = String(PUBLIC_KEY_HEX);
    
    if (pubKeyHex.length() != 64) {
        Serial.printf("[OTA] Invalid public key length: %d (expected 64 hex chars)\n", pubKeyHex.length());
        return false;
    }
    
    int keyLen = hexStringToBytes(pubKeyHex, publicKey, sizeof(publicKey));
    if (keyLen != 32) {
        Serial.println("[OTA] Failed to parse public key");
        return false;
    }
    
    bool verified = Ed25519::verify(signature, publicKey, hash, hashLen);
    
    if (verified) {
        Serial.println("[OTA] ✓ ED25519 signature verification PASSED");
        return true;
    } else {
        Serial.println("[OTA] ✗ ED25519 signature verification FAILED");
        return false;
    }
}

void OTAUpdater::performOTA(const String& expectedHashHex, const String& signatureHex) {
    Serial.println("[OTA] Starting firmware download and verification...");
    Serial.printf("[OTA] Free heap: %d bytes\n", ESP.getFreeHeap());
    
    monitorStartStage();
    
    WiFiClient client;
    HTTPClient http;
    
#if FIRMWARE_TLS == 1
    WiFiClientSecure secureClient;
    
    // Configure TLS buffer sizes
    secureClient.setBufferSizes(512, 512);
    
    // Use fingerprint verification
    secureClient.setFingerprint(OTA_FINGERPRINT);
    
    Serial.println("[HTTPS] TLS: Fingerprint verification");
    Serial.printf("[HTTPS] Free heap: %d bytes\n", ESP.getFreeHeap());
    
    if (!http.begin(secureClient, FIRMWARE_URL)) {
        Serial.println("[HTTPS] ERROR: Failed to begin HTTPS connection");
        return;
    }
#else
    if (!http.begin(client, FIRMWARE_URL)) {
        Serial.println("[HTTP] ERROR: Failed to begin connection");
        return;
    }
#endif

    Serial.println("[OTA] Downloading firmware for verification...");
    Serial.printf("[OTA] Free heap before download: %d bytes\n", ESP.getFreeHeap());
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[OTA] Download failed: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        return;
    }
    
    int totalSize = http.getSize();
    Serial.printf("[OTA] Firmware size: %d bytes\n", totalSize);
    
    br_sha256_context sha256_ctx;
    br_sha256_init(&sha256_ctx);
    
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buffer[OTA_DOWNLOAD_BUFFER];
    int totalRead = 0;
    int lastPercent = -1;
    
    File firmware = SPIFFS.open("/firmware.tmp", "w");
    if (!firmware) {
        Serial.println("[OTA] Failed to open temp file");
        http.end();
        return;
    }
    
    while (http.connected() && (totalRead < totalSize || totalSize == -1)) {
        size_t available = stream->available();
        if (available) {
            int readLen = stream->readBytes(buffer, min((size_t)sizeof(buffer), available));
            if (readLen > 0) {
                firmware.write(buffer, readLen);
                br_sha256_update(&sha256_ctx, buffer, readLen);
                totalRead += readLen;
                
                int percent = (totalRead * 100) / totalSize;
                if (percent != lastPercent && percent % 10 == 0) {
                    Serial.printf("[OTA] Download: %d%% (%d/%d)\n", percent, totalRead, totalSize);
                    lastPercent = percent;
                }
            }
        }
        yield();
    }
    
    firmware.close();
    http.end();
    
    Serial.printf("[OTA] Download complete: %d bytes\n", totalRead);
    monitorEndStage("stream_firmware");
    
    monitorStartStage();
    uint8_t calculatedHash[32];
    br_sha256_out(&sha256_ctx, calculatedHash);
    
    char hashHex[65];
    for (int i = 0; i < 32; i++) {
        sprintf(hashHex + (i * 2), "%02x", calculatedHash[i]);
    }
    hashHex[64] = 0;
    
    Serial.printf("[OTA] Calculated hash: %s\n", hashHex);
    Serial.printf("[OTA] Expected hash: %s\n", expectedHashHex.c_str());
    
    if (expectedHashHex != String(hashHex)) {
        Serial.println("[OTA] ERROR: Hash mismatch!");
        SPIFFS.remove("/firmware.tmp");
        return;
    }
    
    Serial.println("[OTA] Hash verification passed!");
    monitorEndStage("verify_hash");
    
    monitorStartStage();
    uint8_t signatureBytes[128];
    int sigLen = hexStringToBytes(signatureHex, signatureBytes, sizeof(signatureBytes));
    
    if (sigLen < 0) {
        Serial.println("[OTA] ERROR: Failed to parse signature");
        SPIFFS.remove("/firmware.tmp");
        return;
    }
    
    if (!verifySignature(calculatedHash, 32, signatureBytes, sigLen)) {
        Serial.println("[OTA] ERROR: Signature verification failed!");
        SPIFFS.remove("/firmware.tmp");
        return;
    }
    
    Serial.println("[OTA] Signature verification passed!");
    monitorEndStage("verify_signature");
    
    Serial.println("[OTA] Proceeding to flash...");
    monitorStartStage();
    
    SPIFFS.remove("/firmware.tmp");
    
#if FIRMWARE_TLS == 1
    WiFiClientSecure secureClient2;
    
    // Configure TLS buffer sizes
    secureClient2.setBufferSizes(512, 512);
    
    // Use fingerprint verification for final flash
    secureClient2.setFingerprint(OTA_FINGERPRINT);
    
    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
    
    Serial.println("[OTA] Starting HTTPS update...");
    Serial.printf("[OTA] Free heap: %d bytes\n", ESP.getFreeHeap());
    t_httpUpdate_return ret = ESPhttpUpdate.update(secureClient2, FIRMWARE_URL);
#else
    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
    
    ESPhttpUpdate.onStart([]() {
        Serial.println("[OTA] Update started");
    });
    
    ESPhttpUpdate.onEnd([]() {
        Serial.println("\n[OTA] Update finished");
    });
    
    ESPhttpUpdate.onProgress([](int current, int total) {
        static int lastPercent = -1;
        int percent = (current * 100) / total;
        if (percent != lastPercent && percent % 10 == 0) {
            Serial.printf("[OTA] Progress: %d%% (%d/%d)\n", percent, current, total);
            lastPercent = percent;
        }
    });
    
    ESPhttpUpdate.onError([](int error) {
        Serial.printf("[OTA] Error (%d): %s\n", error, ESPhttpUpdate.getLastErrorString().c_str());
    });
    
    Serial.println("[OTA] Starting HTTP update...");
    t_httpUpdate_return ret = ESPhttpUpdate.update(client, FIRMWARE_URL);
#endif
    
    switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("[OTA] Update failed. Error (%d): %s\n", 
                        ESPhttpUpdate.getLastError(), 
                        ESPhttpUpdate.getLastErrorString().c_str());
            break;
            
        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("[OTA] No updates available");
            break;
            
        case HTTP_UPDATE_OK:
            Serial.println("[OTA] Update successful! Rebooting...");
            monitorEndStage("ota_finalize");
            delay(1000);
            ESP.restart();
            break;
    }
}

void OTAUpdater::monitorStartStage() {
    _stageStartTime = micros();
}

void OTAUpdater::monitorEndStage(const char* stageName) {
    if (!_mqttHandler) return;
    
    // Feed watchdog
    yield();
    
    // Calculate elapsed time
    unsigned long elapsed_us = micros() - _stageStartTime;
    unsigned long elapsed_ms = elapsed_us / 1000;
    
    // Get heap info
    uint32_t free_heap = ESP.getFreeHeap();
    
    // Get current time
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    // Create ISO 8601 timestamp
    char timestamp[64];
    snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02dT%02d:%02d:%02d",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    
    // Create metrics JSON
    char msg[256];
    snprintf(msg, sizeof(msg),
             "{\"stage\":\"%s\",\"elapsed_ms\":%lu,\"free_heap\":%u,\"algorithm\":\"ed25519\",\"timestamp\":\"%s\"}",
             stageName, elapsed_ms, free_heap, timestamp);
    
    Serial.printf("[%s] Stage %s completed in %lu ms, free_heap=%u\n", 
                  timestamp, stageName, elapsed_ms, free_heap);
    
    // Publish to MQTT
    _mqttHandler->publish("ota/metrics", msg);
    
    // Feed watchdog after publish
    yield();
}
