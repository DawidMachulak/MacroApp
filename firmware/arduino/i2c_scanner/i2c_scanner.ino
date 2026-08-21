/*
 * Skaner magistrali I2C - krok 14 instrukcji.
 *
 * Plytka: Seeed XIAO nRF52840
 * Monitor portu szeregowego: 115200
 *
 * Oczekiwany wynik: dokladnie dwa adresy
 *   0x3C - wyswietlacz OLED SSD1306
 *   0x20 - ekspander PCF8574 (po zwarciu A0, A1, A2 do GND)
 */
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Wire.begin();
  Serial.println("Szukam ukladow na I2C...");

  int znalezione = 0;
  for (byte adres = 1; adres < 127; adres++) {
    Wire.beginTransmission(adres);
    if (Wire.endTransmission() == 0) {
      Serial.print("Znalazlem uklad pod adresem 0x");
      Serial.println(adres, HEX);
      znalezione++;
    }
  }

  if (znalezione == 0)
    Serial.println("Nic nie znalazlem - sprawdz SDA/SCL i zasilanie modulow.");
  Serial.println("Koniec.");
}

void loop() {}
