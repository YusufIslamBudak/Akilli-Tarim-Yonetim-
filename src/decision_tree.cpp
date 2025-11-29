/**
 * @file decision_tree.cpp
 * @brief Fasulye Sera Karar Ağacı Implementasyonu
 * 
 * @author Yusuf Islam Budak
 * @date 2025
 */

#include "decision_tree.h"
#include "bean_conditions.h"
#include "serial_handler.h"
#include "ntp_handler.h"
#include <time.h>

// ========================================
// GLOBAL DEĞİŞKENLER
// ========================================
static SensorData currentSensors = {0};
static DecisionStatus decisionStatus = {0};

// Son komut zamanları (tekrar önleme)
static String lastFanCommand = "";
static unsigned long lastFanCommandTime = 0;

static String lastLightCommand = "";
static unsigned long lastLightCommandTime = 0;

static String lastPumpCommand = "";
static unsigned long lastPumpCommandTime = 0;

// Karar ağacı zamanlayıcı
static unsigned long lastDecisionTime = 0;

// ========================================
// SAAT KONTROLÜ FONKSİYONLARI
// ========================================
static int getCurrentHour() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    if (timeinfo == nullptr) return -1;
    return timeinfo->tm_hour;
}

static bool isNightTime() {
    int hour = getCurrentHour();
    if (hour < 0) return false; // NTP henüz senkronize olmadıysa gündüz varsay
    
    // Gece: 20:00 - 06:00 arası
    return (hour >= HOUR_NIGHT_START || hour < HOUR_NIGHT_END);
}

static bool isDayTime() {
    int hour = getCurrentHour();
    if (hour < 0) return true; // NTP henüz senkronize olmadıysa gündüz varsay
    
    // Gündüz: 06:00 - 20:00 arası
    return (hour >= HOUR_DAY_START && hour < HOUR_DAY_END);
}

// ========================================
// JSON PARSE YARDIMCI FONKSİYONLARI
// ========================================
static float parseJsonFloat(const String& json, const char* key) {
    String searchKey = String("\"") + key + "\":";
    int keyIndex = json.indexOf(searchKey);
    if (keyIndex == -1) return 0.0;
    
    int valueStart = keyIndex + searchKey.length();
    int valueEnd = json.indexOf(",", valueStart);
    if (valueEnd == -1) valueEnd = json.indexOf("}", valueStart);
    
    String valueStr = json.substring(valueStart, valueEnd);
    valueStr.trim();
    return valueStr.toFloat();
}

static int parseJsonInt(const String& json, const char* key) {
    String searchKey = String("\"") + key + "\":";
    int keyIndex = json.indexOf(searchKey);
    if (keyIndex == -1) return 0;
    
    int valueStart = keyIndex + searchKey.length();
    int valueEnd = json.indexOf(",", valueStart);
    if (valueEnd == -1) valueEnd = json.indexOf("}", valueStart);
    
    String valueStr = json.substring(valueStart, valueEnd);
    valueStr.trim();
    return valueStr.toInt();
}

static bool parseJsonBool(const String& json, const char* key) {
    String searchKey = String("\"") + key + "\":";
    int keyIndex = json.indexOf(searchKey);
    if (keyIndex == -1) return false;
    
    int valueStart = keyIndex + searchKey.length();
    String remaining = json.substring(valueStart, valueStart + 10);
    remaining.trim();
    return remaining.startsWith("true");
}

// ========================================
// KARAR AĞACI FONKSİYONLARI
// ========================================

void initDecisionTree() {
    memset(&currentSensors, 0, sizeof(SensorData));
    lastDecisionTime = 0;
    
    Serial.println("\n[KARAR AGACI] Fasulye sera modu baslatildi");
    Serial.println("  Sicaklik: 18-30C (gun), 12-18C (gece)");
    Serial.println("  Nem: 55-75%");
    Serial.println("  CO2: 700-1000 ppm");
    Serial.println("  Toprak: 50-70%");
}

bool parseSensorData(const String& json) {
    // Kısa key isimleri ile parse et
    // Arduino'dan gelen format: {"temp":23.5,"hum":60,...}
    
    currentSensors.temperature   = parseJsonFloat(json, "temp");
    currentSensors.humidity      = parseJsonFloat(json, "hum");
    currentSensors.pressure      = parseJsonFloat(json, "pres");
    currentSensors.gasResistance = parseJsonFloat(json, "gas");
    currentSensors.lux           = parseJsonFloat(json, "lux");
    currentSensors.co2           = parseJsonInt(json, "co2");
    currentSensors.soilMoisture  = parseJsonFloat(json, "soil");
    currentSensors.dewPoint      = parseJsonFloat(json, "dew");
    currentSensors.heatIndex     = parseJsonFloat(json, "heat");
    currentSensors.roofPosition  = parseJsonInt(json, "roof");
    currentSensors.fanOn         = parseJsonBool(json, "fan");
    currentSensors.lightOn       = parseJsonBool(json, "light");
    currentSensors.pumpOn        = parseJsonBool(json, "pump");
    currentSensors.uptime        = parseJsonInt(json, "uptime");
    currentSensors.timestamp     = millis();
    
    // Temel doğrulama
    if (currentSensors.temperature == 0 && currentSensors.humidity == 0) {
        return false;
    }
    
    return true;
}

SensorData& getCurrentSensors() {
    return currentSensors;
}

DecisionStatus& getDecisionStatus() {
    return decisionStatus;
}

// ========================================
// DURUM DEĞERLENDİRME FONKSİYONLARI
// ========================================
static void updateParameterStatus() {
    float temp = currentSensors.temperature;
    float hum = currentSensors.humidity;
    int co2 = currentSensors.co2;
    float soil = currentSensors.soilMoisture;
    float pres = currentSensors.pressure;
    float lux = currentSensors.lux;
    
    // Sıcaklık durumu
    if (temp <= BEAN_TEMP_CRITICAL_COLD) {
        decisionStatus.tempStatus = "KRITIK_SOGUK";
    } else if (temp >= BEAN_TEMP_CRITICAL_HOT) {
        decisionStatus.tempStatus = "KRITIK_SICAK";
    } else if (temp < BEAN_TEMP_MIN_IDEAL) {
        decisionStatus.tempStatus = "SOGUK";
    } else if (temp > BEAN_TEMP_MAX_IDEAL) {
        decisionStatus.tempStatus = "SICAK";
    } else {
        decisionStatus.tempStatus = "OPTIMAL";
    }
    
    // Nem durumu
    if (hum >= BEAN_HUM_MAX_RISK) {
        decisionStatus.humStatus = "KUF_RISKI";
    } else if (hum > BEAN_HUM_MAX_IDEAL) {
        decisionStatus.humStatus = "YUKSEK";
    } else if (hum < BEAN_HUM_MIN_RISK) {
        decisionStatus.humStatus = "COK_DUSUK";
    } else if (hum < BEAN_HUM_MIN_IDEAL) {
        decisionStatus.humStatus = "DUSUK";
    } else {
        decisionStatus.humStatus = "OPTIMAL";
    }
    
    // CO2 durumu
    if (co2 >= BEAN_CO2_HARMFUL) {
        decisionStatus.co2Status = "TEHLIKELI";
    } else if (co2 > BEAN_CO2_MAX_IDEAL) {
        decisionStatus.co2Status = "YUKSEK";
    } else if (co2 < BEAN_CO2_MIN_PHOTOSYN) {
        decisionStatus.co2Status = "COK_DUSUK";
    } else if (co2 < BEAN_CO2_MIN_IDEAL) {
        decisionStatus.co2Status = "DUSUK";
    } else {
        decisionStatus.co2Status = "OPTIMAL";
    }
    
    // Toprak nemi durumu
    if (soil <= BEAN_SOIL_TOO_DRY) {
        decisionStatus.soilStatus = "ACIL_KURU";
    } else if (soil >= BEAN_SOIL_TOO_WET) {
        decisionStatus.soilStatus = "ASIRI_ISLAK";
    } else if (soil < BEAN_SOIL_MIN_IDEAL) {
        decisionStatus.soilStatus = "KURU";
    } else if (soil > BEAN_SOIL_MAX_IDEAL) {
        decisionStatus.soilStatus = "ISLAK";
    } else {
        decisionStatus.soilStatus = "OPTIMAL";
    }
    
    // Basınç durumu
    if (pres > 0 && pres < PRESSURE_LOW_STORM) {
        decisionStatus.pressureStatus = "FIRTINA_RISKI";
    } else if (pres > PRESSURE_HIGH_WET) {
        decisionStatus.pressureStatus = "COK_YUKSEK";
    } else {
        decisionStatus.pressureStatus = "NORMAL";
    }
    
    // Işık durumu
    if (lux <= BEAN_LUX_NIGHT_MAX) {
        decisionStatus.luxStatus = "KARANLIK";
    } else if (lux < BEAN_LUX_CLOUDY) {
        decisionStatus.luxStatus = "COK_DUSUK";
    } else if (lux < BEAN_LUX_DAY_MIN) {
        decisionStatus.luxStatus = "BULUTLU";
    } else if (lux > BEAN_LUX_DAY_MAX) {
        decisionStatus.luxStatus = "COK_GUNESLI";
    } else {
        decisionStatus.luxStatus = "YETERLI";
    }
    
    // Mod bilgisi
    decisionStatus.currentHour = getCurrentHour();
    decisionStatus.isNightMode = isNightTime();
    decisionStatus.isDayMode = isDayTime();
}

void sendCommandSafe(const char* command, String& lastCommand, unsigned long& lastTime) {
    unsigned long now = millis();
    
    // Aynı komut cooldown süresi içinde tekrar gönderilmez
    if (lastCommand == command && (now - lastTime) < COMMAND_COOLDOWN) {
        return;
    }
    
    // Komutu gönder
    sendCommandToArduino(command);
    lastCommand = command;
    lastTime = now;
}

void makeDecision() {
    Serial.println("\n========================================");
    Serial.println("KARAR AGACI (FASULYE SERA) CALISIYOR...");
    Serial.println("========================================");
    
    float temp = currentSensors.temperature;
    float hum  = currentSensors.humidity;
    float pres = currentSensors.pressure;
    int   co2  = currentSensors.co2;
    float soil = currentSensors.soilMoisture;
    float lux  = currentSensors.lux;
    float dew  = currentSensors.dewPoint;
    float gas  = currentSensors.gasResistance;
    
    // Durum özeti
    Serial.print("Temp: "); Serial.print(temp); Serial.println(" C");
    Serial.print("Hum:  "); Serial.print(hum); Serial.println(" %");
    Serial.print("CO2:  "); Serial.print(co2); Serial.println(" ppm");
    Serial.print("Soil: "); Serial.print(soil); Serial.println(" %");
    Serial.print("Lux:  "); Serial.println(lux);
    Serial.print("Pres: "); Serial.print(pres); Serial.println(" hPa");
    
    // Saat bazlı mod belirleme
    int currentHour = getCurrentHour();
    bool isNight = isNightTime();
    bool isDay = isDayTime();
    
    Serial.print("Saat: "); Serial.print(currentHour); 
    Serial.print(" - Mod: "); Serial.println(isNight ? "GECE" : "GUNDUZ");
    
    // Parametre durumlarını güncelle
    updateParameterStatus();
    decisionStatus.totalDecisions++;
    decisionStatus.lastDecisionTime = millis();
    
    // Varsayılan öneriler
    decisionStatus.suggestFan = false;
    decisionStatus.suggestLight = false;
    decisionStatus.suggestPump = false;
    
    // ---- 0) GEÇERSİZ VERİ KONTROLÜ ----
    if (temp == 0 && hum == 0 && co2 == 0 && soil == 0) {
        Serial.println("[UYARI] Sensor verileri dolu degil, karar atlaniyor.");
        decisionStatus.lastDecisionCode = "HATA";
        decisionStatus.lastDecisionDesc = "Sensor verileri bos";
        return;
    }

    // ================================
    // 1) ACİL DURUMLAR
    // ================================

    // 1.a) DONMA RİSKİ (< 12°C)
    if (temp <= BEAN_TEMP_CRITICAL_COLD || dew < 5.0) {
        Serial.println(">>> KOD-1: DONMA RISKI / COK SOGUK <<<");
        decisionStatus.lastDecisionCode = "KOD-1";
        decisionStatus.lastDecisionDesc = "Donma riski - Cok soguk";
        decisionStatus.suggestLight = true;
        
        // Hava akışını kapat, ısıyı koru
        if (currentSensors.fanOn) {
            sendCommandSafe("havakapa", lastFanCommand, lastFanCommandTime);
        }
        if (currentSensors.pumpOn) {
            sendCommandSafe("sulakapa", lastPumpCommand, lastPumpCommandTime);
        }
        // Işık ile ısıtma desteği
        if (!currentSensors.lightOn) {
            sendCommandSafe("isikac", lastLightCommand, lastLightCommandTime);
            decisionStatus.lightOnCount++;
        }
        Serial.println("========================================\n");
        return;
    }

    // 1.b) AŞIRI SICAK (>= 35°C) - Fotosentez durur!
    if (temp >= BEAN_TEMP_CRITICAL_HOT) {
        Serial.println(">>> KOD-2: ASIRI SICAK - FOTOSENTEZ TEHLIKEDE <<<");
        decisionStatus.lastDecisionCode = "KOD-2";
        decisionStatus.lastDecisionDesc = "Asiri sicak - Fotosentez tehlikede";
        decisionStatus.suggestFan = true;
        
        // Acil havalandırma
        if (!currentSensors.fanOn) {
            sendCommandSafe("havaac", lastFanCommand, lastFanCommandTime);
        }
        // Yapay ışığı kapat (ısı kaynağı)
        if (currentSensors.lightOn) {
            sendCommandSafe("isikkapa", lastLightCommand, lastLightCommandTime);
        }
        // Toprak çok kuruysa hafif sulama
        if (soil <= BEAN_SOIL_TOO_DRY && !currentSensors.pumpOn) {
            sendCommandSafe("sulaac", lastPumpCommand, lastPumpCommandTime);
        }
        Serial.println("========================================\n");
        return;
    }

    // 1.c) BASINÇ KAYNAKLI RİSKLER (Fırtına)
    if (pres > 0 && pres < PRESSURE_LOW_STORM) {
        Serial.println(">>> KOD-3: DUSUK BASINC - FIRTINA RISKI <<<");
        // Sulamayı destekle
        if (!currentSensors.pumpOn && soil < BEAN_SOIL_MIN_IDEAL) {
            sendCommandSafe("sulaac", lastPumpCommand, lastPumpCommandTime);
        }
        Serial.println("========================================\n");
        return;
    }

    if (pres > PRESSURE_HIGH_WET) {
        Serial.println(">>> KOD-3B: COK YUKSEK BASINC <<<");
        // Havalandır, sulamayı azalt
        if (!currentSensors.fanOn) {
            sendCommandSafe("havaac", lastFanCommand, lastFanCommandTime);
        }
        if (currentSensors.pumpOn) {
            sendCommandSafe("sulakapa", lastPumpCommand, lastPumpCommandTime);
        }
        Serial.println("========================================\n");
        return;
    }

    // ================================
    // 2) CO2 ODAKLI HAVALANDIRMA
    // ================================

    // CO2 > 2000 ppm: Zararlı seviye - Acil havalandır
    if (co2 > BEAN_CO2_VENTILATE_HARD && co2 < BEAN_CO2_LETHAL && temp > 15.0) {
        Serial.println(">>> KOD-4: YUKSEK CO2 - ACIL HAVALANDIR <<<");
        if (!currentSensors.fanOn) {
            sendCommandSafe("havaac", lastFanCommand, lastFanCommandTime);
        }
        Serial.println("========================================\n");
        return;
    }

    // CO2 > 1200 ppm: Hafif havalandır
    if (co2 > BEAN_CO2_VENTILATE_SOFT && co2 <= BEAN_CO2_VENTILATE_HARD && temp > 18.0) {
        Serial.println(">>> KOD-5: ORTA CO2 - HAFIF HAVALANDIR <<<");
        if (!currentSensors.fanOn) {
            sendCommandSafe("havaac", lastFanCommand, lastFanCommandTime);
        }
        Serial.println("========================================\n");
        return;
    }

    // CO2 < 600 ppm: Fotosentez zayıflıyor, havalandırmayı kapat
    if (co2 < BEAN_CO2_MIN_PHOTOSYN && co2 > 0) {
        Serial.println(">>> KOD-5B: DUSUK CO2 - HAVALANDIRMAYI AZALT <<<");
        if (currentSensors.fanOn && temp < BEAN_TEMP_MAX_IDEAL) {
            sendCommandSafe("havakapa", lastFanCommand, lastFanCommandTime);
        }
    }

    // ================================
    // 3) KÜF / MANTAR RİSKİ
    // ================================
    float dewDiff = temp - dew;
    if (hum >= BEAN_HUM_MAX_RISK && dewDiff < 2.5) {
        Serial.println(">>> KOD-6: KUF RISKI - HAVALANDIR & SULAMA DUR <<<");
        
        if (!currentSensors.fanOn) {
            sendCommandSafe("havaac", lastFanCommand, lastFanCommandTime);
        }
        if (currentSensors.pumpOn) {
            sendCommandSafe("sulakapa", lastPumpCommand, lastPumpCommandTime);
        }
        Serial.println("========================================\n");
        return;
    }

    // ================================
    // 4) GECE MODU (Saat bazlı: 20:00 - 06:00)
    // ================================
    if (isNight) {
        // Gece soğuk koruma
        if (temp < BEAN_TEMP_NIGHT_MIN) {
            Serial.println(">>> KOD-7: GECE - SOGUK KORUMA <<<");
            
            // Hava akışını kapat
            if (currentSensors.fanOn) {
                sendCommandSafe("havakapa", lastFanCommand, lastFanCommandTime);
            }
            // Işık ile ısıt
            if (!currentSensors.lightOn) {
                sendCommandSafe("isikac", lastLightCommand, lastLightCommandTime);
            }
            Serial.println("========================================\n");
            return;
        }

        // Gece optimal sıcaklık (12-18°C)
        if (temp >= BEAN_TEMP_NIGHT_MIN && temp <= BEAN_TEMP_NIGHT_MAX) {
            Serial.println(">>> GECE OPTIMAL - Sistem dinlenme modunda <<<");
            // Gece normalse ışık kapalı kalabilir (enerji tasarrufu)
            // Ama çok soğursa yukarıdaki blok ışığı açar
        }
        
        // Gece sıcak ise havalandır
        if (temp > BEAN_TEMP_NIGHT_MAX + 5) {
            Serial.println(">>> GECE SICAK - Hafif havalandirma <<<");
            if (!currentSensors.fanOn) {
                sendCommandSafe("havaac", lastFanCommand, lastFanCommandTime);
            }
        }
    }

    // ================================
    // 5) GÜNDÜZ MODU (Saat bazlı: 06:00 - 20:00)
    // ================================
    if (isDay) {
        // Işık yetersizse yapay aydınlatma
        if (lux < BEAN_LUX_CLOUDY) {
            Serial.println(">>> GUNDUZ BULUTLU - Isik destegi <<<");
            if (!currentSensors.lightOn) {
                sendCommandSafe("isikac", lastLightCommand, lastLightCommandTime);
            }
        }
        // Güneş yeterliyse yapay ışığı kapat
        else if (lux > BEAN_LUX_DAY_MIN && currentSensors.lightOn) {
            Serial.println(">>> GUNDUZ GUNESLI - Yapay isik kapatiliyor <<<");
            sendCommandSafe("isikkapa", lastLightCommand, lastLightCommandTime);
        }
        
        // Gündüz normal koşullar
        if (temp >= BEAN_TEMP_MIN_IDEAL && temp <= BEAN_TEMP_MAX_IDEAL && 
            co2 >= BEAN_CO2_MIN_IDEAL && co2 <= BEAN_CO2_MAX_IDEAL) {
            
            Serial.println(">>> KOD-8: GUNDUZ NORMAL <<<");
            
            // Havalandırma kontrolü
            if (!currentSensors.fanOn && (temp > 25.0 || hum > 70.0)) {
                sendCommandSafe("havaac", lastFanCommand, lastFanCommandTime);
            }
        }
    }

    // ================================
    // 6) SULAMA LOJİĞİ
    // ================================

    // Aşırı sulama koruması
    if (soil >= BEAN_SOIL_TOO_WET) {
        if (currentSensors.pumpOn) {
            Serial.println(">>> SULAMA-1: ASIRI ISLAK - POMPA KAPAT <<<");
            sendCommandSafe("sulakapa", lastPumpCommand, lastPumpCommandTime);
        }
        // Kurutmak için havalandır
        if (!currentSensors.fanOn) {
            sendCommandSafe("havaac", lastFanCommand, lastFanCommandTime);
        }
        Serial.println("========================================\n");
        return;
    }

    // Çok kuru – acil sulama
    if (soil <= BEAN_SOIL_TOO_DRY && temp > 15.0) {
        Serial.println(">>> SULAMA-2: COK KURU - ACIL SULAMA <<<");
        if (!currentSensors.pumpOn) {
            sendCommandSafe("sulaac", lastPumpCommand, lastPumpCommandTime);
        }
        Serial.println("========================================\n");
        return;
    }

    // Gündüz normal sulama
    if (soil < BEAN_SOIL_MIN_IDEAL && soil > BEAN_SOIL_TOO_DRY &&
        temp >= 20.0 && temp <= 30.0 && lux > 1000.0) {
        
        Serial.println(">>> SULAMA-3: GUNDUZ NORMAL SULAMA <<<");
        if (!currentSensors.pumpOn) {
            sendCommandSafe("sulaac", lastPumpCommand, lastPumpCommandTime);
        }
        Serial.println("========================================\n");
        return;
    }

    // Toprak ideal aralıkta → pompa kapat
    if (soil >= BEAN_SOIL_MIN_IDEAL && soil <= BEAN_SOIL_MAX_IDEAL && currentSensors.pumpOn) {
        Serial.println(">>> SULAMA-4: TOPRAK IDEAL - POMPA KAPAT <<<");
        sendCommandSafe("sulakapa", lastPumpCommand, lastPumpCommandTime);
    }

    // ================================
    // 7) OPTİMAL DURUM & TASARRUF
    // ================================
    bool tempOk = (temp >= BEAN_TEMP_MIN_IDEAL && temp <= BEAN_TEMP_MAX_IDEAL);
    bool humOk  = (hum  >= BEAN_HUM_MIN_IDEAL  && hum  <= BEAN_HUM_MAX_IDEAL);
    bool co2Ok  = (co2  >= BEAN_CO2_MIN_IDEAL  && co2 <= BEAN_CO2_MAX_IDEAL);
    bool soilOk = (soil >= BEAN_SOIL_MIN_IDEAL && soil <= BEAN_SOIL_MAX_IDEAL);

    if (tempOk && humOk && co2Ok && soilOk) {
        Serial.println(">>> KOD-9: FASULYE ICIN OPTIMAL KOSULLAR <<<");
        Serial.println("Tum parametreler ideal aralikta!");
        
        // Enerji tasarrufu - gereksiz sistemleri kapat
        if (currentSensors.fanOn && co2 < 800 && temp < 24.0 && hum < 70.0) {
            sendCommandSafe("havakapa", lastFanCommand, lastFanCommandTime);
        }
        if (currentSensors.pumpOn) {
            sendCommandSafe("sulakapa", lastPumpCommand, lastPumpCommandTime);
        }
        if (currentSensors.lightOn && lux > BEAN_LUX_DAY_MIN) {
            sendCommandSafe("isikkapa", lastLightCommand, lastLightCommandTime);
        }
        
        Serial.println("Sistem enerji tasarrufu modunda.");
    }

    // ================================
    // 8) GAZ DİRENCİ KONTROLÜ
    // ================================
    if (gas > 0 && gas < GAS_RESISTANCE_BAD) {
        Serial.println(">>> GAZ UYARI: Hava kalitesi cok kotu! <<<");
        if (!currentSensors.fanOn) {
            sendCommandSafe("havaac", lastFanCommand, lastFanCommandTime);
        }
    }

    Serial.println("========================================\n");
}

void runDecisionTreeIfNeeded() {
    unsigned long now = millis();
    
    if (now - lastDecisionTime >= DECISION_INTERVAL) {
        lastDecisionTime = now;
        
        // Sensör verisi varsa karar ver
        if (currentSensors.timestamp > 0) {
            makeDecision();
        }
    }
}

void printDecisionStatus() {
    Serial.println("\n--- KARAR AGACI DURUMU ---");
    Serial.print("Son karar: ");
    Serial.print((millis() - lastDecisionTime) / 1000);
    Serial.println(" sn once");
    
    Serial.print("Sensor verisi yasi: ");
    if (currentSensors.timestamp > 0) {
        Serial.print((millis() - currentSensors.timestamp) / 1000);
        Serial.println(" sn");
    } else {
        Serial.println("YOK");
    }
    
    Serial.print("Fan: "); Serial.println(currentSensors.fanOn ? "ACIK" : "KAPALI");
    Serial.print("Isik: "); Serial.println(currentSensors.lightOn ? "ACIK" : "KAPALI");
    Serial.print("Pompa: "); Serial.println(currentSensors.pumpOn ? "ACIK" : "KAPALI");
    Serial.println("--------------------------\n");
}

// ========================================
// JSON FONKSİYONLARI
// ========================================

String getDecisionStatusJSON() {
    String json = "{";
    
    // Mod bilgisi
    json += "\"mode\":\"" + String(decisionStatus.isNightMode ? "GECE" : "GUNDUZ") + "\",";
    json += "\"hour\":" + String(decisionStatus.currentHour) + ",";
    
    // Son karar
    json += "\"lastCode\":\"" + decisionStatus.lastDecisionCode + "\",";
    json += "\"lastDesc\":\"" + decisionStatus.lastDecisionDesc + "\",";
    json += "\"lastDecisionAge\":" + String((millis() - decisionStatus.lastDecisionTime) / 1000) + ",";
    
    // Parametre durumları
    json += "\"tempStatus\":\"" + decisionStatus.tempStatus + "\",";
    json += "\"humStatus\":\"" + decisionStatus.humStatus + "\",";
    json += "\"co2Status\":\"" + decisionStatus.co2Status + "\",";
    json += "\"soilStatus\":\"" + decisionStatus.soilStatus + "\",";
    json += "\"pressureStatus\":\"" + decisionStatus.pressureStatus + "\",";
    json += "\"luxStatus\":\"" + decisionStatus.luxStatus + "\",";
    
    // Öneriler
    json += "\"suggestFan\":" + String(decisionStatus.suggestFan ? "true" : "false") + ",";
    json += "\"suggestLight\":" + String(decisionStatus.suggestLight ? "true" : "false") + ",";
    json += "\"suggestPump\":" + String(decisionStatus.suggestPump ? "true" : "false") + ",";
    
    // İstatistikler
    json += "\"totalDecisions\":" + String(decisionStatus.totalDecisions) + ",";
    json += "\"fanOnCount\":" + String(decisionStatus.fanOnCount) + ",";
    json += "\"lightOnCount\":" + String(decisionStatus.lightOnCount) + ",";
    json += "\"pumpOnCount\":" + String(decisionStatus.pumpOnCount);
    
    json += "}";
    return json;
}

String createExtendedJSON(const String& sensorJson) {
    // Orijinal JSON'un son '}' karakterini kaldır
    String extended = sensorJson;
    if (extended.endsWith("}")) {
        extended = extended.substring(0, extended.length() - 1);
    }
    
    // Karar ağacı bilgilerini ekle
    extended += ",\"decision\":{";
    extended += "\"mode\":\"" + String(decisionStatus.isNightMode ? "GECE" : "GUNDUZ") + "\",";
    extended += "\"hour\":" + String(decisionStatus.currentHour) + ",";
    extended += "\"code\":\"" + decisionStatus.lastDecisionCode + "\",";
    extended += "\"desc\":\"" + decisionStatus.lastDecisionDesc + "\",";
    extended += "\"tempSt\":\"" + decisionStatus.tempStatus + "\",";
    extended += "\"humSt\":\"" + decisionStatus.humStatus + "\",";
    extended += "\"co2St\":\"" + decisionStatus.co2Status + "\",";
    extended += "\"soilSt\":\"" + decisionStatus.soilStatus + "\",";
    extended += "\"presSt\":\"" + decisionStatus.pressureStatus + "\",";
    extended += "\"luxSt\":\"" + decisionStatus.luxStatus + "\"";
    extended += "}}";
    
    return extended;
}
