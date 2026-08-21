#include "hid_names.h"
#include "config.h"             // MAKROPAD_MAX_DL_TRESCI_AKCJI
#include <Adafruit_TinyUSB.h>   // HID_KEY_* — patrz uwaga w hid_names.h
#include <string.h>
#include <stdlib.h>

uint8_t modyfikatorZNazwy(const char *token) {
  if (strcmp(token, "LCTRL")  == 0) return MOD_LCTRL;
  if (strcmp(token, "RCTRL")  == 0) return MOD_RCTRL;
  if (strcmp(token, "LSHIFT") == 0) return MOD_LSHIFT;
  if (strcmp(token, "RSHIFT") == 0) return MOD_RSHIFT;
  if (strcmp(token, "LALT")   == 0) return MOD_LALT;
  if (strcmp(token, "RALT")   == 0) return MOD_RALT;
  if (strcmp(token, "LGUI")   == 0) return MOD_LGUI;
  if (strcmp(token, "RGUI")   == 0) return MOD_RGUI;
  return 0;
}

uint8_t kodKlawiszaZNazwy(const char *token) {
  size_t dl = strlen(token);

  // Pojedyncza litera A-Z — kody HID_KEY_A..HID_KEY_Z sa sekwencyjne wg
  // specyfikacji USB HID Usage Tables (strona "Keyboard/Keypad").
  if (dl == 1 && token[0] >= 'A' && token[0] <= 'Z') {
    return HID_KEY_A + (token[0] - 'A');
  }

  // Pojedyncza cyfra — UWAGA, w USB HID kolejnosc to 1,2,...,9,0 (nie
  // 0,1,...,9), wiec '0' trzeba obsluzyc osobno.
  if (dl == 1 && token[0] >= '0' && token[0] <= '9') {
    if (token[0] == '0') return HID_KEY_0;
    return HID_KEY_1 + (token[0] - '1');
  }

  // F1-F12 i F13-F24 — dwie oddzielne sekwencje w tablicy USB HID (nie
  // stykaja sie ze soba), stad dwa oddzielne przypadki.
  if (dl >= 2 && dl <= 3 && token[0] == 'F') {
    int n = atoi(token + 1);
    if (n >= 1 && n <= 12)  return HID_KEY_F1  + (n - 1);
    if (n >= 13 && n <= 24) return HID_KEY_F13 + (n - 13);
  }

  if (strcmp(token, "ESC")       == 0) return HID_KEY_ESCAPE;
  if (strcmp(token, "TAB")       == 0) return HID_KEY_TAB;
  if (strcmp(token, "ENTER")     == 0) return HID_KEY_ENTER;
  if (strcmp(token, "SPACE")     == 0) return HID_KEY_SPACE;
  if (strcmp(token, "BACKSPACE") == 0) return HID_KEY_BACKSPACE;
  if (strcmp(token, "DEL")       == 0) return HID_KEY_DELETE;
  if (strcmp(token, "UP")        == 0) return HID_KEY_ARROW_UP;
  if (strcmp(token, "DOWN")      == 0) return HID_KEY_ARROW_DOWN;
  if (strcmp(token, "LEFT")      == 0) return HID_KEY_ARROW_LEFT;
  if (strcmp(token, "RIGHT")     == 0) return HID_KEY_ARROW_RIGHT;
  if (strcmp(token, "HOME")      == 0) return HID_KEY_HOME;
  if (strcmp(token, "END")       == 0) return HID_KEY_END;
  if (strcmp(token, "PGUP")      == 0) return HID_KEY_PAGE_UP;
  if (strcmp(token, "PGDN")      == 0) return HID_KEY_PAGE_DOWN;

  // Dodatkowe znaki interpunkcyjne uzywane m.in. przez wpisywanie tekstu
  // (asciiNaHid nizej) — poza podstawowa lista z docs/06 §3.4, ale przydatne
  // przy recznym wpisaniu kombinacji w configuratorze.
  if (strcmp(token, "MINUS")      == 0) return HID_KEY_MINUS;
  if (strcmp(token, "EQUAL")      == 0) return HID_KEY_EQUAL;
  if (strcmp(token, "LBRACKET")   == 0) return HID_KEY_BRACKET_LEFT;
  if (strcmp(token, "RBRACKET")   == 0) return HID_KEY_BRACKET_RIGHT;
  if (strcmp(token, "BACKSLASH")  == 0) return HID_KEY_BACKSLASH;
  if (strcmp(token, "SEMICOLON")  == 0) return HID_KEY_SEMICOLON;
  if (strcmp(token, "QUOTE")      == 0) return HID_KEY_APOSTROPHE;
  if (strcmp(token, "GRAVE")      == 0) return HID_KEY_GRAVE;
  if (strcmp(token, "COMMA")      == 0) return HID_KEY_COMMA;
  if (strcmp(token, "PERIOD")     == 0) return HID_KEY_PERIOD;
  if (strcmp(token, "SLASH")      == 0) return HID_KEY_SLASH;

  return 0; // nieznany token — pomijany przez wywolujacego
}

uint8_t rozbierzKombinacje(const char *kombinacja, uint8_t &modyfikatory, uint8_t kody[6]) {
  modyfikatory = 0;
  uint8_t liczbaKodow = 0;

  // Kopia robocza — strtok modyfikuje bufor, a `kombinacja` jest tu const
  // (wskazuje wprost na Akcja::c z configu w RAM, ktorego nie chcemy ruszac).
  char bufor[MAKROPAD_MAX_DL_TRESCI_AKCJI + 1];
  strncpy(bufor, kombinacja, sizeof(bufor) - 1);
  bufor[sizeof(bufor) - 1] = '\0';

  char *token = strtok(bufor, "+");
  while (token != nullptr) {
    uint8_t bitMod = modyfikatorZNazwy(token);
    if (bitMod != 0) {
      modyfikatory |= bitMod;
    } else if (liczbaKodow < 6) {
      uint8_t kod = kodKlawiszaZNazwy(token);
      if (kod != 0) kody[liczbaKodow++] = kod;
      // nieznany token: pomijamy po cichu (walidacja "miekka" jest juz
      // zrobiona po stronie web/index.html; firmware ma byc odporne, nie
      // ma po co blokowac calej sekwencji przez jeden literowka)
    }
    token = strtok(nullptr, "+");
  }
  return liczbaKodow;
}

bool asciiNaHid(char znak, uint8_t &modyfikator, uint8_t &kod) {
  modyfikator = 0;

  if (znak >= 'a' && znak <= 'z') { kod = HID_KEY_A + (znak - 'a'); return true; }
  if (znak >= 'A' && znak <= 'Z') { kod = HID_KEY_A + (znak - 'A'); modyfikator = MOD_LSHIFT; return true; }
  if (znak >= '1' && znak <= '9') { kod = HID_KEY_1 + (znak - '1'); return true; }

  switch (znak) {
    case '0':  kod = HID_KEY_0;            return true;
    case ' ':  kod = HID_KEY_SPACE;        return true;
    case '\n': kod = HID_KEY_ENTER;        return true;
    case '\t': kod = HID_KEY_TAB;          return true;
    case '-':  kod = HID_KEY_MINUS;        return true;
    case '=':  kod = HID_KEY_EQUAL;        return true;
    case '[':  kod = HID_KEY_BRACKET_LEFT;  return true;
    case ']':  kod = HID_KEY_BRACKET_RIGHT; return true;
    case '\\': kod = HID_KEY_BACKSLASH;    return true;
    case ';':  kod = HID_KEY_SEMICOLON;    return true;
    case '\'': kod = HID_KEY_APOSTROPHE;   return true;
    case '`':  kod = HID_KEY_GRAVE;        return true;
    case ',':  kod = HID_KEY_COMMA;        return true;
    case '.':  kod = HID_KEY_PERIOD;       return true;
    case '/':  kod = HID_KEY_SLASH;        return true;

    // Znaki wymagajace Shift na standardowym ukladzie US-QWERTY (ten sam
    // fizyczny klawisz co odpowiednik bez Shift powyzej).
    case '!':  kod = HID_KEY_1;            modyfikator = MOD_LSHIFT; return true;
    case '@':  kod = HID_KEY_2;            modyfikator = MOD_LSHIFT; return true;
    case '#':  kod = HID_KEY_3;            modyfikator = MOD_LSHIFT; return true;
    case '$':  kod = HID_KEY_4;            modyfikator = MOD_LSHIFT; return true;
    case '%':  kod = HID_KEY_5;            modyfikator = MOD_LSHIFT; return true;
    case '^':  kod = HID_KEY_6;            modyfikator = MOD_LSHIFT; return true;
    case '&':  kod = HID_KEY_7;            modyfikator = MOD_LSHIFT; return true;
    case '*':  kod = HID_KEY_8;            modyfikator = MOD_LSHIFT; return true;
    case '(':  kod = HID_KEY_9;            modyfikator = MOD_LSHIFT; return true;
    case ')':  kod = HID_KEY_0;            modyfikator = MOD_LSHIFT; return true;
    case '_':  kod = HID_KEY_MINUS;        modyfikator = MOD_LSHIFT; return true;
    case '+':  kod = HID_KEY_EQUAL;        modyfikator = MOD_LSHIFT; return true;
    case '{':  kod = HID_KEY_BRACKET_LEFT;  modyfikator = MOD_LSHIFT; return true;
    case '}':  kod = HID_KEY_BRACKET_RIGHT; modyfikator = MOD_LSHIFT; return true;
    case '|':  kod = HID_KEY_BACKSLASH;    modyfikator = MOD_LSHIFT; return true;
    case ':':  kod = HID_KEY_SEMICOLON;    modyfikator = MOD_LSHIFT; return true;
    case '"':  kod = HID_KEY_APOSTROPHE;   modyfikator = MOD_LSHIFT; return true;
    case '~':  kod = HID_KEY_GRAVE;        modyfikator = MOD_LSHIFT; return true;
    case '<':  kod = HID_KEY_COMMA;        modyfikator = MOD_LSHIFT; return true;
    case '>':  kod = HID_KEY_PERIOD;       modyfikator = MOD_LSHIFT; return true;
    case '?':  kod = HID_KEY_SLASH;        modyfikator = MOD_LSHIFT; return true;
  }

  return false; // znak spoza podstawowego US-QWERTY — pomijany przy wpisywaniu
}

uint16_t kodMediaZNazwy(const char *nazwa) {
  // Kody z tablicy USB HID Usage "Consumer" (strona 0x0C) — nazwy stalych w
  // Adafruit TinyUSB, patrz uwaga na gorze hid_names.h.
  if (strcmp(nazwa, "VOL_UP")     == 0) return HID_USAGE_CONSUMER_VOLUME_INCREMENT;
  if (strcmp(nazwa, "VOL_DOWN")   == 0) return HID_USAGE_CONSUMER_VOLUME_DECREMENT;
  if (strcmp(nazwa, "MUTE")       == 0) return HID_USAGE_CONSUMER_MUTE;
  if (strcmp(nazwa, "PLAY_PAUSE") == 0) return HID_USAGE_CONSUMER_PLAY_PAUSE;
  if (strcmp(nazwa, "NEXT_TRACK") == 0) return HID_USAGE_CONSUMER_SCAN_NEXT;
  if (strcmp(nazwa, "PREV_TRACK") == 0) return HID_USAGE_CONSUMER_SCAN_PREVIOUS;
  return 0;
}
