/*
 * Makropad BLE — firmware docelowy (Krok 3 projektu).
 *
 * Plytka:     Seeed XIAO nRF52840
 * Biblioteki: PCF8574 (Rob Tillaart), Adafruit SSD1306, Adafruit GFX,
 *             ArduinoJson 6.x, Adafruit TinyUSB (wbudowana w pakiet plytek
 *             Seeed nRF52), Adafruit nRF52 BLE (Bluefruit, jw.),
 *             Adafruit LittleFS + InternalFileSystem (jw.)
 * Zobacz firmware/README.md po pelna liste krokow instalacji.
 *
 * ============================================================================
 * WAZNE — PRZECZYTAJ PRZED PIERWSZYM WGRANIEM
 * ============================================================================
 * To srodowisko (w ktorym powstal ten kod) nie ma dostepu do arduino-cli ani
 * do pakietow plytek/bibliotek Arduino - NIE dalo sie skompilowac tego
 * firmware ani razu. Cala logika bez zaleznosci od konkretnych bibliotek
 * (parsowanie JSON wg schematu, maszyna stanow trybow aktywacji w
 * macro_engine.cpp) zostala NAJPIERW zweryfikowana osobna symulacja w
 * Pythonie (opisana w komentarzu na gorze macro_engine.cpp) - w tym
 * symulacja zlapala realny blad projektowy przed przepisaniem na C++.
 * Natomiast pliki dotykajace bezposrednio bibliotek sprzetowych
 * (hid_output.cpp - USB/BLE HID, storage.cpp - LittleFS, ble_pairing.cpp -
 * Bluefruit) opieraja sie na oficjalnych przykladach Adafruit, ale MAJA
 * ponizsza pewnosc kompilacji:
 *
 *   json_codec.cpp    - wysoka (bardzo stabilne, standardowe API ArduinoJson 6)
 *   key_scanner.cpp    - wysoka (ten sam wzorzec co makropad_test.ino, ktory
 *                         juz dziala na tym sprzecie)
 *   display.cpp        - wysoka (jak makropad_test.ino + Adafruit_GFX::getTextBounds,
 *                         standardowa metoda tej biblioteki)
 *   storage.cpp         - srednia (Adafruit_LittleFS/InternalFS API)
 *   hid_output.cpp      - najnizsza (dokladne nazwy metod Adafruit_USBD_HID
 *                         i BLEHidAdafruit)
 *   ble_pairing.cpp     - srednia (standardowe API Bluefruit.Advertising, ale
 *                         nie zweryfikowane kompilacja)
 *
 * Kazdy z tych plikow ma na gorze wlasny komentarz z dokladnymi zalozeniami
 * i wskazowka, co poprawic, jesli kompilator sie poskarzy. Pierwsze wgranie
 * najlepiej zrobic z fizycznym dostepem do plytki i cierpliwoscia na
 * kilka rund drobnych poprawek nazw - to normalne przy kodzie pisanym bez
 * mozliwosci kompilacji, nie oznacza bledu w logice.
 * ============================================================================
 */
#include <Wire.h>
#include "config.h"
#include "json_codec.h"
#include "hid_names.h"
#include "key_scanner.h"
#include "macro_engine.h"
#include "hid_output.h"
#include "storage.h"
#include "ble_pairing.h"
#include "display.h"
#include "serial_protocol.h"

void setup() {
  Serial.begin(115200);
  Wire.begin();

  randomSeed(micros()); // wystarczajace dla humanizacji timingu makr (nie kryptografia)

  storageBegin();       // wczytuje config z flash (albo buduje domyslny) - PRZED reszta inicjalizacji
  keyScannerBegin();
  macroEngineBegin();
  hidBegin();            // USB HID
  blePairingBegin();     // Bluefruit.begin() + BLE HID + advertising (docs/06 §7)
  displayBegin();
  serialProtocolBegin();

  zmianaDoWyswietlenia = true; // wymus pierwsze rysowanie po starcie
}

void loop() {
  unsigned long teraz = millis();

  keyScannerKrok(teraz);     // odczyt wejsc + maszyna stanow makr (macro_engine) + wysylka HID
  blePairingKrok(teraz);     // gest "zapomnij parowanie" (dlugie przytrzymanie SW enkodera)
  serialProtocolKrok();      // protokol Web Serial (docs/06 §4) - niebblokujaco
  displayKrok(teraz);        // throttled odswiezanie OLED
  hidLoop();
}
