#include "key_scanner.h"
#include "config.h"
#include "macro_engine.h"
#include <Wire.h>
#include <PCF8574.h>

#define ADRES_PCF   0x20      // po zwarciu A0, A1, A2 do GND (patrz docs/02)
#define DEBOUNCE_MS 8         // jak w makropad_test.ino

const int PIN_ENC_A  = D0;
const int PIN_ENC_B  = D1;
const int PIN_ENC_SW = D2;
const int PIN_KLAW9  = D6;
const int PIN_KLAW10 = D7;

static PCF8574 pcf(ADRES_PCF);

static bool          stanSurowyKlaw[MAKROPAD_LICZBA_KLAWISZY]   = { false };
static bool          stanStabilnyKlaw[MAKROPAD_LICZBA_KLAWISZY] = { false };
static unsigned long czasZmianyKlaw[MAKROPAD_LICZBA_KLAWISZY]   = { 0 };

static bool          swSurowy = false, swStabilny = false;
static unsigned long swCzasZmiany = 0;
static unsigned long swCzasPoczatkuWcisniecia = 0; // millis() poczatku biezacego wcisniecia (0 = nie wcisniety)
static bool          swZawieszonyDlaMakra = false;  // patrz zawiesMakroEnkoderaDoZwolnienia()

// Tabela przejsc kwadratury (Gray code) - jak w makropad_test.ino.
static const int8_t TABELA_KWADRATURY[16] = {
   0, -1,  1,  0,
   1,  0,  0, -1,
  -1,  0,  0,  1,
   0,  1, -1,  0
};
static uint8_t stanKwadratury     = 0;
static int16_t akumulatorCwiartek = 0;

// Aktualizuje debounce jednego wejscia binarnego. Zwraca true, jesli
// stabilny (odfiltrowany) stan zmienil sie WLASNIE w tym wywolaniu - to
// jest "zbocze", ktorego potrzebuje macro_engine.
static bool aktualizujDebounce(bool surowy, bool &pamSurowy, bool &pamStabilny,
                                unsigned long &czasZmiany, unsigned long teraz) {
  if (surowy != pamSurowy) {
    pamSurowy  = surowy;
    czasZmiany = teraz;
  }
  if (pamStabilny != pamSurowy && (teraz - czasZmiany) >= DEBOUNCE_MS) {
    pamStabilny = pamSurowy;
    return true;
  }
  return false;
}

void keyScannerBegin() {
  pinMode(PIN_ENC_A,  INPUT_PULLUP);
  pinMode(PIN_ENC_B,  INPUT_PULLUP);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);
  pinMode(PIN_KLAW9,  INPUT_PULLUP);
  pinMode(PIN_KLAW10, INPUT_PULLUP);

  if (!pcf.begin(0xFF)) {
    Serial.println("Nie widze PCF8574 pod 0x20 - sprawdz zasilanie i zworki adresowe.");
  }

  stanKwadratury = (digitalRead(PIN_ENC_A) << 1) | digitalRead(PIN_ENC_B);
}

unsigned long enkoderPrzyciskCzasTrwaniaMs(unsigned long teraz) {
  if (!swStabilny || swCzasPoczatkuWcisniecia == 0) return 0;
  return teraz - swCzasPoczatkuWcisniecia;
}

void zawiesMakroEnkoderaDoZwolnienia() {
  swZawieszonyDlaMakra = true;
}

void keyScannerKrok(unsigned long teraz) {
  Warstwa &w = aktywnaWarstwa();

  // --- 10 klawiszy: 8x PCF8574 (bit=0 -> wcisniety) + 2x wprost -----------
  uint8_t stanPcf = pcf.read8();
  bool surowe[MAKROPAD_LICZBA_KLAWISZY];
  for (uint8_t i = 0; i < 8; i++) surowe[i] = !(stanPcf & (1 << i));
  surowe[8] = (digitalRead(PIN_KLAW9)  == LOW);
  surowe[9] = (digitalRead(PIN_KLAW10) == LOW);

  for (uint8_t i = 0; i < MAKROPAD_LICZBA_KLAWISZY; i++) {
    bool zbocze = aktualizujDebounce(surowe[i], stanSurowyKlaw[i], stanStabilnyKlaw[i],
                                      czasZmianyKlaw[i], teraz);
    bool zboczeWcisniecia = zbocze && stanStabilnyKlaw[i]; // zbocze narastajace tylko
    const Przypisanie *p = w.k[i].przypisany ? &w.k[i] : nullptr;
    macroEngineKrok(SLOT_KLAWISZ_0 + i, p, stanStabilnyKlaw[i], zboczeWcisniecia, teraz);
  }

  // --- przycisk enkodera (SW) ----------------------------------------------
  bool swZboczeSurowe = aktualizujDebounce(digitalRead(PIN_ENC_SW) == LOW, swSurowy,
                                            swStabilny, swCzasZmiany, teraz);
  if (swZboczeSurowe) {
    if (swStabilny) {
      swCzasPoczatkuWcisniecia = teraz; // swiezy klik - start liczenia dlugosci
      swZawieszonyDlaMakra = false;      // nowy cykl wcisniecia - zdejmij ewentualna blokade z poprzedniego
    } else {
      swCzasPoczatkuWcisniecia = 0;      // puszczony
    }
  }
  bool swZboczeWcisniecia = swZboczeSurowe && swStabilny;

  if (swZawieszonyDlaMakra) {
    // Biezace przytrzymanie zostalo juz uznane za gest "zapomnij parowanie
    // BLE" (ble_pairing.cpp) - macro_engine ma je widziec jako PUSZCZONY
    // przycisk, zeby ewentualne trwajace makro trybu przytrzymaj/przelacz
    // zatrzymalo sie natychmiast (patrz macro_engine.cpp).
    macroEngineKrok(SLOT_ENC_B, w.enc.b.przypisany ? &w.enc.b : nullptr,
                     /*fizycznyPoziom=*/false, /*zboczeWcisniecia=*/false, teraz);
  } else {
    macroEngineKrok(SLOT_ENC_B, w.enc.b.przypisany ? &w.enc.b : nullptr,
                     swStabilny, swZboczeWcisniecia, teraz);
  }

  // --- obrot enkodera: pelna kwadratura, bez debounce'u czasowego --------
  // (filtr RC na CLK/DT - patrz docs/02). Kazdy "klik" (4 cwiartki) daje
  // JEDNOTAKTOWY impuls do macro_engine: fizycznyPoziom i zboczeWcisniecia
  // oba true przez dokladnie jeden przebieg petli, potem oba false - patrz
  // uwaga w key_scanner.h o tym, jak to sie przeklada na tryby aktywacji.
  stanKwadratury = ((stanKwadratury << 2) | (digitalRead(PIN_ENC_A) << 1) | digitalRead(PIN_ENC_B)) & 0x0F;
  akumulatorCwiartek += TABELA_KWADRATURY[stanKwadratury];

  // Zalozenie: przyrost akumulatora = obrot w prawo. Jesli w praktyce
  // enkoder "l" i "r" sa zamienione (jak w makropad_test.ino), zamien tu
  // impulsLewo/impulsPrawo albo PIN_ENC_A z PIN_ENC_B wyzej.
  bool impulsLewo = false, impulsPrawo = false;
  if (akumulatorCwiartek >= 4) {
    impulsPrawo = true;
    akumulatorCwiartek = 0;
  } else if (akumulatorCwiartek <= -4) {
    impulsLewo = true;
    akumulatorCwiartek = 0;
  }

  macroEngineKrok(SLOT_ENC_L, w.enc.l.przypisany ? &w.enc.l : nullptr,
                   impulsLewo, impulsLewo, teraz);
  macroEngineKrok(SLOT_ENC_R, w.enc.r.przypisany ? &w.enc.r : nullptr,
                   impulsPrawo, impulsPrawo, teraz);
}
