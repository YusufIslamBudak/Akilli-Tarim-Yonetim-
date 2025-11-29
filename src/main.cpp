/**
 * @file main.cpp
 * @brief NodeMCU Sera Otomasyon Sistemi - Ana Program
 * 
 * Bu dosya sadece setup() ve loop() fonksiyonlarını içerir.
 * Tüm işlevsellik modüllere ayrılmıştır:
 * 
 * - config.h             : Tüm yapılandırma sabitleri
 * - wifi_manager         : WiFi bağlantı yönetimi
 * - ntp_handler          : NTP zaman senkronizasyonu
 * - sd_card_handler      : SD kart okuma/yazma işlemleri
 * - serial_handler       : Arduino Mega ile iletişim
 * - web_server_handler   : Web arayüzü
 * - firebase_handler     : Firebase RTDB entegrasyonu
 * 
 * @author Yusuf Islam Budak
 * @date 2025
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>

// Proje modülleri
#include "config.h"
#include "wifi_manager.h"
#include "ntp_handler.h"
#include "sd_card_handler.h"
#include "serial_handler.h"
#include "web_server_handler.h"
#include "firebase_handler.h"
#include "decision_tree.h"       // Fasulye sera karar ağacı

// JSON buffer (Arduino'dan gelen sensor verileri)
String jsonBuffer = "";
bool jsonComplete = false;

// WiFi bağlantı durumu takibi
bool wasConnected = true;

// Zamanlayıcılar
unsigned long lastWiFiCheck = 0;
unsigned long lastCommandCheck = 0;
unsigned long lastControlCheck = 0;
unsigned long lastFirebaseSend = 0;  // Firebase gönderim zamanı
String lastJSONForFirebase = "";     // Firebase için bekleyen son JSON

// Forward declaration
void processReceivedJSON();

// ============================================
// SETUP - Sistem başlatma
// ============================================
void setup() {
    // CPU frekansını 80MHz'e düşür (güç tasarrufu)
    system_update_cpu_freq(80);
    
    // USB Serial Monitor başlat
    Serial.begin(SERIAL_BAUD);
    delay(1000);
    
    // Arduino Mega ile iletişimi başlat
    initArduinoSerial(SOFT_RX_PIN, SOFT_TX_PIN, ARDUINO_BAUD);
    
    // WiFi bağlantısını kur
    initWiFi(WIFI_SSID, WIFI_PASSWORD, WIFI_CONNECT_TIMEOUT);
    
    // SD kart modülünü başlat
    initSDCard(SD_CARD_CS_PIN, SD_SPI_FREQUENCY);
    
    Serial.println("\nArduino Mega'dan JSON verisi bekleniyor...");
    Serial.println("-------------------------------------------");
    
    // NTP zaman senkronizasyonu
    initNTP(NTP_SERVER, GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SYNC_TIMEOUT);
    
    // Web server'ı başlat
    initWebServer(WEB_SERVER_PORT);
    
    // Firebase bağlantısını kur
    initFirebase();
    
    // Karar ağacını başlat
    initDecisionTree();
    
    // Başlangıçta SD'deki eksik paketleri Firebase'e yükle
    // NOT: Şimdilik devre dışı - sistem yavaşlamasına neden oluyor
    // Serial.println("\nOffline paketler kontrol ediliyor...");
    // syncSDtoFirebase();
}

// ============================================
// LOOP - Ana döngü
// ============================================
void loop() {
    // Web Server isteklerini işle
    handleWebServer();
    
    // WiFi bağlantı kontrolü
    if (millis() - lastWiFiCheck > WIFI_RECONNECT_INTERVAL) {
        lastWiFiCheck = millis();
        bool isConnected = isWiFiConnected();
        
        if (!isConnected && wasConnected) {
            Serial.println("\n!!! WiFi baglantisi koptu !!!");
            Serial.println("SD'ye kayit devam ediyor...");
        } else if (isConnected && !wasConnected) {
            Serial.println("\n!!! WiFi yeniden baglandi !!!");
            reconnectWiFi();
            delay(2000);
            // NOT: Şimdilik devre dışı - sistem yavaşlamasına neden oluyor
            // Serial.println("Offline paketler Firebase'e yukleniyor...");
            // syncSDtoFirebase();
        }
        wasConnected = isConnected;
    }
    
    // Serial Monitor'den komut oku (test için)
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        cmd.toLowerCase();
        if (cmd.length() > 0) {
            Serial.print("[NodeMCU] Komut gonderiliyor: ");
            Serial.println(cmd);
            sendCommandToArduino(cmd);
        }
    }
    
    // Firebase komut kontrolü
    if (millis() - lastCommandCheck > COMMAND_CHECK_INTERVAL) {
        lastCommandCheck = millis();
        checkFirebaseCommands();
    }
    
    // Firebase kontrol durumları
    if (millis() - lastControlCheck > CONTROL_CHECK_INTERVAL) {
        lastControlCheck = millis();
        checkFirebaseControl();
    }
    
    // Arduino'dan JSON oku
    readArduinoJSON(jsonBuffer, jsonComplete, MAX_SERIAL_READ);
    
    // Tam JSON alındıysa işle
    if (jsonComplete) {
        processReceivedJSON();
    }
    
    // Karar ağacını çalıştır (10 saniyede bir)
    runDecisionTreeIfNeeded();
    
    // Firebase'e periyodik gönderim (30 saniyede bir)
    if (millis() - lastFirebaseSend > FIREBASE_SEND_INTERVAL) {
        lastFirebaseSend = millis();
        if (lastJSONForFirebase.length() > 0 && isWiFiConnected()) {
            Serial.println("[Firebase] Periyodik gonderim...");
            if (pushToFirebase(lastJSONForFirebase)) {
                Serial.println("[Firebase] OK");
            } else {
                Serial.println("[Firebase] HATA");
            }
            lastJSONForFirebase = ""; // Gönderilen veriyi temizle
        }
    }
    
    // CPU'yu rahatlatmak için kısa bekleme
    delay(MAIN_LOOP_DELAY);
    yield();
}

// ============================================
// JSON İŞLEME FONKSİYONU (Optimize edildi)
// SD'ye hemen yaz, Firebase'e sadece 30 saniyede bir gönder
// Karar ağacı için sensör verilerini parse et
// ============================================
void processReceivedJSON() {
    Serial.println("\n>>> ARDUINO'DAN JSON ALINDI <<<");
    Serial.print("Boyut: ");
    Serial.print(jsonBuffer.length());
    Serial.println(" byte");
    
    // JSON validasyonu
    if (!isValidJSON(jsonBuffer)) {
        Serial.println("[HATA] Bozuk veya eksik JSON, atlanıyor!");
        jsonBuffer = "";
        jsonComplete = false;
        return;
    }
    
    // Karar ağacı için sensör verilerini parse et
    if (parseSensorData(jsonBuffer)) {
        Serial.println("[OK] Sensor verileri karar agacina aktarildi");
    }
    
    // Web interface için son veriyi sakla
    updateLastSensorData(jsonBuffer);
    
    // Günlük dosya adı oluştur (tek dosyaya append)
    String timestamp = getTimestampForFilename();
    String dateOnly = timestamp.substring(0, 10); // "2025-11-29" kısmını al
    String filename = "/" + dateOnly + ".log";
    
    // SD'ye kaydet (her zaman - çok hızlı)
    saveJSONToSD(filename, jsonBuffer);
    
    // Firebase için son veriyi sakla (30 saniyede bir gönderilecek)
    lastJSONForFirebase = jsonBuffer;
    
    Serial.println(">>> ISLEM TAMAMLANDI <<<\n");
    
    // Buffer temizle
    jsonBuffer = "";
    jsonComplete = false;
}
