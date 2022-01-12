#ifndef UNITTEST_TRANSPORT_H
#define UNITTEST_TRANSPORT_H

#ifdef ARDUINO

#include "Arduino.h"

void unittest_uart_begin() {
    Serial.begin(115200);
    while (!Serial)
        yield();
}

void unittest_uart_putchar(char c) {
    Serial.write(c);
}

void unittest_uart_flush() {
    Serial.flush();
}

void unittest_uart_end() {
    Serial.end();
}

#else

#include <stdio.h>

void unittest_uart_begin() {

}

void unittest_uart_putchar(char c) {
  putchar(c);
}

void unittest_uart_flush() {
  fflush(stdout);
}

void unittest_uart_end() {

}

#endif

#endif