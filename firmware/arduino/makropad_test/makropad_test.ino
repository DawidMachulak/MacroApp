/*
 * Program testowy makropada - krok 15 instrukcji.
 *
 * Plytka:     Seeed XIAO nRF52840
 * Biblioteki: PCF8574 (Rob Tillaart), Adafruit SSD1306, Adafruit GFX
 * Monitor portu szeregowego: 115200
 *
 * Pokazuje na wyswietlaczu i na porcie szeregowym, ktore klawisze sa
 * wcisniete oraz licznik obrotow enkodera. Nie wysyla jeszcze nic przez
 * Bluetooth - to tylko diagnostyka polaczen.
 *
 * WAZNE: petla nie uzywa delay() - wymog projektu (docs/06 §5), zeby
 * zwolnienie klawisza w przyszlym trybie "przytrzymaj" bylo wykrywane
 * natychmiast, a nie dopiero po odczekaniu sztywnego opoznienia. Skanowanie
 * klawiszy dziala wiec z pelna predkoscia petli glownej; ograniczone jest
 * tylko odswiezanie wyswietlacza/Serial (patrz OLED_ODSWIEZ_MS), zeby nie
 * zapychac magistrali I2C przy kazdym przebiegu.
 *
 * Jesli enkoder liczy w odwrotna strone, zamien ponizej PIN_ENC_A z PIN_ENC_B.
 */
#include <Wire.h>
#include <PCF8574.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define ADRES_PCF   0x20      // po zwarciu A0, A1, A2 do GND
#define ADRES_OLED  0x3C
#define OLED_SZER   128
#define OLED_WYS    32        // modul 0.91 cala ma 32, nie 64

// Czas, przez jaki odczyt musi byc stabilny, zeby uznac go za prawdziwa
// zmiane stanu (odfiltrowuje drgania stykow mechanicznych klawiszy).
#define DEBOUNCE_MS       8
// Maksymalna czestosc odswiezania OLED/Serial - same odczyty klawiszy i
// tak dzieja sie co kazdy przebieg petli, to ogranicza tylko rysowanie.
#define OLED_ODSWIEZ_MS   80

const int PIN_ENC_A  = D0;    // enkoder CLK
const int PIN_ENC_B  = D1;    // enkoder DT
const int PIN_ENC_SW = D2;    // enkoder - przycisk
const int PIN_INT    = D3;    // przerwanie z PCF8574 (na razie tylko odczytywany
                               // jako wejscie diagnostyczne - obsluga sleep/wake
                               // na przerwaniu to zadanie Kroku 3, nie tego testu)
const int PIN_KLAW9  = D6;
const int PIN_KLAW10 = D7;

PCF8574 pcf(ADRES_PCF);
Adafruit_SSD1306 oled(OLED_SZER, OLED_WYS, &Wire, -1);

const uint8_t LICZBA_KLAWISZY = 10; // 0-7 = PCF8574 P0-P7, 8 = klawisz 9, 9 = klawisz 10

bool          stanSurowyKlaw[LICZBA_KLAWISZY]   = { false };
bool          stanStabilnyKlaw[LICZBA_KLAWISZY] = { false };
unsigned long czasZmianyKlaw[LICZBA_KLAWISZY]   = { 0 };

bool          swSurowy = false, swStabilny = false;
unsigned long swCzasZmiany = 0;

// Tabela przejsc kwadratury (Gray code) - klasyczna, sprawdzona metoda
// dekodowania enkoderow inkrementalnych. Kazde poprawne przejscie miedzy
// stanami CLK/DT daje +1 albo -1 "cwiartki"; jeden mechaniczny "klik"
// typowego KY-040 to 4 cwiartki, stad dzielenie przez 4 nizej.
const int8_t TABELA_KWADRATURY[16] = {
   0, -1,  1,  0,
   1,  0,  0, -1,
  -1,  0,  0,  1,
   0,  1, -1,  0
};
uint8_t stanKwadratury     = 0;
int16_t akumulatorCwiartek = 0;

long licznikEnkodera = 0;
unsigned long ostatnieOdswiezenieOled = 0;
bool zmianaDoWyswietlenia = true; // wymus pierwsze rysowanie po starcie

void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(PIN_ENC_A,  INPUT_PULLUP);
  pinMode(PIN_ENC_B,  INPUT_PULLUP);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);
  pinMode(PIN_INT,    INPUT_PULLUP);
  pinMode(PIN_KLAW9,  INPUT_PULLUP);
  pinMode(PIN_KLAW10, INPUT_PULLUP);

  if (!pcf.begin(0xFF)) {          // wszystkie osiem pinow jako wejscia
    Serial.println("Nie widze PCF8574 pod 0x20 - sprawdz zasilanie i zworki adresowe.");
  }

  if (!oled.begin(SSD1306_SWITCHCAPVCC, ADRES_OLED)) {
    Serial.println("Nie widze wyswietlacza pod 0x3C - sprawdz SDA/SCL.");
  }
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);

  // Stan poczatkowy kwadratury, zeby pierwszy odczyt w loop() nie zaliczyl
  // falszywego przejscia.
  stanKwadratury = (digitalRead(PIN_ENC_A) << 1) | digitalRead(PIN_ENC_B);
}

// Aktualizuje debounce jednego wejscia binarnego. Zwraca true, jesli
// stabilny (odfiltrowany) stan zmienil sie wlasnie w tym wywolaniu.
bool aktualizujDebounce(bool surowy, bool &pamSurowy, bool &pamStabilny,
                         unsigned long &czasZmiany, unsigned long teraz) {
  if (surowy != pamSurowy) {
    pamSurowy  = surowy;
    czasZmiany = teraz;
  }
  if (pamStabilny != pamSurowy && (teraz - czasZmiany) >= DEBOUNCE_MS) {
    pamStabilny = pamSurowy;
    return true;
  }
  return false;
}

void loop() {
  unsigned long teraz = millis();

  // --- klawisze: 8x PCF8574 + 2x wprost na XIAO -------------------------
  uint8_t stanPcf = pcf.read8();     // bit = 0 oznacza wcisniety klawisz
  bool surowe[LICZBA_KLAWISZY];
  for (uint8_t i = 0; i < 8; i++) surowe[i] = !(stanPcf & (1 << i));
  surowe[8] = (digitalRead(PIN_KLAW9)  == LOW);
  surowe[9] = (digitalRead(PIN_KLAW10) == LOW);

  for (uint8_t i = 0; i < LICZBA_KLAWISZY; i++) {
    if (aktualizujDebounce(surowe[i], stanSurowyKlaw[i], stanStabilnyKlaw[i],
                            czasZmianyKlaw[i], teraz)) {
      zmianaDoWyswietlenia = true;
    }
  }

  // --- przycisk enkodera --------------------------------------------------
  if (aktualizujDebounce(digitalRead(PIN_ENC_SW) == LOW, swSurowy, swStabilny,
                          swCzasZmiany, teraz)) {
    zmianaDoWyswietlenia = true;
  }

  // --- obrot enkodera: pelna kwadratura, bez debounce'u czasowego --------
  // (rezystory/kondensatory na CLK/DT filtruja drgania sprzetowo; patrz
  // docs/02-elektronika-i-piny.md - filtr 100 nF). Sprawdzane co kazdy
  // przebieg petli, zeby nie zgubic szybkiego obrotu.
  stanKwadratury = ((stanKwadratury << 2) | (digitalRead(PIN_ENC_A) << 1) | digitalRead(PIN_ENC_B)) & 0x0F;
  akumulatorCwiartek += TABELA_KWADRATURY[stanKwadratury];
  if (akumulatorCwiartek >= 4) {
    licznikEnkodera++;
    akumulatorCwiartek = 0;
    zmianaDoWyswietlenia = true;
  } else if (akumulatorCwiartek <= -4) {
    licznikEnkodera--;
    akumulatorCwiartek = 0;
    zmianaDoWyswietlenia = true;
  }

  // --- rysowanie: tylko gdy jest zmiana i minelo okno throttlingu --------
  if (zmianaDoWyswietlenia && (teraz - ostatnieOdswiezenieOled >= OLED_ODSWIEZ_MS)) {
    String lista = "";
    for (uint8_t i = 0; i < LICZBA_KLAWISZY; i++) {
      if (stanStabilnyKlaw[i]) lista += String(i + 1) + " ";
    }
    if (swStabilny) lista += "ENC ";

    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.println("Klawisze:");
    oled.println(lista);
    oled.println("Enkoder: " + String(licznikEnkodera));
    oled.display();

    Serial.println(lista + "| enkoder " + String(licznikEnkodera));

    ostatnieOdswiezenieOled = teraz;
    zmianaDoWyswietlenia = false;
  }
}
