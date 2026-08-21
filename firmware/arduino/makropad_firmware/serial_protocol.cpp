#include "serial_protocol.h"
#include "json_codec.h"
#include "storage.h"
#include "config.h"
#include <ArduinoJson.h>
#include <string.h>

// Budzet linii wejsciowej: docs/06 §4 szacuje jedna warstwe na ok.
// 400-800 bajtow zminifikowanego JSON - 1200 daje zapas.
#define SERIAL_BUFOR_MAX 1200

static char buforLinii[SERIAL_BUFOR_MAX];
static uint16_t dlugoscBufora = 0;

static void wyslijAck(bool ok) {
  StaticJsonDocument<64> doc;
  doc["cmd"] = "ack";
  doc["ok"] = ok;
  serializeJson(doc, Serial);
  Serial.println();
}

static void wyslijErr(const char *msg) {
  StaticJsonDocument<192> doc;
  doc["cmd"] = "err";
  doc["msg"] = msg;
  serializeJson(doc, Serial);
  Serial.println();
}

static void obslugaPing() {
  StaticJsonDocument<96> doc;
  doc["cmd"] = "pong";
  doc["fw"] = MAKROPAD_WERSJA_FIRMWARE;
  serializeJson(doc, Serial);
  Serial.println();
}

static void obslugaGetConfig() {
  // Wysylka warstwa-po-warstwie (docs/06 §4) - kazda ramka miesci sie
  // bezpiecznie w SERIAL_BUFOR_MAX nawet po stronie hosta.
  for (uint8_t i = 0; i < config.liczbaWarstw; i++) {
    StaticJsonDocument<1024> doc;
    doc["cmd"] = "layer";
    doc["idx"] = i;
    JsonObject data = doc.createNestedObject("data");
    jsonZapiszWarstwe(config.warstwy[i], data);
    serializeJson(doc, Serial);
    Serial.println();
  }
  StaticJsonDocument<32> koniec;
  koniec["cmd"] = "end";
  serializeJson(koniec, Serial);
  Serial.println();
}

static void obslugaSetLayer(JsonObjectConst msg) {
  int idx = msg["idx"] | -1;
  if (idx < 0 || idx >= MAKROPAD_MAX_WARSTW) {
    wyslijErr("set_layer: idx poza zakresem");
    return;
  }
  JsonObjectConst data = msg["data"].as<JsonObjectConst>();
  if (data.isNull()) {
    wyslijErr("set_layer: brak pola data");
    return;
  }
  if (!jsonWczytajWarstwe(data, config.warstwy[idx])) {
    wyslijErr("set_layer: niepoprawne dane warstwy");
    return;
  }
  if ((uint8_t)idx >= config.liczbaWarstw) {
    config.liczbaWarstw = idx + 1; // nowa warstwa dopisana na koncu
  }
  zmianaDoWyswietlenia = true; // gdyby to byla akurat warstwa aktywna
  wyslijAck(true);
}

static void obslugaSave() {
  if (storageZapiszConfig()) {
    wyslijAck(true);
  } else {
    wyslijErr("save: zapis do flash nie powiodl sie");
  }
}

static void obslugaSetActive(JsonObjectConst msg) {
  int idx = msg["idx"] | -1;
  if (idx < 0 || idx >= config.liczbaWarstw) {
    wyslijErr("set_active: idx poza zakresem");
    return;
  }
  ustawAktywnaWarstwe((uint8_t)idx);
  wyslijAck(true);
}

static void obsluzLinie(const char *linia) {
  if (linia[0] == '\0') return; // pusta linia (np. samo \r\n) - ignoruj po cichu

  StaticJsonDocument<1280> doc;
  DeserializationError blad = deserializeJson(doc, linia);
  if (blad) {
    wyslijErr("nie udalo sie sparsowac JSON");
    return;
  }
  JsonObjectConst msg = doc.as<JsonObjectConst>();
  const char *cmd = msg["cmd"] | "";

  if      (strcmp(cmd, "ping")       == 0) obslugaPing();
  else if (strcmp(cmd, "get_config") == 0) obslugaGetConfig();
  else if (strcmp(cmd, "set_layer")  == 0) obslugaSetLayer(msg);
  else if (strcmp(cmd, "save")       == 0) obslugaSave();
  else if (strcmp(cmd, "set_active") == 0) obslugaSetActive(msg);
  else wyslijErr("nieznana komenda");
}

void serialProtocolBegin() {
  dlugoscBufora = 0;
}

void serialProtocolKrok() {
  // Nieblokujaco: petla dziala TYLKO na tym, co juz jest w buforze UART w
  // tym przebiegu petli glownej - Serial.available()/read() nigdy nie
  // czekaja (docs/06 §5, "bez delay()").
  while (Serial.available() > 0) {
    char znak = (char)Serial.read();
    if (znak == '\n') {
      buforLinii[dlugoscBufora] = '\0';
      // \r ewentualnie zostawiony na koncu (line ending CRLF) nie przeszkadza
      // parserowi JSON (biale znaki na koncu sa ignorowane).
      obsluzLinie(buforLinii);
      dlugoscBufora = 0;
    } else if (dlugoscBufora < SERIAL_BUFOR_MAX - 1) {
      buforLinii[dlugoscBufora++] = znak;
    } else {
      // Przepelnienie (linia dluzsza niz oczekiwany budzet) - zrzuc bufor,
      // zeby nie sklejac dwoch ramek w jedna niepoprawna; kolejna linia
      // zacznie sie od nowa.
      dlugoscBufora = 0;
    }
  }
}
