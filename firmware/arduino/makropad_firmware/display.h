/*
 * display.h — wyswietlacz OLED SSD1306 128x32: linia 1 = nazwa warstwy
 * ("n"), linia 2 = tekst statusu ("s") - zgodnie z docs/06 §3 i zmiana
 * slownictwa uzgodniona wczesniej w projekcie (juz nie "Warstwa: X", tylko
 * sama nazwa - patrz web/index.html, idleOled()).
 */
#ifndef MAKROPAD_DISPLAY_H
#define MAKROPAD_DISPLAY_H

#include <Arduino.h>

void displayBegin();

// Wywolywane w kazdym przebiegu loop() - rysuje tylko gdy config.h ustawil
// zmianaDoWyswietlenia i minelo okno throttlingu (jak OLED_ODSWIEZ_MS w
// makropad_test.ino), zeby nie zapychac magistrali I2C.
void displayKrok(unsigned long teraz);

// Krotki komunikat systemowy na 2. linii (np. "Czyszcze parowanie...") -
// uzywane przez ble_pairing.cpp. Wraca automatycznie do normalnego statusu
// warstwy po uplywie `czasTrwaniaMs` (modul sam sobie planuje kolejne
// odswiezenie - wywolujacy nie musi nic pilnowac).
void displayKomunikat(const char *tekst, unsigned long czasTrwaniaMs);

#endif // MAKROPAD_DISPLAY_H
