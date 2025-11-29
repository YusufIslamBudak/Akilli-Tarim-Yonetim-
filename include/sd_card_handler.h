#ifndef SD_CARD_HANDLER_H
#define SD_CARD_HANDLER_H

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

/**
 * @brief SD kart modülünü başlatır
 * @param csPin CS pin numarası
 * @param spiFrequency SPI frekansı (Hz)
 * @return true başarılı, false başarısız
 */
bool initSDCard(uint8_t csPin, uint32_t spiFrequency);

/**
 * @brief SD kart durumunu döndürür
 * @return true hazır, false hazır değil
 */
bool isSDCardReady();

/**
 * @brief SD kart durumunu ayarlar
 * @param ready durum
 */
void setSDCardReady(bool ready);

/**
 * @brief JSON verisini dosyaya kaydeder
 * @param filename Dosya adı (/ ile başlamalı)
 * @param jsonData JSON verisi
 * @return true başarılı, false başarısız
 */
bool saveJSONToSD(const String& filename, const String& jsonData);

/**
 * @brief SD kartı yeniden başlatır
 * @param csPin CS pin numarası
 * @return true başarılı, false başarısız
 */
bool restartSDCard(uint8_t csPin);

/**
 * @brief SD karttaki tüm .json dosyalarını listeler
 * @param callback Her dosya için çağrılacak fonksiyon (filename, content)
 */
void listJSONFiles(void (*callback)(const String& filename, const String& content));

#endif // SD_CARD_HANDLER_H
