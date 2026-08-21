/*
 * hid_output.h — wysylka raportow HID (klawiatura + multimedia) przez USB
 * (Adafruit TinyUSB) i BLE (Adafruit Bluefruit BLEHidAdafruit) rownolegle.
 *
 * Klawiatura wymaga agregacji: standardowy raport HID boot keyboard to
 * JEDEN bajt modyfikatorow + do 6 jednoczesnie wcisnietych klawiszy DLA
 * CALEGO URZADZENIA, nie osobno per klawisz fizyczny. Skoro kilka slotow
 * (makr) moze byc "w dole" jednoczesnie (np. dwa trzymane klawisze naraz),
 * kazdy slot zglasza tu tylko SWOJ wklad (hidUstawWkladKlawiatury /
 * hidWyczyscWkladKlawiatury), a modul sam sklada je w jeden raport i
 * wysyla przy kazdej zmianie. Bez tego zwolnienie jednego klawisza
 * nadpisywaloby (kasowalo) wciskniecie drugiego.
 *
 * Multimedia (Consumer Control) nie maja tego problemu w praktyce — realny
 * przypadek "dwa rozne klawisze multimedialne naraz" jest na tyle rzadki
 * (i prosty raport "Consumer Control" typowo i tak przenosi jeden aktywny
 * usage), ze kazdy slot wysyla wlasny raport niezaleznie.
 */
#ifndef MAKROPAD_HID_OUTPUT_H
#define MAKROPAD_HID_OUTPUT_H

#include <Arduino.h>
#include <bluefruit.h>

// Obiekt BLE HID (klawiatura + consumer control) - zdefiniowany w
// hid_output.cpp, ale ble_pairing.cpp potrzebuje go wprost przy budowaniu
// pakietu advertisingu (Bluefruit.Advertising.addService(blehid)).
extern BLEHidAdafruit blehid;

// Inicjalizuje USB HID (TinyUSB). Wolane raz w setup(), NIEZALEZNIE od BLE.
void hidBegin();

// Inicjalizuje BLE HID (blehid.begin()) - MUSI byc wolane PO Bluefruit.begin(),
// wiec ble_pairing.cpp woła to w odpowiednim momencie swojej inicjalizacji,
// zamiast hidBegin() robic to samemu (kolejnosc Bluefruit.begin() ->
// hidBeginBLE() -> Advertising.addService(blehid) -> Advertising.start()
// jest wazna).
void hidBeginBLE();

// Musi byc wywolywane w kazdym przebiegu loop() - obsluguje TinyUSB device task.
void hidLoop();

// --- klawiatura: wklad pojedynczego slotu do wspolnego raportu -------------
void hidUstawWkladKlawiatury(uint8_t slot, uint8_t modyfikatory, const uint8_t *kody, uint8_t liczbaKodow);
void hidWyczyscWkladKlawiatury(uint8_t slot);

// --- multimedia (Consumer Control), niezalezne per slot ---------------------
void hidWyslijMediaDol(uint16_t usage);
void hidWyslijMediaGora();

bool hidGotowyUSB();
bool hidGotowyBLE();

#endif // MAKROPAD_HID_OUTPUT_H
