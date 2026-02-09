#ifndef NTP_SYNC_H
#define NTP_SYNC_H

#include <Arduino.h>

class NTPSync {
public:
    NTPSync();
    bool initialize();
    String getCurrentTime();
    
private:
    int _timeout;
};

#endif // NTP_SYNC_H
