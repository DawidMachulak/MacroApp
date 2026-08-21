/*
 * Uzywa standardowego API ArduinoJson 6.x (StaticJsonDocument,
 * createNestedObject/createNestedArray, JsonVariant::to<T>()/set()) - patrz
 * uwaga o wersji biblioteki w json_codec.h. Podobnie jak reszta firmware,
 * ten plik NIE zostal skompilowany w tym srodowisku (brak arduino-cli) -
 * skladnia jest oparta na dobrze udokumentowanym, stabilnym API v6, wiec
 * pewnosc jest tu wyzsza niz np. w hid_output.cpp, ale warto to
 * skompilowac jako pierwsze przy pierwszym sprawdzeniu w Arduino IDE.
 */
#include "json_codec.h"
#include <string.h>

static void skopiujString(const char *src, char *dst, size_t maxDl) {
  if (src == nullptr) { dst[0] = '\0'; return; }
  strncpy(dst, src, maxDl);
  dst[maxDl] = '\0';
}

bool jsonWczytajPrzypisanie(JsonVariantConst zrodlo, Przypisanie &p) {
  if (zrodlo.isNull()) {
    wyczyscPrzypisanie(p);
    return true;
  }
  if (!zrodlo.is<JsonObjectConst>()) return false;
  JsonObjectConst obj = zrodlo.as<JsonObjectConst>();

  JsonVariantConst aVar = obj["a"];
  if (!aVar.is<JsonArrayConst>()) return false;
  JsonArrayConst aArr = aVar.as<JsonArrayConst>();
  if (aArr.size() == 0) return false; // docs/06 §3: "sekwencja akcji (min. 1 element)"

  wyczyscPrzypisanie(p);
  p.przypisany = true;

  int m = obj["m"] | 0;
  if (m < TRYB_NACISNIJ_I_PUSC || m > TRYB_PRZELACZ) m = TRYB_NACISNIJ_I_PUSC;
  p.m = (TrybAktywacji)m;

  p.liczbaAkcji = 0;
  for (JsonVariantConst krokVar : aArr) {
    if (p.liczbaAkcji >= MAKROPAD_MAX_AKCJI) break; // wiecej krokow niz limit - ucinamy, nie blokujemy
    if (!krokVar.is<JsonObjectConst>()) continue;
    JsonObjectConst krok = krokVar.as<JsonObjectConst>();

    int t = krok["t"] | 0;
    if (t < AKCJA_KLAWISZ || t > AKCJA_MEDIA) t = AKCJA_KLAWISZ;

    Akcja &a = p.akcje[p.liczbaAkcji];
    a.t = (TypAkcji)t;
    skopiujString(krok["c"] | "", a.c, MAKROPAD_MAX_DL_TRESCI_AKCJI);
    p.liczbaAkcji++;
  }
  if (p.liczbaAkcji == 0) return false; // wszystkie kroki byly niepoprawne

  JsonVariantConst dVar = obj["d"];
  long mn = dVar["mn"] | 0;
  long mx = dVar["mx"] | 0;
  if (mn < 0) mn = 0;
  if (mx < mn) mx = mn;
  if (mn > 65535) mn = 65535;
  if (mx > 65535) mx = 65535;
  p.d.mn = (uint16_t)mn;
  p.d.mx = (uint16_t)mx;

  return true;
}

void jsonZapiszPrzypisanie(const Przypisanie &p, JsonVariant cel) {
  if (!p.przypisany) {
    cel.set(nullptr);
    return;
  }
  JsonObject obj = cel.to<JsonObject>();
  obj["m"] = (int)p.m;
  JsonArray a = obj.createNestedArray("a");
  for (uint8_t i = 0; i < p.liczbaAkcji; i++) {
    JsonObject krok = a.createNestedObject();
    krok["t"] = (int)p.akcje[i].t;
    krok["c"] = p.akcje[i].c;
  }
  JsonObject d = obj.createNestedObject("d");
  d["mn"] = p.d.mn;
  d["mx"] = p.d.mx;
}

bool jsonWczytajWarstwe(JsonObjectConst zrodlo, Warstwa &w) {
  skopiujString(zrodlo["n"] | "", w.n, MAKROPAD_MAX_DL_NAZWY_WARSTWY);
  if (w.n[0] == '\0') skopiujString("Warstwa", w.n, MAKROPAD_MAX_DL_NAZWY_WARSTWY);
  skopiujString(zrodlo["s"] | "Gotowy", w.s, MAKROPAD_MAX_DL_STATUSU);

  JsonVariantConst kVar = zrodlo["k"];
  if (!kVar.is<JsonArrayConst>()) return false;
  JsonArrayConst kArr = kVar.as<JsonArrayConst>();

  for (uint8_t i = 0; i < MAKROPAD_LICZBA_KLAWISZY; i++) {
    if (i < kArr.size()) {
      if (!jsonWczytajPrzypisanie(kArr[i], w.k[i])) wyczyscPrzypisanie(w.k[i]);
    } else {
      wyczyscPrzypisanie(w.k[i]);
    }
  }

  JsonObjectConst enc = zrodlo["enc"].as<JsonObjectConst>();
  if (!jsonWczytajPrzypisanie(enc["l"], w.enc.l)) wyczyscPrzypisanie(w.enc.l);
  if (!jsonWczytajPrzypisanie(enc["r"], w.enc.r)) wyczyscPrzypisanie(w.enc.r);
  if (!jsonWczytajPrzypisanie(enc["b"], w.enc.b)) wyczyscPrzypisanie(w.enc.b);

  return true;
}

void jsonZapiszWarstwe(const Warstwa &w, JsonObject cel) {
  cel["n"] = w.n;
  cel["s"] = w.s;

  JsonArray k = cel.createNestedArray("k");
  for (uint8_t i = 0; i < MAKROPAD_LICZBA_KLAWISZY; i++) {
    jsonZapiszPrzypisanie(w.k[i], k.add<JsonVariant>());
  }

  JsonObject enc = cel.createNestedObject("enc");
  jsonZapiszPrzypisanie(w.enc.l, enc["l"]);
  jsonZapiszPrzypisanie(w.enc.r, enc["r"]);
  jsonZapiszPrzypisanie(w.enc.b, enc["b"]);
}
