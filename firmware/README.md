# Firmware

## Przygotowanie Arduino IDE

1. `Plik → Ustawienia → Dodatkowe adresy URL do menedżera płytek`, dodaj:
   `https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json`
   (jeśli adres nie zadziała, sprawdź aktualny w dokumentacji Seeed Studio)
2. `Narzędzia → Płytka → Menedżer płytek` → zainstaluj **Seeed nRF52 Boards**
3. Wybierz płytkę **Seeed XIAO nRF52840**
4. `Narzędzia → Zarządzaj bibliotekami` → zainstaluj:
   - **PCF8574** (Rob Tillaart)
   - **Adafruit SSD1306**
   - **Adafruit GFX Library**
   - **ArduinoJson** (Benoit Blanchon) — **wersja 6.x** (nie 7.x, patrz uwaga
     w `arduino/makropad_firmware/json_codec.h`)
   - **Adafruit TinyUSB Library** — zwykle instaluje się automatycznie razem
     z pakietem płytek Seeed nRF52 (USB HID)
   - **Adafruit nRF52 Bluefruit** i **Adafruit LittleFS** — jw., razem z
     pakietem płytek (BLE HID, zapis configu na flash)

Jeśli płytka nie pojawia się na liście portów — wciśnij dwa razy szybko reset na XIAO.
Wejdzie w tryb bootloadera i zgłosi się jako pamięć masowa.

## Programy

| Katalog | Co robi |
|---|---|
| `arduino/i2c_scanner/` | Wypisuje adresy znalezione na magistrali. Oczekiwane: `0x3C` i `0x20`. |
| `arduino/makropad_test/` | Pokazuje wciśnięte klawisze i licznik enkodera. Diagnostyka wszystkich połączeń. |
| `arduino/makropad_firmware/` | **Firmware docelowy** (Krok 3): BLE/USB HID, logika makr, warstwy, parowanie, zapis na flash. Patrz `docs/06-architektura-i-json.md` §9 po zakres i ograniczenia weryfikacji. |

## Pierwsze wgranie `makropad_firmware`

Ten firmware nie mógł zostać skompilowany w środowisku, w którym powstał
(brak `arduino-cli`/pakietów płytek) — logika (maszyna stanów makr) została
zweryfikowana symulacją i częściowym sprawdzeniem kompilatorem C++ ze
sztucznymi nagłówkami (opisane w `docs/06-architektura-i-json.md` §9), ale
**pierwsze prawdziwe wgranie prawdopodobnie wymaga kilku drobnych poprawek**
nazw metod w `hid_output.cpp`, `storage.cpp` i `ble_pairing.cpp` — każdy z
tych plików ma na górze komentarz z dokładnymi założeniami i wskazówką, co
poprawić, jeśli kompilator się poskarży (najczęściej: `'xxx' was not
declared` → sprawdź podpowiedzi autouzupełniania Arduino IDE dla tej klasy i
podmień nazwę w tym jednym pliku). To normalne przy kodzie pisanym bez
możliwości kompilacji.

Zalecana kolejność sprawdzania po pierwszym `Sketch → Verify/Compile`:

1. Błędy w `json_codec.cpp`/`serial_protocol.cpp`/`storage.cpp` (ArduinoJson) —
   najbardziej prawdopodobna przyczyna to zainstalowana wersja 7.x zamiast 6.x.
2. Błędy w `hid_output.cpp` (`Adafruit_USBD_HID`, `BLEHidAdafruit`) — sprawdź
   dokładne nazwy metod w zainstalowanej wersji biblioteki.
3. Błędy w `storage.cpp` (`Adafruit_LittleFS`/`InternalFS`) — porównaj z
   przykładem `Plik → Przykłady → Adafruit LittleFS → internal_flash`.

## Alternatywa: ZMK

**ZMK** to gotowy firmware do bezprzewodowych klawiatur na nRF52840 (warstwy,
makra, oszczędzanie energii bez pisania kodu C++, ale wymaga nauczenia się
plików konfiguracyjnych YAML/devicetree). `makropad_firmware` powyżej idzie
ścieżką Adafruit Bluefruit (pełna kontrola, cała logika w C++) — jeśli wolisz
ZMK zamiast tego, to osobna, niezależna ścieżka niekorzystająca z żadnego
kodu w tym repo poza schematem połączeń.
