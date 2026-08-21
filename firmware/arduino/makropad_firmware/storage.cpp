/*
 * storage.cpp
 *
 * UWAGA (jak w hid_output.cpp - nie dalo sie tu skompilowac ani
 * zweryfikowac): zalozenia co do API `Adafruit_LittleFS`/`InternalFS`
 * (oparte na oficjalnym przykladzie Adafruit "internal_flash" dla nRF52):
 *
 *   #include <Adafruit_LittleFS.h>
 *   #include <InternalFileSystem.h>
 *   using namespace Adafruit_LittleFS_Namespace;
 *
 *   InternalFS.begin();
 *   InternalFS.exists("/config.json");
 *   InternalFS.remove("/config.json");
 *   File plik(InternalFS);
 *   plik.open("/config.json", FILE_O_WRITE);
 *   plik.write(bufor, dlugosc);
 *   plik.close();
 *   plik.open("/config.json", FILE_O_READ);
 *   plik.read(bufor, maxDlugosc);
 *
 * Jesli kompilator zglosi problem z ktoras z tych nazw - to jedyny plik do
 * poprawienia (sprawdz przyklady w Arduino IDE: Plik -> Przyklady ->
 * Adafruit LittleFS -> internal_flash, powinny byc zainstalowane razem z
 * pakietem plytek Seeed nRF52).
 */
#include "storage.h"
#include "json_codec.h"
#include "config.h"
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <ArduinoJson.h>

using namespace Adafruit_LittleFS_Namespace;

static const char *SCIEZKA_CONFIGU = "/config.json";

// 8 warstw x ~800 B (gorna granica z docs/06 §4) + naglowki JSON -
// zapas do 12 kB. Bufor jest lokalny/tymczasowy (na stosie/stercie tylko
// na czas save/load), nie trzymany na stale - nRF52840 ma 256 kB RAM.
#define STORAGE_BUFOR_MAX 12288

static bool wczytajZBufora(const char *json, size_t dlugosc) {
  DynamicJsonDocument doc(STORAGE_BUFOR_MAX);
  DeserializationError blad = deserializeJson(doc, json, dlugosc);
  if (blad) return false;

  JsonObjectConst root = doc.as<JsonObjectConst>();
  JsonVariantConst lyVar = root["ly"];
  if (!lyVar.is<JsonArrayConst>()) return false;
  JsonArrayConst ly = lyVar.as<JsonArrayConst>();
  if (ly.size() == 0) return false;

  // static, NIE na stosie: Konfiguracja to ~28 kB (8 warstw x ~3.5 kB) -
  // realny stos watku petli glownej na nRF52840 jest zwykle duzo mniejszy
  // niz to, wiec lokalna zmienna tego rozmiaru grozilaby przepelnieniem
  // stosu i cichym uszkodzeniem pamieci.
  static Konfiguracja nowy;
  nowy.wersja = root["v"] | MAKROPAD_WERSJA_FORMATU;
  nowy.liczbaWarstw = 0;

  for (JsonObjectConst warstwaJson : ly) {
    if (nowy.liczbaWarstw >= MAKROPAD_MAX_WARSTW) break;
    if (!jsonWczytajWarstwe(warstwaJson, nowy.warstwy[nowy.liczbaWarstw])) continue;
    nowy.liczbaWarstw++;
  }
  if (nowy.liczbaWarstw == 0) return false;

  int aktywna = root["active"] | 0;
  if (aktywna < 0 || aktywna >= nowy.liczbaWarstw) aktywna = 0;
  nowy.aktywnaWarstwa = (uint8_t)aktywna;

  config = nowy;
  return true;
}

bool storageBegin() {
  InternalFS.begin();

  if (!InternalFS.exists(SCIEZKA_CONFIGU)) {
    Serial.println("Brak zapisanego configu na flash - startuje z configiem domyslnym.");
    zbudujConfigDomyslny(config);
    return false;
  }

  File plik(InternalFS);
  if (!plik.open(SCIEZKA_CONFIGU, FILE_O_READ)) {
    Serial.println("Nie udalo sie otworzyc zapisanego configu - config domyslny.");
    zbudujConfigDomyslny(config);
    return false;
  }

  static char bufor[STORAGE_BUFOR_MAX]; // static: nie na stosie (za duze)
  int wczytano = plik.read(bufor, STORAGE_BUFOR_MAX - 1);
  plik.close();

  if (wczytano <= 0) {
    Serial.println("Zapisany config jest pusty/nieczytelny - config domyslny.");
    zbudujConfigDomyslny(config);
    return false;
  }
  bufor[wczytano] = '\0';

  if (!wczytajZBufora(bufor, (size_t)wczytano)) {
    Serial.println("Zapisany config nie przeszedl walidacji - config domyslny.");
    zbudujConfigDomyslny(config);
    return false;
  }

  Serial.println("Config wczytany z flash.");
  return true;
}

bool storageZapiszConfig() {
  DynamicJsonDocument doc(STORAGE_BUFOR_MAX);
  doc["v"] = config.wersja;
  doc["active"] = config.aktywnaWarstwa;
  JsonArray ly = doc.createNestedArray("ly");
  for (uint8_t i = 0; i < config.liczbaWarstw; i++) {
    JsonObject w = ly.createNestedObject();
    jsonZapiszWarstwe(config.warstwy[i], w);
  }

  static char bufor[STORAGE_BUFOR_MAX];
  size_t dlugosc = serializeJson(doc, bufor, STORAGE_BUFOR_MAX);
  if (dlugosc == 0) return false;

  if (InternalFS.exists(SCIEZKA_CONFIGU)) InternalFS.remove(SCIEZKA_CONFIGU);

  File plik(InternalFS);
  if (!plik.open(SCIEZKA_CONFIGU, FILE_O_WRITE)) return false;
  size_t zapisano = plik.write((const uint8_t *)bufor, dlugosc);
  plik.close();

  return zapisano == dlugosc;
}
