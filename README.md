# Akıllı Tarım Yönetim Sistemi

NodeMCU ESP8266 tabanlı akıllı sera yönetim sistemi.

## Özellikler

- 🌡️ Sıcaklık ve nem takibi
- 💧 Otomatik sulama kontrolü
- 💡 Aydınlatma kontrolü
- 🌬️ Havalandırma sistemi kontrolü
- 📊 Firebase gerçek zamanlı veri depolama
- 💾 SD kart ile offline veri saklama
- 🌐 Web tabanlı kontrol paneli
- 📱 Uzaktan kontrol (Firebase)

## Donanım

- NodeMCU ESP8266
- Arduino Mega
- DHT11/DHT22 sıcaklık-nem sensörü
- Toprak nem sensörü
- SD kart modülü (MH-SD)
- Röle modülleri

## Kurulum

1. PlatformIO IDE'yi yükleyin
2. Projeyi klonlayın
3. `platformio.ini` dosyasındaki ayarları kontrol edin
4. WiFi bilgilerinizi `main.cpp` dosyasında güncelleyin
5. Firebase ayarlarınızı `firebase_handler.h` dosyasında güncelleyin
6. Projeyi derleyin ve NodeMCU'ya yükleyin

## Kullanım

Web arayüzüne NodeMCU IP adresinden erişebilirsiniz:
- `http://<NodeMCU_IP>/` - Kontrol paneli
- `http://<NodeMCU_IP>/status` - Durum sorgulama

## Lisans

MIT License
