#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>
#include <time.h>  // NTP zaman damgası için
#include "firebase_handler.h"  // Firebase entegrasyonu

// WiFi bilgilerinizi buraya girin
const char* ssid = "TurkTelekom_ZEHX3";        // WiFi aginizin adi
const char* password = "fE65e72Db177c";        // WiFi sifreniz

// SoftwareSerial pinleri (Arduino Mega ile CIFT YONLU haberlesme)
// D1 = GPIO5 (RX - Arduino TX2'den veri al)
// D2 = GPIO4 (TX - Arduino RX2'ye komut gonder)
const uint8_t SOFT_RX = D1;  // Arduino TX2'den buraya baglanir
const uint8_t SOFT_TX = D2;  // Arduino RX2'ye komut gonderir

// SD kart CS pini
const uint8_t SD_CS = D8;  // GPIO15

// USB Serial hizi (Serial Monitor icin)
const unsigned long SERIAL_BAUD = 115200;

// Arduino Mega ile SoftwareSerial - 9600 baud (CIFT YONLU)
const unsigned long ARDUINO_BAUD = 9600;

// NTP Sunucu Ayarları (Türkiye saati için)
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3 * 3600;  // GMT+3 (Türkiye)
const int daylightOffset_sec = 0;      // Yaz saati yok

// SoftwareSerial nesnesi
SoftwareSerial arduinoSerial(SOFT_RX, SOFT_TX);

// Web Server (Port 80)
ESP8266WebServer server(80);

// JSON buffer (Arduino'dan gelen sensor verileri)
String jsonBuffer = "";
bool jsonComplete = false;

// Son alinan sensor verileri (Web interface icin)
String lastSensorData = "Henuz veri yok...";

// SD kart durumu (global bayrak)
bool sdCardReady = false;

// Fonksiyon prototipleri (Forward declarations)
void sendCommandToArduino(String command);
void handleRoot();
void handleCommand();
void handleStatus();
void handleNotFound();
String getFormattedTime();

void setup() {
  // CPU frekansini 80MHz'e dusurmek (guc tasarrufu ve dusuk isinma)
  system_update_cpu_freq(80);  // 80MHz (varsayilan 160MHz yerine)
  
  // USB Serial Monitor baslat
  Serial.begin(SERIAL_BAUD);
  delay(1000);
  
  // Arduino Mega ile SoftwareSerial baslat (9600 baud - KARARLI)
  arduinoSerial.begin(ARDUINO_BAUD);
  
  Serial.println("\n\n=== NodeMCU Data Logger ===");
  Serial.println("Arduino Mega'dan JSON verisi bekleniyor...");
  Serial.println("SoftwareSerial: D1(RX)=GPIO5, 9600 baud");
  
  Serial.println();
  Serial.println("WiFi'ye baglaniyor...");
  Serial.print("SSID: ");
  Serial.println(ssid);
  
  // WiFi baglantisini baslat
  WiFi.begin(ssid, password);
  
  // Baglanti kurulana kadar bekle
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  // Baglanti durumunu kontrol et
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi'ye basariyla baglandi!");
    Serial.print("IP Adresi: ");
    Serial.println(WiFi.localIP());
    Serial.print("Sinyal Gucu (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println();
    Serial.println("WiFi baglantisi basarisiz!");
    Serial.println("Lutfen SSID ve sifrenizi kontrol edin.");
  }

  Serial.println();
  Serial.println("=== MH-SD KART MODULU KONTROLU ===");
  Serial.println("Modul: MH-SD Card Module (SPI)");
  Serial.print("CS Pin: D8 (GPIO15)");
  Serial.println();
  Serial.println("Baglanti kontrolu:");
  Serial.println("  VCC -> 3.3V");
  Serial.println("  GND -> GND");
  Serial.println("  MISO -> D6 (GPIO12)");
  Serial.println("  MOSI -> D7 (GPIO13)");
  Serial.println("  SCK -> D5 (GPIO14)");
  Serial.println("  CS -> D8 (GPIO15)");
  Serial.println();
  
  // SPI'yi manuel baslat (MH-SD modul icin gerekli)
  Serial.println("SPI bus baslatiliyor...");
  SPI.begin();
  SPI.setFrequency(4000000);  // 4MHz (MH-SD icin guvenli hiz)
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);  // CS pinini HIGH yap (devre disi)
  delay(100);
  
  Serial.print("SD kart baslatiliyor... ");
  
  // SD kartı başlat (MH-SD modül için)
  sdCardReady = SD.begin(SD_CS);
  
  if (!sdCardReady) {
    Serial.println("BASARISIZ!");
    Serial.println("HATA: MH-SD modul baslatilamadi!");
    Serial.println();
    Serial.println("Kontrol listesi:");
    Serial.println("  1. SD kart modüle tam oturmus mu?");
    Serial.println("  2. SD kart FAT32 formatli mi?");
    Serial.println("  3. Kablolar dogru mu? (VCC=3.3V, GND=GND)");
    Serial.println("  4. SD kart 2GB-32GB arasi mi?");
    Serial.println("  5. SD kart bozuk olabilir, baska kart deneyin");
    Serial.println();
    Serial.println("NOT: SD kart olmadan sistem calismaya devam eder.");
    Serial.println("     Veriler sadece Firebase'e gonderilir.");
  } else {
    Serial.println("BASARILI!");
    Serial.println("MH-SD modul ve SD kart basariyla baslandi.");
    
    // Kart bilgilerini goster
    uint32_t cardSize = SD.size64() / (1024 * 1024);
    Serial.print("SD Kart Boyutu: ");
    Serial.print(cardSize);
    Serial.println(" MB");
    
    // Test dosyası olustur
    Serial.print("Test dosyasi olusturuluyor... ");
    File testFile = SD.open("/test.txt", FILE_WRITE);
    if (testFile) {
      testFile.println("MH-SD Modul Calisiyor!");
      testFile.print("Test Zamani: ");
      testFile.println(millis());
      testFile.close();
      Serial.println("BASARILI!");
      
      // Dosyanin gercekten var oldugunu dogrula
      if (SD.exists("/test.txt")) {
        Serial.println("Dogrulama: /test.txt basariyla olusturuldu");
        Serial.println("MH-SD modul yazma/okuma yapabiliyor.");
      } else {
        Serial.println("UYARI: Test dosyasi dogrulanamadi!");
        sdCardReady = false;
      }
    } else {
      Serial.println("BASARISIZ!");
      Serial.println("UYARI: SD kart okunamazsa da yazma basarisiz!");
      Serial.println("SD kart yazma korumali veya FAT32 degil olabilir.");
      sdCardReady = false;
    }
  }
  Serial.println("====================================");
  Serial.println();
  
  Serial.println("\nArduino Mega'dan JSON verisi bekleniyor...");
  Serial.println("-------------------------------------------");
  
  // NTP ile zaman senkronizasyonu
  Serial.println("\nNTP ile zaman senkronize ediliyor...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // Zaman senkronize olana kadar bekle
  int ntpRetry = 0;
  while (time(nullptr) < 100000 && ntpRetry < 20) {
    delay(500);
    Serial.print(".");
    ntpRetry++;
  }
  
  if (time(nullptr) > 100000) {
    Serial.println("\nNTP zamani basariyla alindi!");
    Serial.print("Suan: ");
    Serial.println(getFormattedTime());
  } else {
    Serial.println("\nUYARI: NTP zamani alinamadi, sistem zamani kullanilacak");
  }
  
  // Web Server rotalarini tanimla
  server.on("/", handleRoot);
  server.on("/command", handleCommand);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);
  
  // Web Server'i baslat
  server.begin();
  Serial.println("\nWeb Server baslatildi!");
  Serial.print("Kontrol Paneli: http://");
  Serial.println(WiFi.localIP());
  Serial.println("\nKomutlar (Serial Monitor veya Web):");
  Serial.println("  havaac, havakapa, isikac, isikkapa, sulaac, sulakapa");
  
  // Firebase baslat
  initFirebase();
  
  // Baslangiçta SD'deki eksik paketleri Firebase'e yukle
  Serial.println("\nOffline paketler kontrol ediliyor...");
  syncSDtoFirebase();
}

void loop() {
  // Web Server isteklerini isle
  server.handleClient();
  
  // WiFi baglantisi koptugunda yeniden baglan (her 10 saniyede kontrol et)
  static unsigned long lastWiFiCheck = 0;
  static bool wasConnected = true;
  
  if (millis() - lastWiFiCheck > 10000) {  // 10 saniye
    lastWiFiCheck = millis();
    bool isConnected = (WiFi.status() == WL_CONNECTED);
  
    if (!isConnected && wasConnected) {
      // WiFi yeni koptu
      Serial.println("\n!!! WiFi baglantisi koptu !!!");
      Serial.println("SD'ye kayit devam ediyor...");
    } else if (isConnected && !wasConnected) {
      // WiFi yeni baglandi
      Serial.println("\n!!! WiFi yeniden baglandi !!!");
      WiFi.reconnect();
      delay(2000);
      
      // Offline paketleri senkronize et
      Serial.println("Offline paketler Firebase'e yukleniyor...");
      syncSDtoFirebase();
    }
    
    wasConnected = isConnected;
  }

  // USB Serial Monitor'den komut oku (test icin)
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
  
  // Firebase'den komut kontrol et (her 3 saniyede bir)
  static unsigned long lastCommandCheck = 0;
  if (millis() - lastCommandCheck > 3000) {  // 3 saniye
    lastCommandCheck = millis();
    checkFirebaseCommands();
  }
  
  // Firebase /kontrol durumlarini kontrol et (her 2 saniyede bir)
  static unsigned long lastControlCheck = 0;
  if (millis() - lastControlCheck > 2000) {  // 2 saniye
    lastControlCheck = millis();
    checkFirebaseControl();
  }

  // Arduino Mega'dan (SoftwareSerial) gelen JSON'u oku
  // Maksimum 100 karakter oku (CPU'yu asiri zorlama)
  int readCount = 0;
  while (arduinoSerial.available() && readCount < 100) {
    char c = (char)arduinoSerial.read();
    readCount++;
    
    // JSON baslangici
    if (c == '{') {
      jsonBuffer = "{";
      jsonComplete = false;
    }
    // JSON bitisi
    else if (c == '}') {
      jsonBuffer += '}';
      jsonComplete = true;
      break;  // JSON tamamlandi, cik
    }
    // JSON icerigi
    else if (jsonBuffer.length() > 0) {
      jsonBuffer += c;
    }
    
    yield();  // ESP8266'ya nefes alma suresi ver
  }

  // Tam JSON alindiysa isle
  if (jsonComplete) {
    Serial.println("\n>>> ARDUINO'DAN JSON ALINDI <<<");
    Serial.print("Boyut: ");
    Serial.print(jsonBuffer.length());
    Serial.println(" byte");
    Serial.println("JSON:");
    Serial.println(jsonBuffer);
    
    // Son veriyi sakla (Web interface icin)
    lastSensorData = jsonBuffer;

    // Zaman damgasi al (Firebase ile ayni format)
    time_t now = time(nullptr);
    String timestamp;
    
    if (now > 100000) {
      struct tm* timeinfo = localtime(&now);
      char buffer[32];
      // Format: 2024-11-27_14-30-45
      strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", timeinfo);
      timestamp = String(buffer);
    } else {
      timestamp = "millis_" + String(millis());
    }
    
    // SD'ye her paket ayri dosya olarak kaydet
    String filename = "/" + timestamp + ".json";
    
    // SD kart hazirsa kaydet
    if (!sdCardReady) {
      Serial.println("[SD] UYARI: SD kart baslatilmamis, dosya kaydedilemiyor.");
      Serial.println("  -> Veri sadece Firebase'e gonderilecek.");
    } else {
      Serial.print("[SD] Dosya aciliyor: ");
      Serial.println(filename);
      
      File log = SD.open(filename.c_str(), FILE_WRITE);
      if (log) {
        size_t bytesWritten = log.println(jsonBuffer);
        log.close();
        
        Serial.print("[SD] Basariyla kaydedildi: ");
        Serial.print(filename);
        Serial.print(" (");
        Serial.print(bytesWritten);
        Serial.println(" byte)");
        
        // Dosyanin gercekten yazildigini dogrula
        File verify = SD.open(filename.c_str(), FILE_READ);
        if (verify) {
          Serial.print("[SD] Dogrulama: Dosya boyutu = ");
          Serial.print(verify.size());
          Serial.println(" byte");
          verify.close();
        } else {
          Serial.println("[SD] UYARI: Dosya dogrulanamadi!");
        }
      } else {
        Serial.print("[SD] HATA: ");
        Serial.print(filename);
        Serial.println(" acilamadi!");
        Serial.println("  -> Olasiliklar:");
        Serial.println("     1. SD kart dolu");
        Serial.println("     2. Dosya sistemi bozuk (FAT32 formatlayin)");
        Serial.println("     3. Yazma korumali");
        Serial.println("     4. SD kart baglantisi zayif");
        
        // SD karti yeniden baslat
        Serial.println("[SD] SD kart yeniden baslatiliyor...");
        SD.end();
        delay(100);
        sdCardReady = SD.begin(SD_CS);
        if (sdCardReady) {
          Serial.println("[SD] Yeniden baslatma BASARILI");
        } else {
          Serial.println("[SD] Yeniden baslatma BASARISIZ!");
        }
      }
    }
    
    // Firebase'e gonder (sadece WiFi varsa)
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("[Firebase] Veri gonderiliyor...");
      if (pushToFirebase(jsonBuffer)) {
        Serial.println("[Firebase] Veri basariyla gonderildi!");
      } else {
        Serial.println("[Firebase] Gonderim basarisiz!");
      }
    } else {
      Serial.println("[Firebase] WiFi yok, sadece SD'ye kaydedildi.");
      Serial.println("  -> Internet gelince otomatik yuklenecek.");
    }
    
    Serial.println(">>> ISLEM TAMAMLANDI <<<\n");
    
    // Buffer temizle
    jsonBuffer = "";
    jsonComplete = false;
  }

  // CPU'yu rahatlatmak icin bekleme
  delay(50);  // 50ms bekleme (isinmayi azaltir)
  yield();     // ESP8266 background islemleri icin
}

// ========================================
// ARDUINO'YA KOMUT GONDERME FONKSIYONU
// ========================================
void sendCommandToArduino(String command) {
  // Komutu Arduino'ya gonder (SoftwareSerial TX pin'i uzerinden)
  arduinoSerial.println(command);
  arduinoSerial.flush();  // Gonderimin tamamlanmasini bekle
  
  Serial.print("[TX Arduino] ");
  Serial.println(command);
}

// ========================================
// WEB SERVER FONKSIYONLARI
// ========================================

// Ana sayfa (Kontrol Paneli)
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Sera Kontrol Paneli</title>";
  html += "<style>";
  html += "body{font-family:Arial;margin:0;padding:20px;background:#f0f0f0}";
  html += ".container{max-width:800px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}";
  html += "h1{color:#333;text-align:center}";
  html += ".status{background:#e8f5e9;padding:15px;border-radius:5px;margin:20px 0}";
  html += ".controls{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin:20px 0}";
  html += "button{padding:15px;font-size:16px;border:none;border-radius:5px;cursor:pointer;transition:0.3s}";
  html += ".btn-on{background:#4CAF50;color:white}.btn-on:hover{background:#45a049}";
  html += ".btn-off{background:#f44336;color:white}.btn-off:hover{background:#da190b}";
  html += ".btn-water{background:#2196F3;color:white}.btn-water:hover{background:#0b7dda}";
  html += ".info{color:#666;font-size:14px;margin-top:20px}";
  html += "</style>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>🌱 Sera Kontrol Paneli</h1>";
  html += "<div class='status' id='sensorData'>";
  html += "<strong>Son Sensor Verisi:</strong><br>";
  html += "<small id='updateTime'>" + getFormattedTime() + "</small><br>";
  html += "<pre style='overflow-x:auto'>" + lastSensorData + "</pre>";
  html += "</div>";
  html += "<div class='controls'>";
  html += "<button class='btn-on' onclick=\"cmd('havaac')\">🌬️ Hava Aç</button>";
  html += "<button class='btn-off' onclick=\"cmd('havakapa')\">🔒 Hava Kapat</button>";
  html += "<button class='btn-on' onclick=\"cmd('isikac')\">💡 Işık Aç</button>";
  html += "<button class='btn-off' onclick=\"cmd('isikkapa')\">🌙 Işık Kapat</button>";
  html += "<button class='btn-water' onclick=\"cmd('sulaac')\">💧 Sulama Aç</button>";
  html += "<button class='btn-off' onclick=\"cmd('sulakapa')\">🛑 Sulama Kapat</button>";
  html += "</div>";
  html += "<div class='info'>";
  html += "<strong>IP Adresi:</strong> " + WiFi.localIP().toString() + "<br>";
  html += "<strong>RSSI:</strong> " + String(WiFi.RSSI()) + " dBm<br>";
  html += "<strong>Uptime:</strong> " + String(millis()/1000) + " saniye";
  html += "</div>";
  html += "</div>";
  html += "<script>";
  html += "function cmd(c){fetch('/command?cmd='+c).then(r=>r.text()).then(d=>{alert(d);updateStatus()})}";
  html += "function updateStatus(){fetch('/status').then(r=>r.json()).then(d=>{";
  html += "document.getElementById('sensorData').innerHTML='<strong>Son Sensor Verisi:</strong><br><small>'+new Date().toLocaleString('tr-TR')+'</small><br><pre style=\"overflow-x:auto\">'+d.lastData+'</pre>';";
  html += "})}";
  html += "setInterval(updateStatus,3000);";
  html += "updateStatus();";
  html += "</script>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// Komut isleme endpoint
void handleCommand() {
  if (server.hasArg("cmd")) {
    String cmd = server.arg("cmd");
    cmd.toLowerCase();
    
    // Komut listesi kontrolu
    if (cmd == "havaac" || cmd == "havakapa" || cmd == "isikac" || 
        cmd == "isikkapa" || cmd == "sulaac" || cmd == "sulakapa") {
      
      sendCommandToArduino(cmd);
      server.send(200, "text/plain", "Komut gonderildi: " + cmd);
    } else {
      server.send(400, "text/plain", "Gecersiz komut: " + cmd);
    }
  } else {
    server.send(400, "text/plain", "Komut parametresi eksik!");
  }
}

// Durum sorgulama endpoint
void handleStatus() {
  String json = "{";
  json += "\"uptime\":" + String(millis()/1000) + ",";
  json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  json += "\"lastData\":\"" + lastSensorData + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
}

// 404 Hata sayfasi
void handleNotFound() {
  server.send(404, "text/plain", "404: Sayfa bulunamadi!");
}

// ========================================
// NTP ZAMAN DAMGASI FONKSIYONU
// ========================================
String getFormattedTime() {
  time_t now = time(nullptr);
  
  // Zaman senkronize olmamışsa sistem zamanı kullan
  if (now < 100000) {
    return String(millis() / 1000) + "s (NTP yok)";
  }
  
  struct tm* timeinfo = localtime(&now);
  
  char buffer[64];
  // Format: 2025-11-21 14:30:45
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
  
  return String(buffer);
}
