# CAD

Skopiuj tu swoje oryginalne pliki:

- `Dol.stl`
- `Lid_final.stl`

Skrypt `../tools/analiza_stl.py` czyta je z tego katalogu i wypisuje wszystkie wymiary,
położenia otworów i wolne przestrzenie — te same, które trafiły do
`../docs/01-analiza-obudowy.md`.

## Zmiana, którą warto rozważyć

W obudowie **nie ma otworu na USB-C**. Wycięcie ok. 10 × 5 mm w lewej ściance dna,
na wysokości gniazda XIAO (ok. X = −112, Y ≈ −80, Z = 4…9), pozwoli ładować
i wgrywać firmware bez rozkręcania.
