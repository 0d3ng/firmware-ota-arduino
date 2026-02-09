#include "ntp_sync.h"
#include "config.h"
#include <time.h>
#include <sys/time.h>
#include <coredecls.h>

NTPSync::NTPSync() {
    _timeout = NTP_SYNC_TIMEOUT;
}

bool NTPSync::initialize() {
    Serial.println("[NTP] Initializing SNTP...");
    
    // Configure NTP servers
    configTime(9 * 3600, 0, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
    
    // Set timezone
    setenv("TZ", NTP_TIMEZONE, 1);
    tzset();
    
    // Wait for time to be set
    Serial.print("[NTP] Waiting for time sync");
    time_t now = time(nullptr);
    int retry = 0;
    while (now < 8 * 3600 * 2 && retry < _timeout) {
        delay(1000);
        Serial.print(".");
        now = time(nullptr);
        retry++;
    }
    Serial.println();
    
    if (now > 8 * 3600 * 2) {
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
        Serial.printf("[NTP] System time synced: %s\n", timeStr);
        return true;
    } else {
        Serial.println("[NTP] Failed to sync time, continuing anyway...");
        return false;
    }
}

String NTPSync::getCurrentTime() {
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(timeStr);
}
