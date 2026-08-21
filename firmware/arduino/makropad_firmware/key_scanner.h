/*
 * key_scanner.h — odczyt fizycznych wejsc (8x PCF8574 + 2x wprost, enkoder
 * obrotowy + przycisk) i podanie ich do macro_engine.
 *
 * Debounce i dekodowanie kwadratury sa bezposrednia kontynuacja wzorca juz
 * sprawdzonego w firmware/arduino/makropad_test/makropad_test.ino (patrz
 * docs/06 §5) - tu dochodzi tylko odrozniene POZIOMU od ZBOCZA (potrzebne
 * przez macro_engine, patrz jego naglowek) oraz synteza "klikniec" enkodera
 * (l/r) jako jednotaktowych impulsow.
 */
#ifndef MAKROPAD_KEY_SCANNER_H
#define MAKROPAD_KEY_SCANNER_H

#include <Arduino.h>

void keyScannerBegin();

// Wywolywane raz w kazdym przebiegu loop(). Aktualizuje wszystkie wejscia i
// przekazuje je do macroEngineKrok() dla kazdego z 13 slotow zgodnie z
// biezaco aktywna warstwa.
void keyScannerKrok(unsigned long teraz);

// Diagnostyka/dlugie przytrzymanie SW enkodera do gestu "zapomnij parowanie
// BLE" (ble_pairing.cpp, docs/06 §7). Zwraca 0, jesli przycisk nie jest
// aktualnie (stabilnie) wcisniety.
unsigned long enkoderPrzyciskCzasTrwaniaMs(unsigned long teraz);

// Wolane przez ble_pairing.cpp, gdy biezace przytrzymanie SW zostalo juz
// uznane za gest systemowy (dlugie przytrzymanie -> clearBonds) - od tego
// momentu, az do fizycznego puszczenia, key_scanner przestaje przekazywac
// wcisniecie SW do macro_engine (natychmiast zatrzymuje ewentualne
// aktywne makro trybu przytrzymaj/przelacz na tym slocie, patrz
// macro_engine.cpp "natychmiastowe zatrzymanie").
void zawiesMakroEnkoderaDoZwolnienia();

#endif // MAKROPAD_KEY_SCANNER_H
