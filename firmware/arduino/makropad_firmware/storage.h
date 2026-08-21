/*
 * storage.h — zapis/odczyt configu na flash (LittleFS/InternalFS, wbudowany
 * w rdzen Adafruit nRF52 - docs/06 §5: "a na flash zapisuje dopiero na
 * {"cmd":"save"}").
 *
 * UWAGA: podobnie jak hid_output.cpp, ten modul opiera sie na API
 * (`Adafruit_LittleFS`, `InternalFS`, `Adafruit_LittleFS_Namespace::File`),
 * ktorego nie dalo sie tu skompilowac ani zweryfikowac - patrz naglowek
 * storage.cpp po dokladne zalozenia i co poprawic, jesli kompilator sie
 * poskarzy.
 */
#ifndef MAKROPAD_STORAGE_H
#define MAKROPAD_STORAGE_H

// Wolane raz w setup(): montuje system plikow, a jesli istnieje zapisany
// config, wczytuje go do globalnego `config` (config.h). Jesli pliku nie
// ma (pierwsze uruchomienie) albo jest uszkodzony/niepoprawny, zostawia
// (lub ustawia) bezpieczny config domyslny (zbudujConfigDomyslny) i zwraca
// false.
bool storageBegin();

// Zapisuje biezacy globalny `config` na flash. Zwraca false przy bledzie
// (np. brak miejsca) - wywolujacy (serial_protocol.cpp) odpowiada wtedy
// hostowi ramka "err".
bool storageZapiszConfig();

#endif // MAKROPAD_STORAGE_H
