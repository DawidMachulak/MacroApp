/*
 * hid_output.cpp
 *
 * UWAGA (przeczytaj przed zglaszaniem bledu kompilacji): to jest plik z
 * NAJWIEKSZA niepewnoscia w calym firmware, bo dotyka dokladnych nazw metod
 * klas `Adafruit_USBD_HID` (Adafruit TinyUSB) i `BLEHidAdafruit` (Adafruit
 * Bluefruit nRF52) — a w tym srodowisku nie da sie zainstalowac pakietu
 * plytek/bibliotek ani skompilowac szkicu, wiec te wywolania NIE zostaly
 * automatycznie zweryfikowane (w odroznieniu od reszty firmware, ktorej
 * logike dalo sie sprawdzic symulacja - patrz macro_engine.cpp). Zalozenia,
 * ktore tu przyjelem (oparte na oficjalnych przykladach Adafruit -
 * `hid_composite` dla TinyUSB, `bleuart_hid_keyboard`/`hid_keys` dla
 * Bluefruit):
 *
 *   Adafruit_USBD_HID (USB):
 *     usb_hid.setPollInterval(2);
 *     usb_hid.setReportDescriptor(desc, sizeof(desc));
 *     usb_hid.begin();
 *     usb_hid.keyboardReport(REPORT_ID_KEYBOARD, modyfikator, kody[6]);
 *     usb_hid.keyboardRelease(REPORT_ID_KEYBOARD);
 *     usb_hid.sendReport(REPORT_ID_CONSUMER, &usage, sizeof(usage));
 *
 *   BLEHidAdafruit (BLE):
 *     blehid.begin();
 *     blehid.keyboardReport(modyfikator, kody[6]);
 *     blehid.keyRelease();
 *     blehid.consumerKeyPress(usage);
 *     blehid.consumerKeyRelease();
 *
 * Jesli kompilator zglosi "no member named 'xxx'" — to jest JEDYNY plik do
 * poprawienia; podmien nazwe metody na te z Twojej wersji biblioteki
 * (sprawdz w plikach .h biblioteki w folderze
 * Arduino/libraries/Adafruit_TinyUSB_Library i
 * Arduino/libraries/Adafruit_nRF52_Bluefruit_nRF52_Libraries — albo po
 * prostu wpisz `blehid.` / `usb_hid.` w Arduino IDE i sprawdz podpowiedzi
 * autouzupelniania).
 */
#include "hid_output.h"
#include "config.h"
#include "macro_engine.h"   // LICZBA_SLOTOW
#include <Adafruit_TinyUSB.h>
#include <bluefruit.h>
#include <cstring>

// --- deskryptor raportow USB: klawiatura + consumer control -----------------
enum {
  REPORT_ID_KEYBOARD = 1,
  REPORT_ID_CONSUMER = 2,
};

uint8_t const descRaportowUsb[] = {
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(REPORT_ID_KEYBOARD)),
  TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(REPORT_ID_CONSUMER)),
};

Adafruit_USBD_HID usb_hid;
BLEHidAdafruit blehid;

// --- agregacja wkladu klawiatury per slot ------------------------------------
struct WkladSlotu {
  bool aktywny;
  uint8_t modyfikatory;
  uint8_t liczbaKodow;
  uint8_t kody[6];
};

static WkladSlotu wklady[LICZBA_SLOTOW];
static uint8_t ostatniModyfikatory = 0;
static uint8_t ostatnieKody[6] = { 0, 0, 0, 0, 0, 0 };
static bool ostatniRaportPusty = true;

void hidBegin() {
  for (uint8_t i = 0; i < LICZBA_SLOTOW; i++) {
    wklady[i].aktywny = false;
    wklady[i].modyfikatory = 0;
    wklady[i].liczbaKodow = 0;
  }

  usb_hid.setPollInterval(2);
  usb_hid.setReportDescriptor(descRaportowUsb, sizeof(descRaportowUsb));
  usb_hid.begin();
}

void hidBeginBLE() {
  blehid.begin();
}

void hidLoop() {
  // Adafruit TinyUSB obsluguje device task automatycznie w tle na nRF52
  // (w odroznieniu od niektorych platform, gdzie trzeba recznie wolac
  // TinyUSBDevice.task()) - funkcja zostawiona jako miejsce na ewentualne
  // przyszle throttlowanie/diagnostyke wysylki HID.
}

bool hidGotowyUSB() {
  return TinyUSBDevice.mounted();
}

bool hidGotowyBLE() {
  return Bluefruit.connected() > 0;
}

// Sklada wklady wszystkich slotow w jeden raport i wysyla go, ALE tylko gdy
// wynikowy raport faktycznie sie zmienil wzgledem ostatnio wyslanego -
// unika zasypywania hosta identycznymi raportami przy kazdym przebiegu
// petli (macro_engine wywoluje hidUstawWklad*/hidWyczyscWklad* przy KAZDEJ
// zmianie stanu danego slotu, nie tylko raz).
static void przeliczIWyslijRaportKlawiatury() {
  uint8_t modyfikatory = 0;
  uint8_t kody[6] = { 0, 0, 0, 0, 0, 0 };
  uint8_t liczbaKodow = 0;

  for (uint8_t i = 0; i < LICZBA_SLOTOW && liczbaKodow < 6; i++) {
    if (!wklady[i].aktywny) continue;
    modyfikatory |= wklady[i].modyfikatory;
    for (uint8_t k = 0; k < wklady[i].liczbaKodow && liczbaKodow < 6; k++) {
      // pomijamy duplikaty (dwa slotyprzypadkiem wysylajace ten sam kod)
      bool jestJuz = false;
      for (uint8_t j = 0; j < liczbaKodow; j++) {
        if (kody[j] == wklady[i].kody[k]) { jestJuz = true; break; }
      }
      if (!jestJuz) kody[liczbaKodow++] = wklady[i].kody[k];
    }
  }

  bool pusty = (modyfikatory == 0 && liczbaKodow == 0);
  bool takieSameJakOstatnio = (modyfikatory == ostatniModyfikatory) &&
                               (memcmp(kody, ostatnieKody, sizeof(kody)) == 0);
  if (pusty && ostatniRaportPusty) return;       // nic sie nie zmienilo, oba puste
  if (!pusty && takieSameJakOstatnio && !ostatniRaportPusty) return; // identyczny jak ostatnio

  if (hidGotowyUSB()) {
    if (pusty) usb_hid.keyboardRelease(REPORT_ID_KEYBOARD);
    else       usb_hid.keyboardReport(REPORT_ID_KEYBOARD, modyfikatory, kody);
  }
  if (hidGotowyBLE()) {
    if (pusty) blehid.keyRelease();
    else       blehid.keyboardReport(modyfikatory, kody);
  }

  ostatniModyfikatory = modyfikatory;
  memcpy(ostatnieKody, kody, sizeof(kody));
  ostatniRaportPusty = pusty;
}

void hidUstawWkladKlawiatury(uint8_t slot, uint8_t modyfikatory, const uint8_t *kody, uint8_t liczbaKodow) {
  if (slot >= LICZBA_SLOTOW) return;
  wklady[slot].aktywny = true;
  wklady[slot].modyfikatory = modyfikatory;
  wklady[slot].liczbaKodow = min<uint8_t>(liczbaKodow, 6);
  for (uint8_t i = 0; i < wklady[slot].liczbaKodow; i++) wklady[slot].kody[i] = kody[i];
  przeliczIWyslijRaportKlawiatury();
}

void hidWyczyscWkladKlawiatury(uint8_t slot) {
  if (slot >= LICZBA_SLOTOW) return;
  if (!wklady[slot].aktywny) return;
  wklady[slot].aktywny = false;
  wklady[slot].liczbaKodow = 0;
  przeliczIWyslijRaportKlawiatury();
}

void hidWyslijMediaDol(uint16_t usage) {
  if (hidGotowyUSB()) usb_hid.sendReport(REPORT_ID_CONSUMER, &usage, sizeof(usage));
  if (hidGotowyBLE()) blehid.consumerKeyPress(usage);
}

void hidWyslijMediaGora() {
  uint16_t pusty = 0;
  if (hidGotowyUSB()) usb_hid.sendReport(REPORT_ID_CONSUMER, &pusty, sizeof(pusty));
  if (hidGotowyBLE()) blehid.consumerKeyRelease();
}
