/*
 * ble_pairing.cpp
 *
 * UWAGA (jak w hid_output.cpp/storage.cpp): API Bluefruit ponizej
 * (Bluefruit.begin/setTxPower/setName, Bluefruit.Advertising.*,
 * Bluefruit.Periph.clearBonds, Bluefruit.disconnect) jest oparte na
 * standardowym przykladzie Adafruit "bleuart_hid_keyboard"/"hid_keyboard"
 * dla nRF52 - wysoka, ale nie stuprocentowa pewnosc bez kompilacji.
 */
#include "ble_pairing.h"
#include "hid_output.h"
#include "key_scanner.h"
#include "display.h"
#include <bluefruit.h>

static bool juzOdpalonoDlaTegoWcisniecia = false;

static void rozpocznijAdvertising() {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_KEYBOARD);
  Bluefruit.Advertising.addService(blehid);
  Bluefruit.Advertising.addName();

  // Standardowe parametry z przykladow Adafruit dla urzadzen HID - szybkie
  // wznawianie po rozlaczeniu, bez limitu czasu (0 = advertisuj w kolko,
  // dopoki ktos sie nie polaczy).
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);   // w jednostkach 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);     // sekundy szybkiego trybu
  Bluefruit.Advertising.start(0);
}

void blePairingBegin() {
  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  Bluefruit.setName("Makropad BLE");

  hidBeginBLE(); // blehid.begin() - patrz hid_output.h, kolejnosc jest wazna

  // Bonding jest domyslnym zachowaniem Bluefruit (docs/06 §7) - nic wiecej
  // nie trzeba wlaczac recznie, biblioteka sama zapisuje/wczytuje bonding
  // na flashu i wraca do ostatniego hosta.
  rozpocznijAdvertising();
}

// Wolane, gdy przytrzymanie SW przekroczy BLE_PAIRING_HOLD_MS - patrz
// docs/06 §7: "przytrzymanie przycisku enkodera przez kilka sekund ->
// clearBonds() -> powrot do advertisingu".
static void wykonajZapomnijParowanie() {
  displayKomunikat("Zapomniano host", 2500);

  if (Bluefruit.connected()) {
    Bluefruit.disconnect(Bluefruit.connHandle());
  }
  Bluefruit.Periph.clearBonds();

  // restartOnDisconnect(true) samo wznowi advertising po rozlaczeniu, ale
  // wolamy explicite na wypadek, gdyby akurat NIE bylo polaczenia (host juz
  // byl poza zasiegiem) - wtedy nie byloby zdarzenia "disconnect", ktore by
  // to zrobilo za nas.
  if (!Bluefruit.Advertising.isRunning()) {
    Bluefruit.Advertising.start(0);
  }
}

void blePairingKrok(unsigned long teraz) {
  unsigned long trwanie = enkoderPrzyciskCzasTrwaniaMs(teraz);

  if (trwanie == 0) {
    // Przycisk aktualnie nie jest (stabilnie) wcisniety - zresetuj flage,
    // zeby kolejne przytrzymanie znowu moglo odpalic gest.
    juzOdpalonoDlaTegoWcisniecia = false;
    return;
  }

  if (trwanie >= BLE_PAIRING_HOLD_MS && !juzOdpalonoDlaTegoWcisniecia) {
    juzOdpalonoDlaTegoWcisniecia = true;
    zawiesMakroEnkoderaDoZwolnienia(); // patrz key_scanner.h - blokuje podwojne dzialanie makra na tym samym wcisnieciu
    wykonajZapomnijParowanie();
  }
}
