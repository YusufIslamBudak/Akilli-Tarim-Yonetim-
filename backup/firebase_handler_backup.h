#ifndef FIREBASE_HANDLER_H
#define FIREBASE_HANDLER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// Firebase ayarlari
#define FIREBASE_HOST "tarhun-greenhouse-default-rtdb.europe-west1.firebasedatabase.app"
#define FIREBASE_AUTH "C7wyEluhxdiNQZiCCgvZjLMxmRVhL63myYt3peLo"

// Firebase nesneleri
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

// Son kontrol durumlari (tekrar ayni komutu gondermemek icin)
struct ControlState {
  bool fan = false;
  bool light = false;
  bool pump = false;
  bool initialized = false;  // Ilk okuma yapildi mi?
};

ControlState lastControlState;

// Firebase baglantisini baslat
void initFirebase() {
  Serial.println("Firebase baglantisi kuruluyor...");
  
  config.database_url = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  
  // Timeout ayarlarini yaplandır (milisaniye cinsinden)
  config.timeout.serverResponse = 10 * 1000;  // 10 saniye (varsayilan 3 sn)
  config.timeout.socketConnection = 10 * 1000;  // 10 saniye
  config.timeout.sslHandshake = 10 * 1000;  // 10 saniye
  config.timeout.rtdbKeepAlive = 45 * 1000;  // 45 saniye
  config.timeout.rtdbStreamReconnect = 1 * 1000;  // 1 saniye
  config.timeout.rtdbStreamError = 3 * 1000;  // 3 saniye
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  Serial.println("Firebase baglantisi kuruldu (timeout: 10s).");
}

// JSON verisini Firebase'e gonder
bool pushToFirebase(String jsonData) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi bagli degil, Firebase'e gonderilemedi!");
    return false;
  }
  
  // NTP zamanini al
  time_t now = time(nullptr);
  String path;
  
  // Eger NTP zamani alinmissa okunaklı format kullan
  if (now > 100000) {
    struct tm* timeinfo = localtime(&now);
    char buffer[32];
    // Format: 2024-11-27_14-30-45
    strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", timeinfo);
    path = "/sensor_data/" + String(buffer);
  } else {
    // NTP yoksa millis kullan
    path = "/sensor_data/millis_" + String(millis());
  }
  
  // Firebase'e gonder
  Serial.print("Firebase'e gonderiliyor: ");
  Serial.println(path);
  
  // FirebaseJson objesi kullan
  FirebaseJson json;
  json.setJsonData(jsonData);
  
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

// Alternatif: Set metodu ile belirli path'e yaz
bool setFirebaseData(String path, String jsonData) {
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

// SD karttaki eksik paketleri Firebase'e yukle (senkronizasyon)
void syncSDtoFirebase() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[SYNC] WiFi yok, senkronizasyon iptal edildi.");
    return;
  }
  
  Serial.println("\n=== SD <-> FIREBASE SENKRONIZASYONU ===");
  
  // SD karttaki tum .json dosyalarini tara
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
    
    // Sadece .json dosyalarini isle
    if (!entry.isDirectory() && filename.endsWith(".json")) {
      // Dosya adından timestamp'i al (ornek: 2024-11-27_14-30-45.json)
      String timestamp = filename;
      timestamp.replace(".json", "");
      timestamp.replace("/", "");  // Basta / varsa kaldir
      
      // Firebase'de bu path var mi kontrol et (getString ile)
      String firebasePath = "/sensor_data/" + timestamp;
      String existingData = "";
      
      bool exists = Firebase.RTDB.getString(&firebaseData, firebasePath.c_str());
      
      if (exists && firebaseData.dataType() == "json") {
        // Firebase'de zaten var, atla
        skipCount++;
      } else {
        // Firebase'de yok, yukle
        Serial.print("[SYNC] Yukleniyor: ");
        Serial.println(timestamp);
        
        // Dosyayi oku
        String jsonContent = "";
        while (entry.available()) {
          jsonContent += (char)entry.read();
        }
        jsonContent.trim();
        
        // Firebase'e yukle (retry mekanizmasi ile)
        FirebaseJson json;
        json.setJsonData(jsonContent);
        
        bool success = false;
        int retryCount = 0;
        const int maxRetries = 3;
        
        while (!success && retryCount < maxRetries) {
          if (Firebase.RTDB.setJSON(&firebaseData, firebasePath.c_str(), &json)) {
            Serial.println("  -> Basarili!");
            syncCount++;
            success = true;
          } else {
            retryCount++;
            Serial.print("  -> HATA (deneme ");
            Serial.print(retryCount);
            Serial.print("/");
            Serial.print(maxRetries);
            Serial.print("): ");
            Serial.println(firebaseData.errorReason());
            
            if (retryCount < maxRetries) {
              Serial.println("  -> 2 saniye sonra yeniden denenecek...");
              delay(2000);  // Retry oncesi bekle
            }
          }
        }
        
        if (!success) {
          Serial.println("  -> Tum denemeler basarisiz, dosya atlanacak.");
        }
        
        delay(500);  // Firebase rate limit icin (100ms -> 500ms)
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

// Firebase'den komut oku ve isle
void checkFirebaseCommands() {
  if (WiFi.status() != WL_CONNECTED) {
    return;  // WiFi yoksa kontrol etme
  }
  
  // Firebase'den /commands path'inden oku
  if (Firebase.RTDB.getString(&firebaseData, "/commands/pending")) {
    if (firebaseData.dataType() == "string") {
      String command = firebaseData.stringData();
      
      if (command.length() > 0 && command != "null" && command != "") {
        Serial.println("\n[FIREBASE CMD] Komut alindi: " + command);
        
        // Komutu isle (Arduino'ya gonder)
        // Bu fonksiyon main.cpp'de tanimli olacak
        extern void sendCommandToArduino(String);
        sendCommandToArduino(command);
        
        // Komutu islendikten sonra temizle
        Firebase.RTDB.setString(&firebaseData, "/commands/pending", "");
        
        // Islenen komutu log'a kaydet
        String logPath = "/commands/history/" + String(millis());
        Firebase.RTDB.setString(&firebaseData, logPath.c_str(), command);
        
        Serial.println("[FIREBASE CMD] Komut islendi ve temizlendi.");
      }
    }
  }
}

// Firebase /kontrol path'inden fan, light, pump durumlarini kontrol et
void checkFirebaseControl() {
  if (WiFi.status() != WL_CONNECTED) {
    return;  // WiFi yoksa kontrol etme
  }
  
  // /kontrol path'ini oku
  if (Firebase.RTDB.getJSON(&firebaseData, "/kontrol")) {
    if (firebaseData.dataType() == "json") {
      FirebaseJson &json = firebaseData.jsonObject();
      FirebaseJsonData result;
      
      bool fan, light, pump;
      bool hasChanges = false;
      
      // fan durumunu oku
      if (json.get(result, "fan")) {
        fan = result.boolValue;
        
        // Ilk okuma veya durum degisti mi?
        if (!lastControlState.initialized || fan != lastControlState.fan) {
          String cmd = fan ? "havaac" : "havakapa";
          Serial.println("[KONTROL] Fan: " + String(fan ? "AC" : "KAPAT"));
          
          extern void sendCommandToArduino(String);
          sendCommandToArduino(cmd);
          
          lastControlState.fan = fan;
          hasChanges = true;
        }
      }
      
      // light durumunu oku
      if (json.get(result, "light")) {
        light = result.boolValue;
        
        if (!lastControlState.initialized || light != lastControlState.light) {
          String cmd = light ? "isikac" : "isikkapa";
          Serial.println("[KONTROL] Light: " + String(light ? "AC" : "KAPAT"));
          
          extern void sendCommandToArduino(String);
          sendCommandToArduino(cmd);
          
          lastControlState.light = light;
          hasChanges = true;
        }
      }
      
      // pump durumunu oku
      if (json.get(result, "pump")) {
        pump = result.boolValue;
        
        if (!lastControlState.initialized || pump != lastControlState.pump) {
          String cmd = pump ? "sulaac" : "sulakapa";
          Serial.println("[KONTROL] Pump: " + String(pump ? "AC" : "KAPAT"));
          
          extern void sendCommandToArduino(String);
          sendCommandToArduino(cmd);
          
          lastControlState.pump = pump;
          hasChanges = true;
        }
      }
      
      // Ilk okuma tamamlandi
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
  } else {
    // Hata durumunda sessiz kalabiliriz (WiFi gecici kesilmis olabilir)
    // Serial.println("[KONTROL] Firebase okuma hatasi: " + String(firebaseData.errorReason()));
  }
}

#endif
