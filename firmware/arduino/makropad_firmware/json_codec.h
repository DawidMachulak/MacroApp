/*
 * json_codec.h — (de)serializacja configu z/do JSON wg schematu w
 * docs/06-architektura-i-json.md §3 (krotkie klucze v/active/ly/n/s/k/enc/m/a/t/c/d/mn/mx).
 *
 * Wymaga biblioteki **ArduinoJson w wersji 6.x** (Benoit Blanchon) - patrz
 * firmware/README.md. Skladnia API (StaticJsonDocument/DynamicJsonDocument,
 * JsonObject, deserializeJson/serializeJson) jest specyficzna dla wersji 6;
 * przy ArduinoJson 7 nazwy typow dokumentow sie zmienily (jest jednolity
 * `JsonDocument`) - jesli masz zainstalowana 7.x, albo obniz wersje przez
 * Menedzera bibliotek, albo (drugi wybor) podmien tu i w serial_protocol.cpp
 * `StaticJsonDocument<N>`/`DynamicJsonDocument(N)` na `JsonDocument`.
 */
#ifndef MAKROPAD_JSON_CODEC_H
#define MAKROPAD_JSON_CODEC_H

#include <ArduinoJson.h>
#include "config.h"

// Deserializuje jedno przypisanie z JsonVariant (obiekt {m,a,d} albo JSON
// null -> wyczyscPrzypisanie). Zwraca false przy oczywiscie zlych danych
// (np. "a" nie jest tablica) - wywolujacy powinien wtedy potraktowac cale
// zadanie jako blad (patrz serial_protocol.cpp, komenda err).
bool jsonWczytajPrzypisanie(JsonVariantConst zrodlo, Przypisanie &p);

// Serializuje przypisanie do JsonVariant docelowego (ustawia go na JSON
// null, jesli !p.przypisany).
void jsonZapiszPrzypisanie(const Przypisanie &p, JsonVariant cel);

// Deserializuje jedna warstwe z JsonObject {n,s,k,enc}.
bool jsonWczytajWarstwe(JsonObjectConst zrodlo, Warstwa &w);

// Serializuje jedna warstwe do JsonObject.
void jsonZapiszWarstwe(const Warstwa &w, JsonObject cel);

#endif // MAKROPAD_JSON_CODEC_H
