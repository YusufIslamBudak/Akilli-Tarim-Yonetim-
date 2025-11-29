#include "sd_card_handler.h"
#include "config.h"

// SD kart durumu (global bayrak)
static bool sdCardReady = false;

bool initSDCard(uint8_t csPin, uint32_t spiFrequency) {
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
    
    // SPI'yi manuel başlat
    Serial.println("SPI bus baslatiliyor...");
    SPI.begin();
    SPI.setFrequency(spiFrequency);
    pinMode(csPin, OUTPUT);
    digitalWrite(csPin, HIGH);
    delay(100);
    
    Serial.print("SD kart baslatiliyor... ");
    
    sdCardReady = SD.begin(csPin);
    
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
        
        // Kart bilgilerini göster
        uint32_t cardSize = SD.size64() / (1024 * 1024);
        Serial.print("SD Kart Boyutu: ");
        Serial.print(cardSize);
        Serial.println(" MB");
        
        // Test dosyası oluştur
        Serial.print("Test dosyasi olusturuluyor... ");
        File testFile = SD.open("/test.txt", FILE_WRITE);
        if (testFile) {
            testFile.println("MH-SD Modul Calisiyor!");
            testFile.print("Test Zamani: ");
            testFile.println(millis());
            testFile.close();
            Serial.println("BASARILI!");
            
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
    
    return sdCardReady;
}

bool isSDCardReady() {
    return sdCardReady;
}

void setSDCardReady(bool ready) {
    sdCardReady = ready;
}

bool saveJSONToSD(const String& filename, const String& jsonData) {
    if (!sdCardReady) {
        Serial.println("[SD] SD kart yok, atlaniyor.");
        return false;
    }
    
    // Dosyayı append modunda aç ("a" = append)
    File log = SD.open(filename.c_str(), "a");
    if (log) {
        size_t bytesWritten = log.println(jsonData);
        log.close();
        
        Serial.print("[SD] Kaydedildi: ");
        Serial.print(filename);
        Serial.print(" (+");
        Serial.print(bytesWritten);
        Serial.println(" byte)");
        
        return true;
    } else {
        Serial.println("[SD] Yazma hatasi!");
        return false;
    }
}

bool restartSDCard(uint8_t csPin) {
    Serial.println("[SD] SD kart yeniden baslatiliyor...");
    SD.end();
    delay(100);
    sdCardReady = SD.begin(csPin);
    
    if (sdCardReady) {
        Serial.println("[SD] Yeniden baslatma BASARILI");
    } else {
        Serial.println("[SD] Yeniden baslatma BASARISIZ!");
    }
    
    return sdCardReady;
}

void listJSONFiles(void (*callback)(const String& filename, const String& content)) {
    if (!sdCardReady) {
        Serial.println("[SD] SD kart hazir degil!");
        return;
    }
    
    File root = SD.open("/");
    if (!root) {
        Serial.println("[SD] Root dizin acilamadi!");
        return;
    }
    
    while (true) {
        File entry = root.openNextFile();
        if (!entry) break;
        
        String filename = String(entry.name());
        
        if (!entry.isDirectory() && filename.endsWith(".json")) {
            // Dosya içeriğini oku
            String content = "";
            while (entry.available()) {
                content += (char)entry.read();
            }
            content.trim();
            
            // Callback çağır
            callback(filename, content);
        }
        
        entry.close();
    }
    
    root.close();
}
