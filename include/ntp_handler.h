#ifndef NTP_HANDLER_H
#define NTP_HANDLER_H

#include <Arduino.h>
#include <time.h>

/**
 * @brief NTP zaman senkronizasyonunu baslatir
 * @param ntpServer NTP sunucu adresi
 * @param gmtOffset GMT offset (saniye)
 * @param daylightOffset Yaz saati offset (saniye)
 * @param timeout Maksimum deneme sayisi
 * @return true basarili, false basarisiz
 */
bool initNTP(const char* ntpServer, long gmtOffset, int daylightOffset, int timeout);

/**
 * @brief NTP zamaninin senkronize olup olmadigini kontrol eder
 * @return true senkronize, false senkronize degil
 */
bool isNTPSynced();

/**
 * @brief Formatli zaman damgasi dondurur (2025-11-21 14:30:45)
 * @return Formatli zaman string'i
 */
String getFormattedTime();

/**
 * @brief Dosya adi icin uygun zaman damgasi dondurur (2025-11-21_14-30-45)
 * @return Dosya adi uyumlu zaman string'i
 */
String getTimestampForFilename();

#endif // NTP_HANDLER_H
