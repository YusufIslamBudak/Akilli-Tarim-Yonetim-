#ifndef FIREBASE_HANDLER_H
#define FIREBASE_HANDLER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <SD.h>

#include "config.h"
#include "serial_handler.h"
#include "ntp_handler.h"
#include "decision_tree.h"

// ============================================
// Firebase nesneleri (static inline)
// ============================================
static FirebaseData firebaseData;
static FirebaseAuth auth;
static FirebaseConfig firebaseConfig;

// Son kontrol durumları
static struct {
    bool fan = false;
    bool light = false;
    bool pump = false;
    bool initialized = false;
} lastControlState;

// ============================================
// Firebase fonksiyonları (inline)
// ============================================

/**
 * @brief Firebase bağlantısını başlatır
 */
inline void initFirebase() {
    Serial.println("Firebase baglantisi kuruluyor...");
    
    firebaseConfig.database_url = FIREBASE_HOST;
    firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH;
    
    // Timeout ayarları
    firebaseConfig.timeout.serverResponse = FB_TIMEOUT_SERVER;
    firebaseConfig.timeout.socketConnection = FB_TIMEOUT_SOCKET;
    firebaseConfig.timeout.sslHandshake = FB_TIMEOUT_SSL;
    firebaseConfig.timeout.rtdbKeepAlive = FB_TIMEOUT_KEEPALIVE;
    firebaseConfig.timeout.rtdbStreamReconnect = FB_TIMEOUT_RECONNECT;
    firebaseConfig.timeout.rtdbStreamError = FB_TIMEOUT_ERROR;
    
    Firebase.begin(&firebaseConfig, &auth);
    Firebase.reconnectWiFi(true);
    
    Serial.println("Firebase baglantisi kuruldu (timeout: 10s).");
}

/**
 * @brief JSON verisini Firebase'e gönderir (karar ağacı durumu ile birlikte)
 */
inline bool pushToFirebase(const String& jsonData) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi bagli degil, Firebase'e gonderilemedi!");
        return false;
    }
    
    String timestamp = getTimestampForFilename();
    String path = "/sensor_data/" + timestamp;
    
    Serial.print("Firebase'e gonderiliyor: ");
    Serial.println(path);
    
    // Orijinal JSON'u al ve karar ağacı durumunu ekle
    String enhancedJson = jsonData;
    
    // Karar ağacı durumunu al
    DecisionStatus status = getDecisionStatus();
    
    // JSON'un sonundaki } karakterini kaldır
    if (enhancedJson.endsWith("}")) {
        enhancedJson = enhancedJson.substring(0, enhancedJson.length() - 1);
        
        // Karar ağacı verilerini ekle
        enhancedJson += ",\"decision\":{";
        enhancedJson += "\"mode\":\"" + String(status.isNightMode ? "GECE" : "GUNDUZ") + "\",";
        enhancedJson += "\"hour\":" + String(status.currentHour) + ",";
        enhancedJson += "\"code\":\"" + status.lastDecisionCode + "\",";
        enhancedJson += "\"desc\":\"" + status.lastDecisionDesc + "\",";
        enhancedJson += "\"temp\":\"" + status.tempStatus + "\",";
        enhancedJson += "\"hum\":\"" + status.humStatus + "\",";
        enhancedJson += "\"co2\":\"" + status.co2Status + "\",";
        enhancedJson += "\"soil\":\"" + status.soilStatus + "\",";
        enhancedJson += "\"pres\":\"" + status.pressureStatus + "\",";
        enhancedJson += "\"lux\":\"" + status.luxStatus + "\",";
        enhancedJson += "\"fan\":" + String(status.suggestFan ? "true" : "false") + ",";
        enhancedJson += "\"pump\":" + String(status.suggestPump ? "true" : "false") + ",";
        enhancedJson += "\"light\":" + String(status.suggestLight ? "true" : "false");
        enhancedJson += "}}";
    }
    
    FirebaseJson json;
    json.setJsonData(enhancedJson);
    
    if (Firebase.RTDB.setJSON(&firebaseData, path.c_str(), &json)) {
        Serial.println("Firebase'e basariyla gonderildi!");
        Serial.print("Path: ");
        Serial.println(firebaseData.dataPath());
        return true;
    } else {
        Serial.println("Firebase gonderimi BASARISIZ!");
        Serial.print("Hata: ");
        Serial.println(firebaseData.errorReason());
        return false;
    }
}

/**
 * @brief Belirli path'e veri yazar
 */
inline bool setFirebaseData(const String& path, const String& jsonData) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi bagli degil!");
        return false;
    }
    
    FirebaseJson json;
    json.setJsonData(jsonData);
    
    if (Firebase.RTDB.setJSON(&firebaseData, path.c_str(), &json)) {
        Serial.println("Firebase'e yazildi: " + path);
        return true;
    } else {
        Serial.println("Firebase yazma hatasi: " + String(firebaseData.errorReason()));
        return false;
    }
}

/**
 * @brief SD karttaki eksik paketleri Firebase'e yükler
 */
inline void syncSDtoFirebase() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[SYNC] WiFi yok, senkronizasyon iptal edildi.");
        return;
    }
    
    Serial.println("\n=== SD <-> FIREBASE SENKRONIZASYONU ===");
    
    File root = SD.open("/");
    if (!root) {
        Serial.println("[SYNC] SD kart okunamadi!");
        return;
    }
    
    int syncCount = 0;
    int skipCount = 0;
    
    while (true) {
        File entry = root.openNextFile();
        if (!entry) break;
        
        String filename = String(entry.name());
        
        if (!entry.isDirectory() && filename.endsWith(".json")) {
            String timestamp = filename;
            timestamp.replace(".json", "");
            timestamp.replace("/", "");
            
            String firebasePath = "/sensor_data/" + timestamp;
            
            bool exists = Firebase.RTDB.getString(&firebaseData, firebasePath.c_str());
            
            if (exists && firebaseData.dataType() == "json") {
                skipCount++;
            } else {
                Serial.print("[SYNC] Yukleniyor: ");
                Serial.println(timestamp);
                
                String jsonContent = "";
                while (entry.available()) {
                    jsonContent += (char)entry.read();
                }
                jsonContent.trim();
                
                // JSON formatını kontrol et
                if (jsonContent.length() < 10 || !jsonContent.startsWith("{") || !jsonContent.endsWith("}")) {
                    Serial.println("  -> Bozuk dosya, siliniyor...");
                    entry.close();
                    String fullPath = "/" + filename;
                    SD.remove(fullPath.c_str());
                    continue;
                }
                
                FirebaseJson json;
                json.setJsonData(jsonContent);
                
                // Tek deneme - başarısızsa atla (retry yok)
                if (Firebase.RTDB.setJSON(&firebaseData, firebasePath.c_str(), &json)) {
                    Serial.println("  -> Basarili!");
                    syncCount++;
                    // Başarılı yüklenen dosyayı sil
                    entry.close();
                    String fullPath = "/" + filename;
                    SD.remove(fullPath.c_str());
                } else {
                    Serial.print("  -> HATA: ");
                    Serial.println(firebaseData.errorReason());
                    skipCount++;
                }
                
                yield(); // Watchdog için
            }
        }
        
        entry.close();
    }
    
    root.close();
    
    Serial.println("=== SENKRONIZASYON TAMAMLANDI ===");
    Serial.print("Yuklenen: ");
    Serial.print(syncCount);
    Serial.print(" | Atlanan: ");
    Serial.println(skipCount);
    Serial.println();
}

/**
 * @brief Firebase'den bekleyen komutları kontrol eder
 */
inline void checkFirebaseCommands() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }
    
    if (Firebase.RTDB.getString(&firebaseData, "/commands/pending")) {
        if (firebaseData.dataType() == "string") {
            String command = firebaseData.stringData();
            
            if (command.length() > 0 && command != "null" && command != "") {
                Serial.println("\n[FIREBASE CMD] Komut alindi: " + command);
                
                sendCommandToArduino(command);
                
                Firebase.RTDB.setString(&firebaseData, "/commands/pending", "");
                
                String logPath = "/commands/history/" + String(millis());
                Firebase.RTDB.setString(&firebaseData, logPath.c_str(), command);
                
                Serial.println("[FIREBASE CMD] Komut islendi ve temizlendi.");
            }
        }
    }
}

/**
 * @brief Firebase /kontrol path'inden durumları kontrol eder
 */
inline void checkFirebaseControl() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }
    
    if (Firebase.RTDB.getJSON(&firebaseData, "/kontrol")) {
        if (firebaseData.dataType() == "json") {
            FirebaseJson &json = firebaseData.jsonObject();
            FirebaseJsonData result;
            
            bool fan, light, pump;
            bool hasChanges = false;
            
            // fan durumu
            if (json.get(result, "fan")) {
                fan = result.boolValue;
                
                if (!lastControlState.initialized || fan != lastControlState.fan) {
                    String cmd = fan ? "havaac" : "havakapa";
                    Serial.println("[KONTROL] Fan: " + String(fan ? "AC" : "KAPAT"));
                    sendCommandToArduino(cmd);
                    lastControlState.fan = fan;
                    hasChanges = true;
                }
            }
            
            // light durumu
            if (json.get(result, "light")) {
                light = result.boolValue;
                
                if (!lastControlState.initialized || light != lastControlState.light) {
                    String cmd = light ? "isikac" : "isikkapa";
                    Serial.println("[KONTROL] Light: " + String(light ? "AC" : "KAPAT"));
                    sendCommandToArduino(cmd);
                    lastControlState.light = light;
                    hasChanges = true;
                }
            }
            
            // pump durumu
            if (json.get(result, "pump")) {
                pump = result.boolValue;
                
                if (!lastControlState.initialized || pump != lastControlState.pump) {
                    String cmd = pump ? "sulaac" : "sulakapa";
                    Serial.println("[KONTROL] Pump: " + String(pump ? "AC" : "KAPAT"));
                    sendCommandToArduino(cmd);
                    lastControlState.pump = pump;
                    hasChanges = true;
                }
            }
            
            if (!lastControlState.initialized) {
                lastControlState.initialized = true;
                Serial.println("[KONTROL] Baslangic durumlari alindi:");
                Serial.println("  Fan: " + String(lastControlState.fan ? "AC" : "KAPAT"));
                Serial.println("  Light: " + String(lastControlState.light ? "AC" : "KAPAT"));
                Serial.println("  Pump: " + String(lastControlState.pump ? "AC" : "KAPAT"));
            } else if (hasChanges) {
                Serial.println("[KONTROL] Durum degisiklikleri uygulandi.");
            }
        }
    }
}

#endif // FIREBASE_HANDLER_H
