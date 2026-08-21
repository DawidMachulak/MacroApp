# 01 — Analiza obudowy (z plików STL)

Wszystkie wymiary odczytane bezpośrednio z `Dol.stl` i `Lid_final.stl`.
Skrypt, który je wylicza: `tools/analiza_stl.py`.

## Gabaryty

| Wielkość | Wartość |
|---|---|
| Wymiar zewnętrzny | 109,8 × 87,75 mm |
| Wysokość całkowita | ok. 25,4 mm (z = 0,9 … 26,31) |
| Dno — zakres Z | 0,9 … 12,9 mm (podłoga na z = 2,9, czyli 2 mm grubości) |
| Pokrywa — zakres Z | 12,9 … 26,31 mm |
| **Prześwit wewnętrzny** | **12,0 mm** (od podłogi 2,9 do spodu pokrywy 14,9) |
| Grubość płyty pod klawisze | 2,4 mm (14,9 … 17,3) |
| Objętość materiału | dno 35,1 cm³, pokrywa 25,6 cm³ |
| Szczelność siatki | obie części watertight — nadają się do cięcia bez naprawy |

## Pokrywa — otwory i cechy

Współrzędne w układzie pliku STL (X rośnie w prawo, Y w górę).

| Cecha | Środek (X, Y) | Rozmiar |
|---|---|---|
| Klawisz 1 | −96,59 / −98,76 | 14 × 14 mm |
| Klawisz 2 | −76,59 / −98,76 | 14 × 14 mm |
| Klawisz 3 | −56,59 / −98,76 | 14 × 14 mm |
| Klawisz 4 | −36,59 / −98,76 | 14 × 14 mm |
| Klawisz 5 | −16,59 / −98,76 | 14 × 14 mm |
| Klawisz 6 | −96,59 / −118,76 | 14 × 14 mm |
| Klawisz 7 | −76,59 / −118,76 | 14 × 14 mm |
| Klawisz 8 | −56,59 / −118,76 | 14 × 14 mm |
| Klawisz 9 | −36,59 / −118,76 | 14 × 14 mm |
| Klawisz 10 | −16,59 / −118,76 | 14 × 14 mm |
| Okno OLED | −64,18 / −64,50 | 45 × 19 mm (w podwyższonym kołnierzu do z = 26,3) |
| Otwór enkodera | −17,29 / −68,43 | Ø 7,0 mm |
| Szczelina suwaka SS12C40 | −106,2 / −56,3 | 4,2 × 9,9 mm, w kieszeni sięgającej z = 24 |
| Otwór śruby, lewy | −86,58 / −108,76 | Ø 5,5 mm |
| Otwór śruby, prawy | −26,58 / −108,76 | Ø 5,5 mm |

- **Raster klawiszy: 20 mm** w obu osiach (standard MX to 19,05 mm — keycapy będą stały odrobinę rzadziej).
- Wycięcia mają w przekroju 14 × 16 mm, czyli 14 × 14 z wcięciami na zatrzaski.
- **Płyta ma 2,4 mm**, a zatrzaski MX są projektowane pod 1,5 mm — mogą nie zaskoczyć. Patrz krok 4 instrukcji.

## Dno — cechy wewnętrzne

| Cecha | Zakres | Uwagi |
|---|---|---|
| Komora na akumulator | X −92 … −27, Y −85 … −45 | ok. 65 × 40 × 10 mm; LP803450 (50 × 34 × 8) wchodzi z zapasem |
| Żebro działowe | X ≈ −83,4, Y −79 … −54 | grubość 2 mm, wysokość do z = 12,9 |
| Wolny pas przy lewej ściance | X −112 … −92 | szerokość 20 mm, pełna wysokość — miejsce na XIAO |
| Wolny pas przy prawej ściance | X −27 … −2,2 | szerokość 25 mm; częściowo zajęty przez moduł KY-040 |
| Słupki śrub | −86,58 / −108,76 oraz −26,58 / −108,76 | 7 × 7 mm, otwór Ø 4,24 mm — **pod wtopki M3** |
| Przestrzeń pod polem klawiszy | z = 2,9 … ok. 9,0 | ok. **6 mm** wolnego pod nóżkami MX — tu leży PCF8574 |

## Czego w obudowie NIE ma

Przekroje wszystkich czterech ścianek w obu częściach są pełne — **nie ma otworu na USB-C**.
Ładowanie i wgrywanie firmware'u wymaga rozkręcenia. Jeśli to przeszkadza, dodaj w modelu
wycięcie ok. 10 × 5 mm w lewej ściance dna, na wysokości gniazda XIAO.
