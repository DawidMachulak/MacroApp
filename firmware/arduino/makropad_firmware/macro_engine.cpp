/*
 * macro_engine.cpp
 *
 * Algorytm ponizej zostal NAJPIERW zweryfikowany symulacja w Pythonie
 * (skrypt poza repo, uzyty tylko do projektowania) - symulacja zlapala
 * realny blad: tryb TRYB_NACISNIJ_I_PUSC startujacy od POZIOMU sygnalu
 * zamiast ZBOCZA odpalalby sekwencje w kolko, dopoki klawisz jest fizycznie
 * trzymany, co przeczy definicji "nacisnij i pusc = raz". Stad rozroznienie
 * `fizycznyPoziom` (do TRYB_PRZYTRZYMAJ) vs `zboczeWcisniecia` (do startu
 * TRYB_NACISNIJ_I_PUSC i przelaczania TRYB_PRZELACZ) w macro_engine.h.
 *
 * Druga rzecz doprecyzowana dopiero przy przepisywaniu na C++ (symulacja
 * jej nie modelowala): co sie dzieje, gdy warstwa zmieni sie W TRAKCIE
 * trwania sekwencji (np. inny klawisz na tej samej warstwie ma akcje
 * AKCJA_WARSTWA i zostanie wcisniety, podczas gdy ten klawisz jest
 * przytrzymany)? Odpowiedz: kazdy aktywny slot pamieta wskaznik `s.p` na
 * przypisanie, ktore ZACZAL wykonywac, i po starcie ignoruje to, co akurat
 * mowi biezaca warstwa dla tego indeksu - inaczej mozna by zostawic
 * "zawieszony" wcisniety klawisz HID, ktorego nic juz nie zwolni (nowa
 * warstwa moze miec ten slot nieprzypisany).
 */
#include "macro_engine.h"
#include "hid_names.h"
#include "hid_output.h"
#include "config.h"
#include <string.h>

static StanPrzypisania stany[LICZBA_SLOTOW];

void macroEngineBegin() {
  for (uint8_t i = 0; i < LICZBA_SLOTOW; i++) {
    stany[i].aktywne = false;
    stany[i].p = nullptr;
    stany[i].indeksKroku = 0;
    stany[i].indeksZnaku = 0;
    stany[i].wcisniety = false;
    stany[i].czasZwolnienia = 0;
    stany[i].togglOn = false;
  }
}

// Arduino random(min, max) losuje z przedzialu [min, max) - stad +1, zeby
// "mx" z configu (wlacznie) bylo mozliwym wynikiem, zgodnie z docs/06 §3.3.
static unsigned long losoweOpoznienie(const OpoznienieLosowe &d) {
  if (d.mx <= d.mn) return d.mn;
  return d.mn + random((long)(d.mx - d.mn) + 1);
}

static void wykonajDol(uint8_t slot, const Akcja &a, uint8_t indeksZnaku) {
  switch (a.t) {
    case AKCJA_KLAWISZ: {
      uint8_t mod, kody[6];
      uint8_t n = rozbierzKombinacje(a.c, mod, kody);
      hidUstawWkladKlawiatury(slot, mod, kody, n);
      break;
    }
    case AKCJA_TEKST: {
      size_t dl = strlen(a.c);
      uint8_t mod, kod;
      if (indeksZnaku < dl && asciiNaHid(a.c[indeksZnaku], mod, kod)) {
        uint8_t kody[1] = { kod };
        hidUstawWkladKlawiatury(slot, mod, kody, 1);
      } else {
        // znak spoza podstawowego US-QWERTY (albo pusty tekst) - nic nie
        // wciskamy, ale krok i tak "trwa" swoj czas `d`, zeby rytm
        // wpisywania zostal zachowany nawet przy pojedynczym pominietym znaku
        hidWyczyscWkladKlawiatury(slot);
      }
      break;
    }
    case AKCJA_WARSTWA:
      wykonajZmianeWarstwyZTokenu(a.c);
      break;
    case AKCJA_MEDIA: {
      uint16_t usage = kodMediaZNazwy(a.c);
      if (usage != 0) hidWyslijMediaDol(usage);
      break;
    }
  }
}

static void wykonajGore(uint8_t slot, const Akcja &a) {
  switch (a.t) {
    case AKCJA_KLAWISZ:
    case AKCJA_TEKST:
      hidWyczyscWkladKlawiatury(slot);
      break;
    case AKCJA_WARSTWA:
      break; // brak koncepcji "Key Up" dla zmiany warstwy
    case AKCJA_MEDIA:
      hidWyslijMediaGora();
      break;
  }
}

static void rozpocznijKrok(uint8_t slot, StanPrzypisania &s, unsigned long teraz) {
  const Akcja &a = s.p->akcje[s.indeksKroku];
  wykonajDol(slot, a, s.indeksZnaku);
  s.wcisniety = true;
  s.czasZwolnienia = teraz + losoweOpoznienie(s.p->d);
}

// Konczy "dol" biezacego kroku/znaku, przechodzi do nastepnego. Jesli to
// byl ostatni znak/krok sekwencji: dla trybu bez petli (zapetlaj=false, czyli
// nacisnij-i-pusc) konczy cala sekwencje; dla trybu z petla (hold/toggle)
// zawija indeksKroku do 0 i leci dalej, dopoki wywolujacy uznaje slot za
// nadal aktywny.
static void zakonczKrokIPrzejdzDalej(uint8_t slot, StanPrzypisania &s, unsigned long teraz, bool zapetlaj) {
  const Akcja &a = s.p->akcje[s.indeksKroku];
  wykonajGore(slot, a);
  s.wcisniety = false;

  bool krokWCalosciZakonczony = true;
  if (a.t == AKCJA_TEKST) {
    s.indeksZnaku++;
    if (s.indeksZnaku < strlen(a.c)) krokWCalosciZakonczony = false;
  }

  if (krokWCalosciZakonczony) {
    s.indeksZnaku = 0;
    s.indeksKroku++;
    if (s.indeksKroku >= s.p->liczbaAkcji) {
      if (!zapetlaj) {
        s.aktywne = false;
        return;
      }
      s.indeksKroku = 0;
    }
  }
  rozpocznijKrok(slot, s, teraz);
}

void macroEngineKrok(uint8_t slot, const Przypisanie *przypisanie,
                      bool fizycznyPoziom, bool zboczeWcisniecia,
                      unsigned long teraz) {
  StanPrzypisania &s = stany[slot];
  bool jestUzywalne = (przypisanie != nullptr && przypisanie->przypisany &&
                        przypisanie->liczbaAkcji > 0 &&
                        przypisanie->liczbaAkcji <= MAKROPAD_MAX_AKCJI);

  if (!s.aktywne) {
    if (!jestUzywalne) return; // nic do zrobienia - brak przypisania na tym slocie w biezacej warstwie

    if (przypisanie->m == TRYB_NACISNIJ_I_PUSC) {
      if (zboczeWcisniecia) {
        s.aktywne = true;
        s.p = przypisanie;
        s.indeksKroku = 0;
        s.indeksZnaku = 0;
        rozpocznijKrok(slot, s, teraz);
      }
      return;
    }

    // TRYB_PRZYTRZYMAJ / TRYB_PRZELACZ
    if (przypisanie->m == TRYB_PRZELACZ && zboczeWcisniecia) s.togglOn = !s.togglOn;
    bool chcemyAktywne = (przypisanie->m == TRYB_PRZYTRZYMAJ) ? fizycznyPoziom : s.togglOn;
    if (chcemyAktywne) {
      s.aktywne = true;
      s.p = przypisanie;
      s.indeksKroku = 0;
      s.indeksZnaku = 0;
      rozpocznijKrok(slot, s, teraz);
    }
    return;
  }

  // --- s.aktywne == true: sekwencja juz trwa - odtad uzywamy WYLACZNIE
  // s.p (przechwyconego przy starcie), NIGDY parametru `przypisanie` -
  // patrz komentarz na gorze pliku o zmianie warstwy w trakcie trzymania.
  if (s.p->m == TRYB_NACISNIJ_I_PUSC) {
    if (s.wcisniety && teraz >= s.czasZwolnienia) {
      zakonczKrokIPrzejdzDalej(slot, s, teraz, /*zapetlaj=*/false);
    }
    return;
  }

  if (s.p->m == TRYB_PRZELACZ && zboczeWcisniecia) s.togglOn = !s.togglOn;
  bool chcemyAktywne = (s.p->m == TRYB_PRZYTRZYMAJ) ? fizycznyPoziom : s.togglOn;

  if (!chcemyAktywne) {
    // NATYCHMIASTOWE ZATRZYMANIE (docs/06 §5): zwalniamy biezacy "dol" OD
    // RAZU, nie czekamy na uplyw losowego opoznienia `d`.
    if (s.wcisniety) {
      wykonajGore(slot, s.p->akcje[s.indeksKroku]);
      s.wcisniety = false;
    }
    s.aktywne = false;
    s.indeksKroku = 0;
    s.indeksZnaku = 0;
    return;
  }

  if (s.wcisniety && teraz >= s.czasZwolnienia) {
    zakonczKrokIPrzejdzDalej(slot, s, teraz, /*zapetlaj=*/true);
  }
}
