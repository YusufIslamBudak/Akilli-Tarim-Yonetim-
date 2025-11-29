#include "serial_handler.h"
#include "config.h"

// SoftwareSerial nesnesi (global)
static SoftwareSerial* arduinoSerialPtr = nullptr;
static SoftwareSerial arduinoSerialInstance(SOFT_RX_PIN, SOFT_TX_PIN);

// JSON minimum beklenen boyut (bozuk veri tespiti için)
// Kısa key isimli JSON: ~180 byte
#define MIN_VALID_JSON_SIZE 150
#define MAX_JSON_SIZE 350

void initArduinoSerial(uint8_t rxPin, uint8_t txPin, unsigned long baudRate) {
    arduinoSerialPtr = &arduinoSerialInstance;
    arduinoSerialPtr->begin(baudRate);
    
    Serial.println("\n=== NodeMCU Data Logger ===");
    Serial.println("Arduino Mega'dan JSON verisi bekleniyor...");
    Serial.print("SoftwareSerial: D1(RX)=GPIO");
    Serial.print(rxPin);
    Serial.print(", ");
    Serial.print(baudRate);
    Serial.println(" baud");
}

void sendCommandToArduino(const String& command) {
    if (arduinoSerialPtr == nullptr) return;
    
    arduinoSerialPtr->println(command);
    arduinoSerialPtr->flush();
    
    Serial.print("[TX Arduino] ");
    Serial.println(command);
}

void readArduinoJSON(String& jsonBuffer, bool& jsonComplete, int maxRead) {
    if (arduinoSerialPtr == nullptr) return;
    
    int readCount = 0;
    while (arduinoSerialPtr->available() && readCount < maxRead) {
        char c = (char)arduinoSerialPtr->read();
        readCount++;
        
        // Sadece yazdırılabilir ASCII karakterleri kabul et (32-126 arası)
        // JSON için önemli karakterler: { } " : , . - 0-9 a-z A-Z
        if (c < 32 || c > 126) {
            // Bozuk karakter algılandı - buffer'ı temizle
            if (jsonBuffer.length() > 0) {
                Serial.println("[SERIAL] Bozuk karakter algilandi, buffer temizlendi");
                jsonBuffer = "";
                jsonComplete = false;
            }
            continue;
        }
        
        // JSON başlangıcı
        if (c == '{') {
            jsonBuffer = "{";
            jsonComplete = false;
        }
        // JSON bitişi
        else if (c == '}') {
            jsonBuffer += '}';
            jsonComplete = true;
            break;
        }
        // JSON içeriği
        else if (jsonBuffer.length() > 0) {
            // Buffer çok büyürse temizle (olası sonsuz döngü)
            if (jsonBuffer.length() > MAX_JSON_SIZE) {
                Serial.println("[SERIAL] Buffer overflow, temizleniyor");
                jsonBuffer = "";
                jsonComplete = false;
                continue;
            }
            jsonBuffer += c;
        }
        
        yield();
    }
}

SoftwareSerial& getArduinoSerial() {
    return arduinoSerialInstance;
}

bool isValidJSON(const String& json) {
    // Minimum boyut kontrolü
    if (json.length() < MIN_VALID_JSON_SIZE) {
        Serial.print("[JSON] Cok kisa: ");
        Serial.print(json.length());
        Serial.print(" < ");
        Serial.println(MIN_VALID_JSON_SIZE);
        return false;
    }
    
    // Temel yapı kontrolü - { ile başlayıp } ile bitmeli
    if (!json.startsWith("{") || !json.endsWith("}")) {
        Serial.println("[JSON] Gecersiz yapi (basi/sonu)");
        return false;
    }
    
    // Beklenen alanları kontrol et (kısa key isimleri)
    if (json.indexOf("\"temp\"") == -1 ||
        json.indexOf("\"hum\"") == -1 ||
        json.indexOf("\"pres\"") == -1 ||
        json.indexOf("\"uptime\"") == -1) {
        Serial.println("[JSON] Eksik alanlar");
        return false;
    }
    
    // Bozuk karakter kontrolü
    for (unsigned int i = 0; i < json.length(); i++) {
        char c = json.charAt(i);
        if (c < 32 || c > 126) {
            Serial.print("[JSON] Bozuk karakter pozisyon ");
            Serial.println(i);
            return false;
        }
    }
    
    return true;
}
