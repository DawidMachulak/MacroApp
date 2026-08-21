# 05 — Kiedy coś nie działa

| Objaw | Co sprawdzić, w tej kolejności |
|---|---|
| Komputer w ogóle nie widzi XIAO | Zmień kabel na taki z transmisją danych. Potem wciśnij dwa razy szybko reset — płytka pojawi się jako pendrive. |
| Skaner I²C nie znajduje niczego | Zamienione SDA ze SCL to przyczyna numer jeden. Potem brak zasilania modułów (zmierz 3,3 V między VCC a GND wyświetlacza), na końcu zimne luty. |
| Skaner widzi tylko `0x3C` | PCF8574 nie ma zasilania albo ma inny adres. Sprawdź ciągłość VCC i GND do modułu, potem czy A0, A1, A2 dotykają masy. |
| Skaner pokazuje `0x27` zamiast `0x20` | Przewody adresowe nie łączą. Przelutuj je. |
| Skaner pokazuje adres `0x38`–`0x3F` | Masz układ PCF8574**A**. Wpisz znaleziony adres do programu. Gdyby wypadł `0x3C` — koliduje z wyświetlaczem; przełóż jeden drucik adresowy z GND na VCC. |
| Wyświetlacz świeci, nic nie pokazuje | Rozmiar w programie: moduł 0,91" to `128, 32`, nie 128, 64. |
| Jeden klawisz „wciśnięty" cały czas | Zwarcie między jego nóżkami albo kropla cyny mostkująca sąsiednie pola na PCF8574. |
| Jeden klawisz nie reaguje wcale | Przerwa. Brzęczyk między jego polem na PCF8574 a nóżką przełącznika, przy wciśniętym klawiszu. |
| Enkoder liczy w złą stronę | Zamień w programie `D0` z `D1`. Nic nie przelutowujesz. |
| Enkoder przeskakuje o dwa | Dolutuj kondensatory 100 nF (CLK–GND, DT–GND). Najczęstsza dolegliwość KY-040. |
| Nie ładuje się | Wyłącznik w pozycji OFF. Przerywa plus akumulatora, więc do ładowania musi być włączone. |
| Działa na USB, nie działa na akumulatorze | Zimny lut na `BAT+`/`BAT−` albo źle podłączony wyłącznik. Zmierz napięcie na `BAT+` przy włączonym — ma być 3,7–4,2 V. |
| Przestało działać po dociśnięciu obudowy | Prawie zawsze zimny lut: trzyma mechanicznie, nie elektrycznie. Znajdź i przelutuj ze świeżą cyną. |

## Test kontrolny przed pierwszym zasilaniem

Multimetr na brzęczyk:

- `3V3` ↔ `GND` na XIAO — **cisza**. Piszczy = zwarcie, nie podłączaj USB.
- `VCC` ↔ `GND` na każdym module — cisza.
- `3V3` XIAO ↔ `VCC` każdego modułu — **piszczy** (potwierdza skrętkę zasilania).
- `GND` XIAO ↔ masa każdego modułu i nóżka dowolnego klawisza — piszczy.
- `SDA` ↔ `SCL` — cisza.
- Każdy klawisz: sygnał ↔ masa, przy wciśnięciu piszczy, po puszczeniu cisza.
