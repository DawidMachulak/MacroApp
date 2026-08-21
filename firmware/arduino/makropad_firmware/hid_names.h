/*
 * hid_names.h — jedyne miejsce w firmware, ktore zna nazwy stalych z
 * bibliotek Adafruit TinyUSB / Bluefruit (HID_KEY_*, HID_USAGE_CONSUMER_*).
 *
 * WAZNE (przeczytaj, zanim zglosisz blad kompilacji): nie mam tu mozliwosci
 * skompilowania firmware (brak arduino-cli/pakietow plytek w tym
 * srodowisku) - nazwy stalych ponizej sa oparte na standardowym, stabilnym
 * ukladzie USB HID Usage Tables i na typowych nazwach uzywanych w
 * przykladach Adafruit TinyUSB, ale MOGA nieznacznie odbiegac od Twojej
 * zainstalowanej wersji biblioteki. Jesli kompilator zglosi
 * "'HID_KEY_XXX' was not declared" - to jest JEDYNY plik, ktory trzeba
 * poprawic (i hid_names.cpp nizej), reszta firmware nie zalezy od
 * konkretnych nazw stalych HID.
 *
 * Modyfikatory (MOD_*) i bitmaska konsumenckich klawiszy multimedialnych sa
 * zdefiniowane lokalnie wg standardu USB HID Boot Keyboard (niezmienny
 * ukladbitowy), zeby nie polegac na dokladnej pisowni odpowiednikow w
 * bibliotece.
 */
#ifndef MAKROPAD_HID_NAZWY_H
#define MAKROPAD_HID_NAZWY_H

#include <Arduino.h>

// Bitmaska bajtu modyfikatorow raportu HID klawiatury (USB HID Boot
// Keyboard, uklad stabilny/standardowy).
#define MOD_LCTRL  0x01
#define MOD_LSHIFT 0x02
#define MOD_LALT   0x04
#define MOD_LGUI   0x08
#define MOD_RCTRL  0x10
#define MOD_RSHIFT 0x20
#define MOD_RALT   0x40
#define MOD_RGUI   0x80

// Rozbija jeden token nazwy modyfikatora (np. "LCTRL") na bit maski MOD_*.
// Zwraca 0, jesli token nie jest modyfikatorem (czyli jest "wlasciwym"
// klawiszem koncowym kombinacji).
uint8_t modyfikatorZNazwy(const char *token);

// Rozbija token nazwy "wlasciwego" klawisza (np. "A", "F5", "ENTER",
// "MINUS") na kod HID_KEY_*. Zwraca 0 (HID "brak klawisza"), jesli nazwa
// jest nieznana - patrz uzycie w macro_engine.cpp (nieznany token jest po
// prostu pomijany, tak samo jak nieznanym tokenom mockup web/index.html
// tylko pokazuje ostrzezenie, ale nie blokuje zapisu).
uint8_t kodKlawiszaZNazwy(const char *token);

// Rozklada string kombinacji typu "LCTRL+LSHIFT+ESC" (bez spacji, separator
// "+") na bajt modyfikatorow + do 6 kodow klawiszy (limit standardowego
// raportu HID boot keyboard). Zwraca liczbe znalezionych kodow klawiszy
// (0-6). `kody` musi miec miejsce na co najmniej 6 elementow.
uint8_t rozbierzKombinacje(const char *kombinacja, uint8_t &modyfikatory, uint8_t kody[6]);

// Mapuje pojedynczy znak ASCII (do wpisywania tekstu, typ akcji "text") na
// modyfikator (0 albo MOD_LSHIFT) + kod HID_KEY_*. Zwraca false, jesli znak
// nie ma odpowiednika w podstawowym ukladzie US-QWERTY (np. znaki spoza
// ASCII 32-126 i '\t'/'\n') - taki znak jest wtedy pomijany przy wpisywaniu.
bool asciiNaHid(char znak, uint8_t &modyfikator, uint8_t &kod);

// Mapuje nazwe klawisza multimedialnego (VOL_UP, VOL_DOWN, MUTE,
// PLAY_PAUSE, NEXT_TRACK, PREV_TRACK - patrz docs/06 §3.2) na 16-bitowy kod
// USB HID Usage z tablicy "Consumer" (strona 0x0C). Zwraca 0, jesli nazwa
// nieznana.
uint16_t kodMediaZNazwy(const char *nazwa);

#endif // MAKROPAD_HID_NAZWY_H
