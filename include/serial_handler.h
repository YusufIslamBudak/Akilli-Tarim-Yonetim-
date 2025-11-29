#ifndef SERIAL_HANDLER_H
#define SERIAL_HANDLER_H

#include <Arduino.h>
#include <SoftwareSerial.h>

/**
 * @brief Arduino Serial iletişimini başlatır
 * @param rxPin RX pin numarası
 * @param txPin TX pin numarası
 * @param baudRate Baud hızı
 */
void initArduinoSerial(uint8_t rxPin, uint8_t txPin, unsigned long baudRate);

/**
 * @brief Arduino'ya komut gönderir
 * @param command Gönderilecek komut
 */
void sendCommandToArduino(const String& command);

/**
 * @brief Arduino'dan gelen JSON verisini okur
 * @param jsonBuffer JSON buffer referansı
 * @param jsonComplete Tamamlanma durumu referansı
 * @param maxRead Maksimum okunacak karakter sayısı
 */
void readArduinoJSON(String& jsonBuffer, bool& jsonComplete, int maxRead);

/**
 * @brief SoftwareSerial nesnesine erişim sağlar
 * @return SoftwareSerial referansı
 */
SoftwareSerial& getArduinoSerial();

/**
 * @brief JSON verisinin geçerliliğini kontrol eder
 * @param json Kontrol edilecek JSON string
 * @return true: geçerli, false: geçersiz/bozuk
 */
bool isValidJSON(const String& json);

#endif // SERIAL_HANDLER_H
