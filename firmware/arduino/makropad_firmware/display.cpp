#include "display.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string.h>

#define ADRES_OLED 0x3C
#define OLED_SZER  128
#define OLED_WYS   32
#define OLED_ODSWIEZ_MS 80  // jak w makropad_test.ino - throttling rysowania, nie skanowania

static Adafruit_SSD1306 oled(OLED_SZER, OLED_WYS, &Wire, -1);
static unsigned long ostatnieOdswiezenie = 0;

// Komunikat systemowy (np. z ble_pairing.cpp) nadpisujacy linie statusu do
// czasu nastepnej "prawdziwej" zmiany (zmianaDoWyswietlenia ustawione przez
// cokolwiek innego niz displayKomunikat samo).
static char komunikatSystemowy[MAKROPAD_MAX_DL_STATUSU + 1] = { 0 };
static unsigned long komunikatWygasaMs = 0; // 0 = nic nie jest zaplanowane

void displayBegin() {
  if (!oled.begin(SSD1306_SWITCHCAPVCC, ADRES_OLED)) {
    Serial.println("Nie widze wyswietlacza pod 0x3C - sprawdz SDA/SCL.");
  }
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
}

// Utnij `tekst` tak, zeby zmiescil sie w `szerokoscPx` przy biezacym
// foncie/rozmiarze - odpowiednik truncOled() z web/index.html (tam liczone
// canvasem, tu przez Adafruit_GFX::getTextBounds). Dopisuje "..." przy
// obcieciu. `wynik` musi miec miejsce na co najmniej dlugosc(tekst)+1.
static void utnijDoSzerokosci(const char *tekst, char *wynik, size_t maxDlWyniku, int16_t szerokoscPx) {
  int16_t x1, y1;
  uint16_t w, h;
  oled.getTextBounds(tekst, 0, 0, &x1, &y1, &w, &h);
  if (w <= (uint16_t)szerokoscPx) {
    strncpy(wynik, tekst, maxDlWyniku - 1);
    wynik[maxDlWyniku - 1] = '\0';
    return;
  }

  size_t dl = strlen(tekst);
  char probny[MAKROPAD_MAX_DL_STATUSU + 4];
  for (size_t obciecie = dl; obciecie > 0; obciecie--) {
    if (obciecie + 3 >= sizeof(probny)) continue;
    strncpy(probny, tekst, obciecie);
    strcpy(probny + obciecie, "...");
    oled.getTextBounds(probny, 0, 0, &x1, &y1, &w, &h);
    if (w <= (uint16_t)szerokoscPx) {
      strncpy(wynik, probny, maxDlWyniku - 1);
      wynik[maxDlWyniku - 1] = '\0';
      return;
    }
  }
  wynik[0] = '\0'; // nawet samo "..." sie nie miesci (skrajny przypadek) - pusto
}

void displayKomunikat(const char *tekst, unsigned long czasTrwaniaMs) {
  strncpy(komunikatSystemowy, tekst, MAKROPAD_MAX_DL_STATUSU);
  komunikatSystemowy[MAKROPAD_MAX_DL_STATUSU] = '\0';
  komunikatWygasaMs = millis() + czasTrwaniaMs;
  zmianaDoWyswietlenia = true;
}

void displayKrok(unsigned long teraz) {
  // Komunikat systemowy wygasl - wroc do statusu warstwy i wymus jeszcze
  // jedno odswiezenie, zeby ekran nie zostal "zamrozony" na starej tresci.
  if (komunikatSystemowy[0] != '\0' && komunikatWygasaMs != 0 && teraz >= komunikatWygasaMs) {
    komunikatSystemowy[0] = '\0';
    komunikatWygasaMs = 0;
    zmianaDoWyswietlenia = true;
  }

  if (!zmianaDoWyswietlenia) return;
  if (teraz - ostatnieOdswiezenie < OLED_ODSWIEZ_MS) return;

  Warstwa &w = aktywnaWarstwa();
  const char *linia2Zrodlo = (komunikatSystemowy[0] != '\0') ? komunikatSystemowy
                                                               : (w.s[0] != '\0' ? w.s : "Gotowy");

  char linia1[OLED_SZER / 6 + 4];
  char linia2[OLED_SZER / 6 + 4];
  utnijDoSzerokosci(w.n, linia1, sizeof(linia1), OLED_SZER);
  utnijDoSzerokosci(linia2Zrodlo, linia2, sizeof(linia2), OLED_SZER);

  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.println(linia1);
  oled.setCursor(0, 16); // druga "logiczna" linia, z odstepem (font 8px, wys. 32px)
  oled.println(linia2);
  oled.display();

  ostatnieOdswiezenie = teraz;
  zmianaDoWyswietlenia = false;
}
