#ifndef WEB_SERVER_HANDLER_H
#define WEB_SERVER_HANDLER_H

#include <Arduino.h>
#include <ESP8266WebServer.h>

/**
 * @brief Web server'ı başlatır
 * @param port Port numarası
 */
void initWebServer(int port);

/**
 * @brief Web server isteklerini işler (loop içinde çağrılmalı)
 */
void handleWebServer();

/**
 * @brief Son sensor verisini günceller
 * @param data Sensor verisi
 */
void updateLastSensorData(const String& data);

/**
 * @brief Son sensor verisini döndürür
 * @return Son sensor verisi
 */
String getLastSensorData();

/**
 * @brief Web server nesnesine erişim sağlar
 * @return ESP8266WebServer referansı
 */
ESP8266WebServer& getWebServer();

#endif // WEB_SERVER_HANDLER_H
