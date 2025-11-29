#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>

/**
 * @brief WiFi baglantisini baslatir
 * @param ssid WiFi agi adi
 * @param password WiFi sifresi
 * @param timeout Maksimum deneme sayisi
 * @return true basarili, false basarisiz
 */
bool initWiFi(const char* ssid, const char* password, int timeout);

/**
 * @brief WiFi baglanti durumunu kontrol eder
 * @return true bagli, false bagli degil
 */
bool isWiFiConnected();

/**
 * @brief WiFi'yi yeniden baglar
 */
void reconnectWiFi();

/**
 * @brief WiFi sinyal gucunu dondurur
 * @return RSSI degeri (dBm)
 */
int getWiFiRSSI();

/**
 * @brief IP adresini String olarak dondurur
 * @return IP adresi
 */
String getIPAddress();

#endif // WIFI_MANAGER_H
