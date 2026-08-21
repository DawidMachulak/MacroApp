#include "config.h"
#include <string.h>
#include <stdlib.h>

Konfiguracja config;

void wyczyscPrzypisanie(Przypisanie &p) {
  p.przypisany = false;
  p.m = TRYB_NACISNIJ_I_PUSC;
  p.liczbaAkcji = 0;
  for (uint8_t i = 0; i < MAKROPAD_MAX_AKCJI; i++) {
    p.akcje[i].t = AKCJA_KLAWISZ;
    p.akcje[i].c[0] = '\0';
  }
  p.d.mn = 0;
  p.d.mx = 0;
}

static void wyczyscWarstwe(Warstwa &w, const char *nazwa) {
  strncpy(w.n, nazwa, MAKROPAD_MAX_DL_NAZWY_WARSTWY);
  w.n[MAKROPAD_MAX_DL_NAZWY_WARSTWY] = '\0';
  strncpy(w.s, "Gotowy", MAKROPAD_MAX_DL_STATUSU);
  w.s[MAKROPAD_MAX_DL_STATUSU] = '\0';
  for (uint8_t i = 0; i < MAKROPAD_LICZBA_KLAWISZY; i++) wyczyscPrzypisanie(w.k[i]);
  wyczyscPrzypisanie(w.enc.l);
  wyczyscPrzypisanie(w.enc.r);
  wyczyscPrzypisanie(w.enc.b);
}

void zbudujConfigDomyslny(Konfiguracja &cfg) {
  cfg.wersja = MAKROPAD_WERSJA_FORMATU;
  cfg.aktywnaWarstwa = 0;
  cfg.liczbaWarstw = 1;
  for (uint8_t i = 0; i < MAKROPAD_MAX_WARSTW; i++) {
    wyczyscWarstwe(cfg.warstwy[i], "Warstwa 1");
  }
}

Warstwa &aktywnaWarstwa() {
  // Zabezpieczenie na wypadek niespojnego stanu (nie powinno sie zdarzyc,
  // bo json_codec/storage pilnuja niezmiennikow przy kazdej zmianie) - lepiej
  // zwrocic warstwe 0 niz wyjsc poza tablice.
  uint8_t idx = config.aktywnaWarstwa;
  if (idx >= config.liczbaWarstw) idx = 0;
  return config.warstwy[idx];
}

volatile bool zmianaDoWyswietlenia = true; // wymus pierwsze rysowanie po starcie

void ustawAktywnaWarstwe(uint8_t idx) {
  if (config.liczbaWarstw == 0) return;
  if (idx >= config.liczbaWarstw) idx = config.liczbaWarstw - 1;
  if (idx == config.aktywnaWarstwa) return;
  config.aktywnaWarstwa = idx;
  zmianaDoWyswietlenia = true;
}

void wykonajZmianeWarstwyZTokenu(const char *token) {
  if (config.liczbaWarstw == 0) return;

  if (strcmp(token, "next") == 0) {
    uint8_t nastepna = (config.aktywnaWarstwa + 1) % config.liczbaWarstw;
    ustawAktywnaWarstwe(nastepna);
    return;
  }
  if (strcmp(token, "prev") == 0) {
    uint8_t poprzednia = (config.aktywnaWarstwa == 0)
                            ? (config.liczbaWarstw - 1)
                            : (config.aktywnaWarstwa - 1);
    ustawAktywnaWarstwe(poprzednia);
    return;
  }

  // Token liczbowy = indeks wprost. `atoi` zwraca 0 dla nieparsowalnego
  // tekstu, co przypadkowo pokrywa sie z warstwa 0 - akceptowalne, bo
  // configurator (web/index.html) i tak nie pozwala wpisac tu nic innego
  // niz index/next/prev.
  int idx = atoi(token);
  if (idx >= 0 && idx < config.liczbaWarstw) {
    ustawAktywnaWarstwe((uint8_t)idx);
  }
}
