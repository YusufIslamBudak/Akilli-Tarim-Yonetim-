#include "ntp_handler.h"
#include "config.h"

bool initNTP(const char* ntpServer, long gmtOffset, int daylightOffset, int timeout) {
    Serial.println("\nNTP ile zaman senkronize ediliyor...");
    configTime(gmtOffset, daylightOffset, ntpServer);
    
    int ntpRetry = 0;
    while (time(nullptr) < 100000 && ntpRetry < timeout) {
        delay(500);
        Serial.print(".");
        ntpRetry++;
    }
    
    if (time(nullptr) > 100000) {
        Serial.println("\nNTP zamani basariyla alindi!");
        Serial.print("Suan: ");
        Serial.println(getFormattedTime());
        return true;
    } else {
        Serial.println("\nUYARI: NTP zamani alinamadi, sistem zamani kullanilacak");
        return false;
    }
}

bool isNTPSynced() {
    return (time(nullptr) > 100000);
}

String getFormattedTime() {
    time_t now = time(nullptr);
    
    if (now < 100000) {
        return String(millis() / 1000) + "s (NTP yok)";
    }
    
    struct tm* timeinfo = localtime(&now);
    char buffer[64];
    // Format: 2025-11-21 14:30:45
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    return String(buffer);
}

String getTimestampForFilename() {
    time_t now = time(nullptr);
    
    if (now < 100000) {
        return "millis_" + String(millis());
    }
    
    struct tm* timeinfo = localtime(&now);
    char buffer[32];
    // Format: 2024-11-27_14-30-45
    strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", timeinfo);
    
    return String(buffer);
}
