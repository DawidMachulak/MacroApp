# 02 — Elektronika i przypisanie pinów

![schemat](../hardware/diagrams/01-schemat-polaczen.svg)

## Dlaczego akurat tak

Potrzeb jest jedenaście wejść cyfrowych (10 klawiszy + przycisk enkodera) plus dwa
szybkie wejścia na obrót enkodera. PCF8574 daje osiem. Stąd podział:

- **8 klawiszy** na ekspanderze PCF8574 (P0–P7),
- **2 klawisze** wprost na XIAO (D6, D7),
- **enkoder** (CLK, DT, SW) wprost na XIAO — obrót wymaga szybkiej reakcji, więc nie
  przepuszczamy go przez magistralę,
- **INT** z PCF8574 na D3 — dzięki temu procesor może spać i budzić się dopiero na
  naciśnięcie klawisza. Przy ogniwie 1500 mAh to różnica idąca w tygodnie czuwania.

## Mapa pinów

| Pin XIAO | Funkcja | Dokąd | Kolor |
|---|---|---|---|
| `3V3` | zasilanie modułów | VCC: OLED, PCF8574, KY-040 | czerwony |
| `GND` | masa | GND wszystkich modułów + łańcuch klawiszy | czarny |
| `D0` | enkoder CLK | KY-040 CLK | fioletowy |
| `D1` | enkoder DT | KY-040 DT | fioletowy |
| `D2` | enkoder SW | KY-040 SW | fioletowy |
| `D3` | przerwanie | PCF8574 INT | zielony |
| `D4` | I²C SDA | OLED SDA → PCF8574 SDA | niebieski |
| `D5` | I²C SCL | OLED SCL → PCF8574 SCL | żółty |
| `D6` | klawisz 9 | prawa nóżka klawisza 9 | fioletowy |
| `D7` | klawisz 10 | prawa nóżka klawisza 10 | fioletowy |
| `D8` `D9` `D10` | **wolne** | np. brzęczyk, dioda statusu | — |
| `BAT+` | zasilanie | SS12C40 → akumulator + | pomarańczowy |
| `BAT−` | zasilanie | akumulator − | czarny |
| `P0`…`P7` (PCF) | klawisze 1–8 | prawa nóżka klawiszy 1–8 | biały |

## Adresy na magistrali I²C

| Układ | Adres | Uwagi |
|---|---|---|
| OLED SSD1306 0,91" | `0x3C` | stały, 128 × 32 piksele |
| PCF8574 | `0x20` | po zwarciu A0, A1, A2 do GND |

Zworki adresowe zostały z modułu usunięte, więc punkty A0–A2 wiszą w powietrzu i adres
jest nieprzewidywalny. **Zewrzyj je krótkimi drucikami do GND** — dostajesz pewne `0x20`.

Gdyby na kostce był napis `PCF8574A`, zakres adresów to `0x38`–`0x3F`. Adres `0x3C` z tego
zakresu koliduje z wyświetlaczem — wtedy przełóż jeden drucik adresowy z GND na VCC.

## Zasilanie

Akumulator → środkowa nóżka SS12C40 → skrajna nóżka → pole `BAT+` na spodzie XIAO.
Trzecia nóżka wyłącznika zostaje nieużywana (zaizoluj albo odetnij).

Konsekwencja: **w pozycji OFF urządzenie się nie ładuje**, bo wyłącznik przerywa plus
przed ładowarką. Do ładowania musi być włączone.

Prąd ładowania XIAO jest fabrycznie ustawiony na ok. 50 mA — przy 1500 mAh to ponad
30 godzin do pełna. Na spodzie płytki jest pole lutownicze podnoszące go do 100 mA;
sprawdź dokumentację Seeed dla swojej rewizji, bo oznaczenia się zmieniały.

## Podciągnięcia i drobiazgi

- Moduły OLED i PCF8574 mają własne rezystory podciągające na SDA/SCL. Dwa komplety
  równolegle dają ok. 2,3 kΩ — przy 3,3 V to wciąż w normie, nic nie usuwaj.
- KY-040 ma podciągnięcia 10 kΩ na CLK i DT, ale **nie na SW** — ten pin ustaw
  w programie jako `INPUT_PULLUP`.
- Dwa kondensatory 100 nF (CLK–GND i DT–GND) wyraźnie ograniczają przeskoki enkodera.
- INT z PCF8574 jest typu otwarty dren, aktywny stanem niskim → D3 też `INPUT_PULLUP`.
- Wejścia PCF8574 mają słabe podciągnięcie wewnętrzne; przed odczytem wpisz `0xFF`.

## Firmware

Program testowy w `firmware/arduino/` tylko pokazuje, co jest wciskane. Żeby makropad
faktycznie wysyłał klawisze przez Bluetooth, masz dwie drogi:

- **Arduino + Adafruit Bluefruit nRF52** — biblioteka BLE HID, dostępna razem z pakietem
  płytek Seeed nRF52. Pełna kontrola, ale wszystko piszesz sam.
- **ZMK** — gotowy firmware do bezprzewodowych klawiatur na nRF52840. Warstwy, makra,
  oszczędzanie energii bez pisania kodu, ale trzeba nauczyć się jego plików konfiguracyjnych.
  Przy 10 klawiszach i enkoderze to prawdopodobnie lepszy wybór na dłuższą metę.
