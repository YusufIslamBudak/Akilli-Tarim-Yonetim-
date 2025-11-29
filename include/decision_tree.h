#ifndef DECISION_TREE_H
#define DECISION_TREE_H

/**
 * @file decision_tree.h
 * @brief Fasulye Sera Karar Ağacı
 * 
 * Sensör verilerine göre otomatik sera kontrolü sağlar.
 * Fan, ışık, sulama sistemlerini optimize eder.
 * 
 * @author Yusuf Islam Budak
 * @date 2025
 */

#include <Arduino.h>

// ========================================
// SENSÖR VERİ YAPISI
// ========================================
struct SensorData {
    float temperature;      // Sıcaklık (°C)
    float humidity;         // Bağıl nem (%)
    float pressure;         // Basınç (hPa)
    float gasResistance;    // Gaz direnci (kOhm)
    float lux;              // Işık şiddeti (lux)
    int   co2;              // CO2 seviyesi (ppm)
    float soilMoisture;     // Toprak nemi (%)
    float dewPoint;         // Çiğ noktası (°C)
    float heatIndex;        // Hissedilen sıcaklık (°C)
    int   roofPosition;     // Çatı pozisyonu (0-100)
    bool  fanOn;            // Fan durumu
    bool  lightOn;          // Işık durumu
    bool  pumpOn;           // Pompa durumu
    unsigned long uptime;   // Çalışma süresi (sn)
    unsigned long timestamp; // Veri zamanı
};

// ========================================
// KARAR AĞACI DURUM YAPISI
// ========================================
struct DecisionStatus {
    // Mevcut mod
    bool isNightMode;           // Gece modu aktif mi
    bool isDayMode;             // Gündüz modu aktif mi
    int currentHour;            // Şu anki saat
    
    // Son alınan karar
    String lastDecisionCode;    // Son karar kodu (KOD-1, KOD-2, vs)
    String lastDecisionDesc;    // Son karar açıklaması
    unsigned long lastDecisionTime; // Son karar zamanı
    
    // Parametre durumları
    String tempStatus;          // "OPTIMAL", "SOGUK", "SICAK", "KRITIK"
    String humStatus;           // "OPTIMAL", "DUSUK", "YUKSEK", "KUF_RISKI"
    String co2Status;           // "OPTIMAL", "DUSUK", "YUKSEK", "TEHLIKELI"
    String soilStatus;          // "OPTIMAL", "KURU", "ISLAK", "ACIL"
    String pressureStatus;      // "NORMAL", "FIRTINA", "YUKSEK"
    String luxStatus;           // "KARANLIK", "BULUTLU", "YETERLI", "GUNESLI"
    
    // Önerilen aksiyonlar
    bool suggestFan;            // Fan açılmalı mı
    bool suggestLight;          // Işık açılmalı mı
    bool suggestPump;           // Pompa açılmalı mı
    
    // İstatistikler
    unsigned long totalDecisions;   // Toplam karar sayısı
    unsigned long fanOnCount;       // Fan açma sayısı
    unsigned long lightOnCount;     // Işık açma sayısı
    unsigned long pumpOnCount;      // Pompa açma sayısı
};

// ========================================
// FONKSİYON PROTOTIPLERI
// ========================================

/**
 * @brief Karar ağacını başlatır
 */
void initDecisionTree();

/**
 * @brief JSON verisini SensorData yapısına parse eder
 * @param json Arduino'dan gelen JSON string
 * @return true: başarılı, false: parse hatası
 */
bool parseSensorData(const String& json);

/**
 * @brief Mevcut sensör verilerini döndürür
 * @return SensorData referansı
 */
SensorData& getCurrentSensors();

/**
 * @brief Karar ağacı durumunu döndürür
 * @return DecisionStatus referansı
 */
DecisionStatus& getDecisionStatus();

/**
 * @brief Ana karar ağacı fonksiyonu
 * Sensör verilerine göre aksiyon alır
 */
void makeDecision();

/**
 * @brief Belirli aralıklarla karar ağacını çağırır
 * Loop içinde çağrılmalı
 */
void runDecisionTreeIfNeeded();

/**
 * @brief Güvenli komut gönderimi (cooldown kontrolü ile)
 * @param command Gönderilecek komut
 * @param lastCommand Son gönderilen komut referansı
 * @param lastTime Son gönderim zamanı referansı
 */
void sendCommandSafe(const char* command, String& lastCommand, unsigned long& lastTime);

/**
 * @brief Karar ağacı durum bilgisini yazdırır
 */
void printDecisionStatus();

/**
 * @brief Karar ağacı durumunu JSON formatında döndürür
 * @return JSON string
 */
String getDecisionStatusJSON();

/**
 * @brief Firebase için genişletilmiş JSON oluşturur
 * @param sensorJson Orijinal sensör JSON'u
 * @return Karar ağacı bilgileri eklenmiş JSON
 */
String createExtendedJSON(const String& sensorJson);

#endif // DECISION_TREE_H
