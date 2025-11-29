#ifndef BEAN_CONDITIONS_H
#define BEAN_CONDITIONS_H

/**
 * @file bean_conditions.h
 * @brief Fasulye Sera Hedef Aralıkları
 * 
 * Bu dosyadaki değerleri değiştirince,
 * karar ağacı otomatik olarak yeni aralıklarla çalışacak.
 * 
 * @author Yusuf Islam Budak
 * @date 2025
 */

// ========================================
// SICAKLIK (°C)
// ========================================
// Gündüz: 18–30°C
// Gece:  12–18°C
// 35 ve üzeri: fotosentez durma / aşırı stres
// 12 altı: şiddetli soğuk stresi

const float BEAN_TEMP_DAY_MIN        = 18.0;
const float BEAN_TEMP_DAY_MAX        = 30.0;
const float BEAN_TEMP_NIGHT_MIN      = 12.0;
const float BEAN_TEMP_NIGHT_MAX      = 18.0;

const float BEAN_TEMP_MIN_IDEAL      = 18.0;  // genel ideal alt (gündüz)
const float BEAN_TEMP_MAX_IDEAL      = 30.0;  // genel ideal üst (gündüz)
const float BEAN_TEMP_CRITICAL_HOT   = 35.0;  // fotosentez durur
const float BEAN_TEMP_CRITICAL_COLD  = 12.0;  // çiçek dökülür

// ========================================
// NEM (%)
// ========================================
// İdeal: 55–75%
// <50-55% çok düşük (su kaybı, solgunluk)
// >75-80% çok yüksek (mantar, küf riski)

const float BEAN_HUM_MIN_IDEAL       = 55.0;
const float BEAN_HUM_MAX_IDEAL       = 75.0;
const float BEAN_HUM_MIN_RISK        = 50.0;  // bunun altı ciddi düşük
const float BEAN_HUM_MAX_RISK        = 80.0;  // bunun üstü ciddi yüksek (küf)

// ========================================
// TOPRAK NEMİ (%)
// ========================================
const float BEAN_SOIL_MIN_IDEAL      = 50.0;
const float BEAN_SOIL_MAX_IDEAL      = 70.0;
const float BEAN_SOIL_TOO_DRY        = 35.0;   // Altı = acil sulama
const float BEAN_SOIL_TOO_WET        = 85.0;   // Üstü = aşırı sulama

// ========================================
// BASINÇ (hPa)
// ========================================
// Normal atmosfer: 990-1020 hPa (sera konumuna göre değişir)
// < 985 hPa: Fırtına riski, bitki stresi
// > 1030 hPa: Yüksek basınç, genelde sorun yok
// NOT: 992 hPa normal bir değerdir!

const float PRESSURE_LOW_STORM       = 985.0;   // Fırtına riski eşiği
const float PRESSURE_HIGH_WET        = 1030.0;  // Çok yüksek basınç

// ========================================
// CO2 (ppm)
// ========================================
// Min 600: altında fotosentez yok
// Optimal: 700-1000 ppm
// Max 1200: üstünde fayda az
// 2000: zararlı
// 5000: ölümcül

const int   BEAN_CO2_MIN_PHOTOSYN    = 600;
const int   BEAN_CO2_MIN_IDEAL       = 700;
const int   BEAN_CO2_MAX_IDEAL       = 1000;
const int   BEAN_CO2_MAX_EFFECTIVE   = 1200;
const int   BEAN_CO2_HARMFUL         = 2000;
const int   BEAN_CO2_LETHAL          = 5000;

// Havalandırma eşikleri
const int   BEAN_CO2_VENTILATE_SOFT  = 1200;   // hafif havalandır
const int   BEAN_CO2_VENTILATE_HARD  = 2000;   // güçlü havalandır

// ========================================
// GAZ DİRENCİ (kOhm)
// ========================================
// 200+ : çok iyi, optimum fotosentez
// 100-200: iyi, normal sera ortamı
// 50-100: orta, hafif kuruma
// 10-50: kötü
// <10: fotosentez ve bitki sağlığı tehlikede

const float GAS_RESISTANCE_EXCELLENT = 200.0;
const float GAS_RESISTANCE_GOOD      = 100.0;
const float GAS_RESISTANCE_MODERATE  = 50.0;
const float GAS_RESISTANCE_BAD       = 10.0;

// ========================================
// IŞIK (lux)
// ========================================
// Sera içi sensör değerleri:
// Gece (ışıklar açık): ~175 lux
// Gece (ışıklar kapalı): 0-10 lux
// Gündüz bulutlu: 500-2000 lux
// Gündüz güneşli: 2000-10000+ lux

const float BEAN_LUX_DAY_MIN         = 1000.0;  // Gündüz yeterli ışık
const float BEAN_LUX_DAY_MAX         = 40000.0; // Güneşli tavan
const float BEAN_LUX_NIGHT_MAX       = 50.0;    // Gece eşiği (ışık kapalıyken)
const float BEAN_LUX_ARTIFICIAL      = 175.0;   // Yapay aydınlatma değeri
const float BEAN_LUX_CLOUDY          = 500.0;   // Bulutlu gündüz

// ========================================
// SAAT BAZLI GECE/GÜNDÜZ AYARLARI
// ========================================
// Işık sensörü yerine NTP saatine göre mod belirleme
const int HOUR_DAY_START             = 6;       // Gündüz başlangıcı (06:00)
const int HOUR_DAY_END               = 20;      // Gündüz bitişi (20:00)
const int HOUR_NIGHT_START           = 20;      // Gece başlangıcı (20:00)
const int HOUR_NIGHT_END             = 6;       // Gece bitişi (06:00)

// ========================================
// KARAR AĞACI ZAMANLAMA
// ========================================
const unsigned long DECISION_INTERVAL      = 10000;  // Karar ağacı çalışma aralığı (10sn)
const unsigned long COMMAND_COOLDOWN       = 30000;  // Aynı komut tekrar süresi (30sn)

#endif // BEAN_CONDITIONS_H
