/*
 * macro_engine.h — maszyna stanow trybow aktywacji (docs/06 §3.1, §3.3, §5).
 *
 * Algorytm ponizej byl NAJPIERW zweryfikowany symulacja w Pythonie
 * (zobacz notatke w macro_engine.cpp) - w tym symulacja zlapala realny blad
 * projektowy (tryb "nacisnij i pusc" startujacy od POZIOMU sygnalu zamiast
 * ZBOCZA, co przy trzymaniu klawisza odpalaloby sekwencje w kolko). Kod
 * ponizej jest wierna transkrypcja juz poprawionej i przetestowanej logiki.
 *
 * 13 "slotow" wykonawczych: 10 klawiszy (SLOT_KLAWISZ_0..9) + 3 akcje
 * enkodera (SLOT_ENC_L/R/B). Kazdy slot ma niezalezny stan, wiec kilka
 * makr moze dzialac jednoczesnie (np. trzymane dwa klawisze naraz).
 */
#ifndef MAKROPAD_MACRO_ENGINE_H
#define MAKROPAD_MACRO_ENGINE_H

#include <Arduino.h>
#include "config.h"

enum IndeksSlotu : uint8_t {
  SLOT_KLAWISZ_0 = 0,
  // ... SLOT_KLAWISZ_1..8 to wartosci 1..8, nieuzywane wprost po nazwie
  SLOT_ENC_L = 10,
  SLOT_ENC_R = 11,
  SLOT_ENC_B = 12,
  LICZBA_SLOTOW = 13
};

// Stan wykonania jednego przypisania (jeden na slot). Zdefiniowany w
// naglowku, zeby macro_engine.cpp i ewentualne przyszle diagnostyki mogly
// go czytac, ale wszystkie zmiany stanu przechodza przez funkcje ponizej.
struct StanPrzypisania {
  bool aktywne;                 // sekwencja w toku (patrz opis trybow w §3.1)
  const Przypisanie *p;         // wskaznik na przypisanie z warstwy aktywnej
                                 // w MOMENCIE STARTU sekwencji (patrz uwaga
                                 // w macro_engine.cpp o zmianie warstwy w trakcie)
  uint8_t indeksKroku;
  uint8_t indeksZnaku;          // dla AKCJA_TEKST — pozycja w c[]
  bool wcisniety;                // trwa "dol" biezacego kroku/znaku?
  unsigned long czasZwolnienia; // millis() docelowy "up" biezacego kroku
  bool togglOn;                 // stan dla TRYB_PRZELACZ
};

// Wywolywane raz w setup().
void macroEngineBegin();

// Wywolywane dla KAZDEGO slotu przy KAZDYM przebiegu petli glownej.
//   przypisanie        — wskaznik na Przypisanie z aktywnej warstwy dla
//                         tego slotu (moze byc nullptr / przypisany=false —
//                         wtedy funkcja nic nie robi).
//   fizycznyPoziom      — zdebounce'owany, biezacy stan fizyczny (true =
//                         trzymany) — steruje TRYB_PRZYTRZYMAJ oraz
//                         natychmiastowym zatrzymaniem.
//   zboczeWcisniecia    — true WYLACZNIE w tym jednym przebiegu petli, w
//                         ktorym debounce wykryl swiezy klik (zwolniony ->
//                         wcisniety). Steruje startem TRYB_NACISNIJ_I_PUSC
//                         i przelaczeniem TRYB_PRZELACZ.
void macroEngineKrok(uint8_t slot, const Przypisanie *przypisanie,
                      bool fizycznyPoziom, bool zboczeWcisniecia,
                      unsigned long teraz);

#endif // MAKROPAD_MACRO_ENGINE_H
