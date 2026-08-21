/*
 * ble_pairing.h — inicjalizacja BLE (Adafruit Bluefruit) i gest "zapomnij
 * parowanie" opisany w docs/06-architektura-i-json.md §7: urzadzenie
 * pamieta JEDNEGO hosta naraz (bonding "za darmo" z samej biblioteki);
 * przytrzymanie przycisku enkodera (SW) przez BLE_PAIRING_HOLD_MS czysci
 * bonding i wraca do advertisingu, zeby sparowac sie z nowym hostem. Bez
 * dostepu do fizycznego przycisku reset (uzytkownik to potwierdzil) to
 * JEDYNA droga do zmiany sparowanego hosta.
 */
#ifndef MAKROPAD_BLE_PAIRING_H
#define MAKROPAD_BLE_PAIRING_H

#include <Arduino.h>

// Jak dlugo trzeba trzymac SW enkodera, zeby zaliczyc to jako gest "zapomnij
// parowanie" zamiast zwyklego wcisniecia przypisanego makrem (docs/06 §7).
#define BLE_PAIRING_HOLD_MS 5000

void blePairingBegin();

// Wywolywane w kazdym przebiegu loop() - sprawdza dlugosc biezacego
// przytrzymania SW enkodera (przez key_scanner.h) i odpala gest, gdy
// przekroczy BLE_PAIRING_HOLD_MS (raz na przytrzymanie).
void blePairingKrok(unsigned long teraz);

#endif // MAKROPAD_BLE_PAIRING_H
