/*
 * config.h — model danych configu w RAM.
 *
 * Struktury 1:1 odwzorowuja schemat JSON z docs/06-architektura-i-json.md
 * §3 (krotkie klucze v/active/ly/n/s/k/enc/m/a/t/c/d/mn/mx), zeby
 * json_codec.cpp mogl je (de)serializowac polem po polu bez dodatkowego
 * mapowania nazw.
 *
 * Limity ponizej (MAKROPAD_MAX_*) sa celowo malymi, bezpiecznymi dla RAM
 * XIAO nRF52840 (256 kB) wartosciami startowymi - da sie je podniesc, jesli
 * po pomiarach na realnym sprzecie okaze sie, ze jest zapas pamieci.
 */
#ifndef MAKROPAD_CONFIG_H
#define MAKROPAD_CONFIG_H

#include <Arduino.h>

// --- limity structury (patrz komentarz na gorze pliku) ---------------------
#define MAKROPAD_MAX_WARSTW           8   // docs/06 §5: "np. 8 warstw"
#define MAKROPAD_MAX_AKCJI            4   // maks. krokow w jednej sekwencji makra
#define MAKROPAD_MAX_DL_NAZWY_WARSTWY 10  // "n" - linia 1 OLED (docs/06 §3)
#define MAKROPAD_MAX_DL_STATUSU       40  // "s" - linia 2 OLED, jak w web/index.html
#define MAKROPAD_MAX_DL_TRESCI_AKCJI  64  // "c" - jak maxlength w web/index.html

#define MAKROPAD_LICZBA_KLAWISZY      10  // k[0..9]
#define MAKROPAD_LICZBA_AKCJI_ENC     3   // enc.l, enc.r, enc.b

// Wersja formatu configu, ktora ten firmware rozumie (pole "v" w JSON).
#define MAKROPAD_WERSJA_FORMATU       1
#define MAKROPAD_WERSJA_FIRMWARE      "0.3.0"

// --- tryby aktywacji (docs/06 §3.1, pole "m") -------------------------------
enum TrybAktywacji : uint8_t {
  TRYB_NACISNIJ_I_PUSC = 0,   // wykonaj raz, do konca, na zbocze wcisniecia
  TRYB_PRZYTRZYMAJ      = 1,  // powtarzaj w petli, dopoki fizycznie trzymany
  TRYB_PRZELACZ         = 2   // wl./wyl. petli kolejnymi wcisnieciami (toggle)
};

// --- typy akcji (docs/06 §3.2, pole "t") ------------------------------------
enum TypAkcji : uint8_t {
  AKCJA_KLAWISZ = 0,   // "c" = kombinacja klawiszy, np. "LCTRL+C"
  AKCJA_TEKST   = 1,   // "c" = dowolny tekst ASCII
  AKCJA_WARSTWA = 2,   // "c" = indeks warstwy jako tekst, albo "next"/"prev"
  AKCJA_MEDIA   = 3    // "c" = jedna z VOL_UP/VOL_DOWN/MUTE/PLAY_PAUSE/NEXT_TRACK/PREV_TRACK
};

// Pojedynczy krok sekwencji ("a[i]" w JSON).
struct Akcja {
  TypAkcji t;
  char c[MAKROPAD_MAX_DL_TRESCI_AKCJI + 1];
};

// Losowe opoznienie ("d" w JSON) - patrz docs/06 §3.3 za dokladna semantyka
// w zaleznosci od trybu.
struct OpoznienieLosowe {
  uint16_t mn;
  uint16_t mx;
};

// Przypisanie pod jednym klawiszem/akcja enkodera. `przypisany == false`
// odpowiada wartosci `null` w JSON (klawisz nieprzypisany w tej warstwie).
struct Przypisanie {
  bool przypisany;
  TrybAktywacji m;
  uint8_t liczbaAkcji;
  Akcja akcje[MAKROPAD_MAX_AKCJI];
  OpoznienieLosowe d;
};

struct AkcjeEnkodera {
  Przypisanie l;
  Przypisanie r;
  Przypisanie b;
};

struct Warstwa {
  char n[MAKROPAD_MAX_DL_NAZWY_WARSTWY + 1];
  char s[MAKROPAD_MAX_DL_STATUSU + 1];
  Przypisanie k[MAKROPAD_LICZBA_KLAWISZY];
  AkcjeEnkodera enc;
};

struct Konfiguracja {
  uint8_t wersja;
  uint8_t aktywnaWarstwa;
  uint8_t liczbaWarstw;
  Warstwa warstwy[MAKROPAD_MAX_WARSTW];
};

// Globalny config trzymany w RAM przez caly czas dzialania (docs/06 §5:
// "Firmware trzyma caly config w RAM (...), a na flash zapisuje dopiero na
// {"cmd":"save"}"). Zdefiniowany w config.cpp.
extern Konfiguracja config;

// Zeruje przypisanie do stanu "nieprzypisany" (odpowiednik `null` w JSON).
void wyczyscPrzypisanie(Przypisanie &p);

// Buduje minimalny, bezpieczny config startowy (1 warstwa, wszystko puste) -
// uzywany, gdy na flashu nie ma jeszcze zapisanego configu (pierwsze
// uruchomienie) albo gdy zapisany config nie przejdzie walidacji przy
// wczytywaniu.
void zbudujConfigDomyslny(Konfiguracja &cfg);

// Zwraca wskaznik na aktualnie aktywna warstwe (config.warstwy[config.aktywnaWarstwa]).
// Zaklada, ze config.liczbaWarstw > 0 i aktywnaWarstwa < liczbaWarstw - te
// niezmienniki sa pilnowane przez json_codec.cpp/storage.cpp przy kazdej
// zmianie configu.
Warstwa &aktywnaWarstwa();

// Flaga "trzeba przerysowac OLED" - ustawiana przez rozne moduly (zmiana
// warstwy, przychodzacy set_layer/set_active z hosta), czytana i zerowana
// przez display.cpp w glownej petli (throttling odswiezania jak w
// makropad_test.ino - patrz OLED_ODSWIEZ_MS w makropad_firmware.ino).
extern volatile bool zmianaDoWyswietlenia;

// Przelacza aktywna warstwe na `idx` (z zabezpieczeniem zakresu 0..liczbaWarstw-1),
// ustawia zmianaDoWyswietlenia.
void ustawAktywnaWarstwe(uint8_t idx);

// Interpretuje "c" akcji typu AKCJA_WARSTWA: liczba jako string ("0".."7")
// = indeks wprost, "next"/"prev" = nastepna/poprzednia z zawijaniem
// (docs/06 §3.2). Nieznany/niepoprawny token jest po cichu ignorowany.
void wykonajZmianeWarstwyZTokenu(const char *token);

#endif // MAKROPAD_CONFIG_H
