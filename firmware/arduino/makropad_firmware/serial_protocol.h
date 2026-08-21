/*
 * serial_protocol.h — protokol Web Serial z docs/06-architektura-i-json.md §4
 * (linia = jedna ramka JSON + \n, 115200 bd).
 *
 * Odczyt jest niebblokujacy: kazde wywolanie serialProtocolKrok() zjada
 * TYLKO to, co juz jest w buforze UART (Serial.available()), nigdy nie
 * czeka - zgodnie z wymogiem "bez delay()" z docs/06 §5. Komenda jest
 * obslugiwana dopiero po odebraniu pelnej linii (znak '\n').
 */
#ifndef MAKROPAD_SERIAL_PROTOCOL_H
#define MAKROPAD_SERIAL_PROTOCOL_H

void serialProtocolBegin();
void serialProtocolKrok();

#endif // MAKROPAD_SERIAL_PROTOCOL_H
