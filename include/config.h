#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================
// WiFi AYARLARI
// ============================================
#define WIFI_SSID "TurkTelekom_ZEHX3"
#define WIFI_PASSWORD "fE65e72Db177c"
#define WIFI_CONNECT_TIMEOUT 30      // Baglanti denemesi (500ms x 30 = 15sn)
#define WIFI_RECONNECT_INTERVAL 10000 // WiFi kontrol suresi (ms)

// ============================================
// SERIAL AYARLARI
// ============================================
#define SERIAL_BAUD 115200           // USB Serial Monitor hizi
#define ARDUINO_BAUD 9600            // Arduino Mega ile haberlesme hizi

// ============================================
// PIN TANIMLARI
// ============================================
// SoftwareSerial pinleri (Arduino Mega ile CIFT YONLU haberlesme)
// D1 = GPIO5 (RX - Arduino TX2'den veri al)
// D2 = GPIO4 (TX - Arduino RX2'ye komut gonder)
#define SOFT_RX_PIN D1               // GPIO5 - Arduino TX2'den buraya baglanir
#define SOFT_TX_PIN D2               // GPIO4 - Arduino RX2'ye komut gonderir

// SD Kart CS pini (Firebase kütüphanesi SD_CS_PIN kullanıyor, biz farklı isim kullanıyoruz)
#define SD_CARD_CS_PIN D8            // GPIO15

// ============================================
// NTP AYARLARI
// ============================================
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC (3 * 3600)    // GMT+3 (Turkiye)
#define DAYLIGHT_OFFSET_SEC 0        // Yaz saati yok
#define NTP_SYNC_TIMEOUT 10          // NTP senkronizasyon denemesi (500ms x 10 = 5sn)

// ============================================
// FIREBASE AYARLARI
// ============================================
#define FIREBASE_HOST "tarhun-greenhouse-default-rtdb.europe-west1.firebasedatabase.app"
#define FIREBASE_AUTH "C7wyEluhxdiNQZiCCgvZjLMxmRVhL63myYt3peLo"

// Firebase timeout ayarlari (milisaniye)
#define FB_TIMEOUT_SERVER 5000       // 5 saniye
#define FB_TIMEOUT_SOCKET 5000       // 5 saniye
#define FB_TIMEOUT_SSL 5000          // 5 saniye
#define FB_TIMEOUT_KEEPALIVE 30000   // 30 saniye
#define FB_TIMEOUT_RECONNECT 500     // 0.5 saniye
#define FB_TIMEOUT_ERROR 2000        // 2 saniye

// ============================================
// ZAMANLAMA AYARLARI
// ============================================
#define COMMAND_CHECK_INTERVAL 5000   // Firebase komut kontrol suresi (ms)
#define CONTROL_CHECK_INTERVAL 5000   // Firebase /kontrol kontrol suresi (ms)
#define FIREBASE_SEND_INTERVAL 30000  // Firebase'e veri gonderim araligi (30sn - SD her zaman yazilir)
#define MAIN_LOOP_DELAY 10            // Ana dongu bekleme suresi (ms) - 50->10
#define MAX_SERIAL_READ 200           // Dongude maksimum karakter okuma - 100->200

// ============================================
// SD KART AYARLARI
// ============================================
#define SD_SPI_FREQUENCY 4000000     // 4MHz (MH-SD icin guvenli hiz)

// ============================================
// WEB SERVER AYARLARI
// ============================================
#define WEB_SERVER_PORT 80

#endif // CONFIG_H
