#include "wifi_manager.h"
#include "config.h"

bool initWiFi(const char* ssid, const char* password, int timeout) {
    Serial.println();
    Serial.println("WiFi'ye baglaniyor...");
    Serial.print("SSID: ");
    Serial.println(ssid);
    
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < timeout) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.println("WiFi'ye basariyla baglandi!");
        Serial.print("IP Adresi: ");
        Serial.println(WiFi.localIP());
        Serial.print("Sinyal Gucu (RSSI): ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        return true;
    } else {
        Serial.println();
        Serial.println("WiFi baglantisi basarisiz!");
        Serial.println("Lutfen SSID ve sifrenizi kontrol edin.");
        return false;
    }
}

bool isWiFiConnected() {
    return (WiFi.status() == WL_CONNECTED);
}

void reconnectWiFi() {
    Serial.println("WiFi yeniden baglaniyor...");
    WiFi.reconnect();
}

int getWiFiRSSI() {
    return WiFi.RSSI();
}

String getIPAddress() {
    return WiFi.localIP().toString();
}
