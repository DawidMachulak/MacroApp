# 06 — Architektura configuratora i struktura JSON

Krok 1 projektu: architektura, format danych i mockup UI, zanim powstanie
prawdziwa integracja Web Serial (Krok 2) i docelowy firmware BLE HID (Krok 3).

## 1. Przegląd

```
┌─────────────────────┐   Web Serial API    ┌──────────────────────┐
│  Web Configurator    │ ───────────────────▶│  XIAO nRF52840        │
│  (index.html, PL)    │  JSON, linia po      │  firmware             │
│  GitHub Pages        │  linii, 115200 bd    │                       │
│                       │◀─────────────────── │  - parsuje JSON        │
│  - edycja warstw      │   ack / błąd / cfg   │  - trzyma config w RAM │
│  - edycja makr        │                      │  - zapisuje do flash   │
│  - symulator OLED     │                      │  - wysyła HID (USB/BLE)│
└─────────────────────┘                      └──────────────────────┘
```

Konfigurator nie wysyła całego configu w jednej gigantycznej ramce — bufor
Serial na nRF52 jest ograniczony. Transfer jest podzielony **warstwa po
warstwie** (patrz §4 Protokół).

## 2. Model danych — pojęcia

- **Warstwa (layer)** — profil przypisań, przełączany enkoderem (klik =
  następna warstwa, albo przytrzymanie = poprzednia — do ustalenia w UI).
- **Przypisanie (assignment)** — to, co się dzieje pod jednym klawiszem albo
  jedną z 3 akcji enkodera w obrębie warstwy: tryb aktywacji + sekwencja
  akcji + losowe opóźnienie.
- **Akcja (action)** — pojedynczy krok w sekwencji: wysłanie kombinacji
  klawiszy, wpisanie tekstu, zmiana warstwy albo klawisz multimedialny.
  Większość przypisań ma jedną akcję; makra złożone (np. kombo w grze) mogą
  mieć kilka kroków wykonywanych po kolei.

## 3. Struktura JSON (krótkie klucze — dla firmware)

```jsonc
{
  "v": 1,                         // wersja formatu configu
  "active": 0,                    // warstwa aktywna po starcie
  "ly": [                         // tablica warstw
    {
      "n": "FPS",                 // nazwa warstwy, max 10 znaków — linia 1 na OLED
      "s": "Gotowy",              // tekst statusu, max 21 znaków — linia 2 na OLED,
                                   // edytowalny w configuratorze (domyślnie "Gotowy")
      "k": [                      // 10 przypisań klawiszy, indeks 0-9 = klawisz 1-10
        {
          "m": 0,                 // tryb aktywacji, patrz tabela niżej
          "a": [                  // sekwencja akcji (min. 1 element)
            { "t": 0, "c": "LCTRL+C" }
          ],
          "d": { "mn": 50, "mx": 150 }   // losowe opóźnienie w ms
        },
        null,                     // null = klawisz nieprzypisany w tej warstwie
        ...
      ],
      "enc": {                    // 3 akcje enkodera
        "l": { "m": 0, "a": [{ "t": 0, "c": "VOL_DOWN" }], "d": { "mn": 0, "mx": 0 } },
        "r": { "m": 0, "a": [{ "t": 0, "c": "VOL_UP" }],   "d": { "mn": 0, "mx": 0 } },
        "b": { "m": 0, "a": [{ "t": 2, "c": "next" }],     "d": { "mn": 0, "mx": 0 } }
      }
    }
  ]
}
```

### 3.1 Tryby aktywacji (`m`)

| `m` | Nazwa w firmware | Pełna nazwa w UI |
|---|---|---|
| `0` | press&release | Naciśnij i puść (raz) |
| `1` | hold | Przytrzymaj (powtarzaj w pętli) |
| `2` | toggle | Przełącz (włącz/wyłącz pętlę) |

### 3.2 Typy akcji (`t`)

| `t` | Nazwa w firmware | Pełna nazwa w UI | `c` zawiera |
|---|---|---|---|
| `0` | key | Klawisz / kombinacja | np. `"LCTRL+LSHIFT+ESC"` — nazwy klawiszy złączone `+` |
| `1` | text | Wpisz tekst | dowolny tekst ASCII, np. `"gg wp"` |
| `2` | layer | Zmiana warstwy | indeks warstwy (`"0"`), albo `"next"` / `"prev"` |
| `3` | media | Multimedia | jedna z: `VOL_UP`, `VOL_DOWN`, `MUTE`, `PLAY_PAUSE`, `NEXT_TRACK`, `PREV_TRACK` |

### 3.3 Opóźnienie losowe (`d`)

```json
"d": { "mn": 50, "mx": 150 }
```

- Tryb `0` (press&release): losowa wartość z zakresu `[mn, mx]` odczekiwana
  między Key Down a Key Up.
- Tryb `1`/`2` (hold / toggle): losowa wartość odczekiwana między kolejnymi
  powtórzeniami pętli (nie blokująco — patrz §5).
- `{ "mn": 0, "mx": 0 }` = brak opóźnienia (użyteczne np. dla enkodera).

### 3.4 Nazewnictwo klawiszy w `c` (typ `0`)

Modyfikatory: `LCTRL`, `RCTRL`, `LSHIFT`, `RSHIFT`, `LALT`, `RALT`, `LGUI`,
`RGUI`. Klawisze: litery `A`-`Z`, cyfry `0`-`9`, `F1`-`F24`, `ESC`, `TAB`,
`ENTER`, `SPACE`, `BACKSPACE`, `DEL`, strzałki `UP`/`DOWN`/`LEFT`/`RIGHT`,
`HOME`, `END`, `PGUP`, `PGDN`. Łączenie przez `+`, bez spacji, np.
`"LCTRL+LALT+DEL"`. Pełną listę + mapowanie na kody HID (`HID_KEY_*` z
biblioteki Adafruit TinyUSB / Bluefruit) trzyma firmware — configurator zna
tylko nazwy do wyświetlenia i walidacji.

## 4. Protokół komunikacji (Web Serial, 115200 bd, linia = jedna ramka JSON + `\n`)

| Kierunek | Komenda | Opis |
|---|---|---|
| Host → Urządzenie | `{"cmd":"ping"}` | Sprawdzenie połączenia |
| Urządzenie → Host | `{"cmd":"pong","fw":"0.1.0"}` | Odpowiedź + wersja firmware |
| Host → Urządzenie | `{"cmd":"get_config"}` | Żądanie odczytu configu |
| Urządzenie → Host | `{"cmd":"layer","idx":0,"data":{...}}` × N, potem `{"cmd":"end"}` | Config wysyłany warstwa po warstwie |
| Host → Urządzenie | `{"cmd":"set_layer","idx":0,"data":{...}}` | Nadpisanie jednej warstwy w RAM |
| Urządzenie → Host | `{"cmd":"ack","ok":true}` / `{"cmd":"err","msg":"..."}` | Potwierdzenie / błąd parsowania |
| Host → Urządzenie | `{"cmd":"save"}` | Zapis aktualnego configu z RAM do pamięci flash (LittleFS / InternalFS) |
| Host → Urządzenie | `{"cmd":"set_active","idx":1}` | Natychmiastowe przełączenie aktywnej warstwy |

Powód dzielenia na warstwy: jedna warstwa (10 kluczy + 3 enkoder, każdy z
1-3 akcjami) to ok. 400-800 bajtów zminifikowanego JSON — bezpiecznie mieści
się w pojedynczej ramce, nawet przy kilku warstwach naraz nie trzeba trzymać
całego configu w jednym buforze wejściowym.

## 5. Wymagania na stronę firmware (przypomnienie z instrukcji projektu)

- **Bez `delay()`** — tryby `1`/`2` oraz opóźnienia `d` implementowane przez
  `millis()` / timery, żeby wciśnięcie kilku klawiszy naraz nie blokowało
  reszty (symultaniczność).
- Generator pseudolosowy (`random(mn, mx+1)`) wywoływany przy każdym
  Key Down / powtórzeniu pętli, nie raz przy starcie.
- Firmware trzyma cały config w RAM (max np. 8 warstw × 13 przypisań —
  parametr do ustalenia razem z realnym budżetem pamięci na etapie Kroku 3),
  a na flash zapisuje dopiero na `{"cmd":"save"}`.
- **Natychmiastowe zwolnienie w trybie `1`/`2`**: stan klawisza (zwolniony?)
  musi być sprawdzany na **każdym** przebiegu pętli głównej, przed decyzją
  o kolejnym powtórzeniu — nie „wykonaj akcję, potem odczekaj X ms”, tylko
  „czy zwolniony? jeśli nie i minęło X ms od ostatniego powtórzenia →
  wykonaj”. Przy pętli bez blokujących wywołań zwolnienie zostanie wykryte
  w granicach czasu debounce'u (typowo 5–15 ms), niezależnie od tego, w
  którym miejscu odliczania losowego opóźnienia (`d`) akurat jesteśmy.
  `firmware/arduino/makropad_test/makropad_test.ino` już implementuje ten
  wzorzec (debounce + skan bez `delay()`) dla samego odczytu klawiszy —
  Krok 3 rozszerza go o właściwą maszynę stanów trybów aktywacji.

## 6. Hosting (GitHub Pages)

`web/index.html` jest wdrażany na GitHub Pages przez
`.github/workflows/deploy-pages.yml` — builduje artefakt Pages bezpośrednio z
folderu `web/`, z pominięciem `docs/` (tam jest dokumentacja montażu w
Markdown; Jekyll by ją niepotrzebnie przetwarzał, gdyby Pages był ustawiony
na serwowanie z `/docs`). Wymaga jednorazowego ustawienia w repo: *Settings →
Pages → Source → GitHub Actions*. Szczegóły w README, sekcja „Wdrożenie web
configuratora”. Web Serial API wymaga HTTPS — domena `github.io` to zapewnia
od razu.

## 7. BLE: parowanie i pamięć urządzeń (zakres na Krok 3)

Decyzja co do zakresu (żeby nie rozrastać Kroku 3 bez potrzeby): urządzenie
pamięta **jednego** hosta naraz. Przełączanie między kilkoma zapamiętanymi
hostami (jak w klawiaturach multi-pairing typu Logitech) jest **całkowicie
opcjonalne i odłożone na później** — nie wchodzi w zakres pierwszej wersji
firmware.

Co z tego wynika dla implementacji:

- **Pamiętanie samo w sobie nie wymaga dodatkowego kodu.** Standardowe
  zachowanie Adafruit Bluefruit: urządzenie advertisuje się, paruje z
  hostem (Just Works, bez PIN-u), bonding zapisuje się automatycznie na
  flash i **samo wraca** do tego samego hosta po każdym włączeniu, dopóki
  jest w zasięgu. To działa „za darmo” z samej biblioteki.
- **Sparowanie z innym/nowym hostem** = przytrzymanie przycisku enkodera
  (SW, D2) przez kilka sekund → `Bluefruit.Periph.clearBonds()` → powrót do
  advertisingu. To jest wzorzec „zapomnij i sparuj od nowa”, NIE „zapamiętaj
  wiele i wybierz” — prostsze w implementacji i wystarczające na teraz.
- **Nie budujemy na razie**: listy kilku zapamiętanych adresów, żadnego UI
  wyboru hosta na OLED, logiki cyklicznego przełączania między nimi. Sam
  mechanizm bondingu w Bluefruit nie ma sztywnego limitu jednego zapisu, więc
  rozszerzenie o wielo-hostowość później jest możliwe bez przebudowy reszty
  architektury — po prostu nie robimy tego teraz.

## 8. Co dalej

- **Krok 1 (ten dokument + mockup UI w `web/index.html`)** — gotowe.
- **Krok 2** — prawdziwa integracja Web Serial API w `web/index.html`
  (nawiązanie połączenia, wysyłka/odbiór wg protokołu z §4, obsługa błędów
  portu) — gotowe.
- **Krok 3** — firmware docelowy w `firmware/arduino/makropad_firmware/`
  (ścieżka Adafruit Bluefruit nRF52, nie ZMK) — kod napisany, patrz §9 po
  zakres i sposób weryfikacji.

## 9. Krok 3 — firmware: zakres i weryfikacja

Kod w `firmware/arduino/makropad_firmware/` implementuje: parser/serializer
JSON wg §3-4 (`json_codec.cpp`, ArduinoJson 6.x), maszynę stanów trybów
aktywacji z §3.1/§3.3/§5 (`macro_engine.cpp`), odczyt klawiszy/enkodera
(`key_scanner.cpp`, rozwinięcie wzorca z `makropad_test.ino`), wyjście
HID USB+BLE równolegle (`hid_output.cpp`), zapis/odczyt configu na flash
przez LittleFS/InternalFS (`storage.cpp`), parowanie BLE zgodnie z zakresem
z §7 (`ble_pairing.cpp`) oraz protokół Serial z §4 (`serial_protocol.cpp`).

**Ważne ograniczenie weryfikacji:** środowisko, w którym ten kod powstał,
nie miało dostępu do `arduino-cli` ani do pakietów płytek/bibliotek Arduino
— nie dało się go ani razu skompilować. Żeby mimo to zminimalizować ryzyko
błędów:

- Sama logika maszyny stanów makr (tryby `0`/`1`/`2`, w tym gwarancja
  natychmiastowego zatrzymania) została **najpierw zweryfikowana osobną
  symulacją w Pythonie** (poza repo) — symulacja złapała realny błąd
  projektowy (start trybu „naciśnij i puść” od poziomu sygnału zamiast
  zbocza) before przepisania na C++.
- Siedem z dziesięciu plików `.cpp` (`config`, `hid_names`, `key_scanner`,
  `macro_engine`, `hid_output`, `display`, `ble_pairing`) zostało
  przepuszczonych przez prawdziwy kompilator C++ (`clang++ -fsyntax-only`)
  z ręcznie napisanymi nagłówkami-atrapami bibliotek Arduino — złapało to
  m.in. brakujący `#include` i użycie niezadeklarowanej stałej, oba
  poprawione. Pozostałe trzy pliki (`json_codec`, `serial_protocol`,
  `storage`) zostały sprawdzone ręcznie pod kątem zgodności sygnatur funkcji
  między nagłówkami a wywołaniami.
- Mimo to **pierwsze wgranie na prawdziwą płytkę prawdopodobnie wymaga kilku
  drobnych poprawek nazw metod bibliotek** (`Adafruit_USBD_HID`,
  `BLEHidAdafruit`, `Adafruit_LittleFS`) — każdy plik, który tego dotyczy, ma
  na górze komentarz z dokładnymi założeniami i wskazówką, co poprawić. To
  normalne przy kodzie pisanym bez możliwości kompilacji, nie oznacza błędu
  w logice.

**Rzeczy celowo odłożone / uproszczone w tej wersji:**

- Przełączanie między wieloma zapamiętanymi hostami BLE — poza zakresem,
  patrz §7.
- Przerwanie z PCF8574 (`D3`) na razie tylko diagnostyczne — sleep/wake na
  przerwaniu (oszczędzanie baterii) nie jest zaimplementowane.
- Wpisywanie tekstu (`t:1`) obsługuje podstawowy układ US-QWERTY (ASCII
  32-126) — polskie znaki diakrytyczne nie są obsługiwane.
- Test fizyczny na prawdziwym sprzęcie pozostaje do zrobienia przez
  użytkownika — to jedyna forma weryfikacji, której nie da się zastąpić w
  tym środowisku.
