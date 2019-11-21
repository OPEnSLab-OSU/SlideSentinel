#include "MAX3243.h"

MAX3243::MAX3243(uint8_t forceoff_n) {
    m_forceoff_pin = forceoff_n; 
    pinMode(m_forceoff_pin, OUTPUT);
}

void MAX3243::enable() {
    digitalWrite(m_forceoff_pin, HIGH);
}

void MAX3243::disable() {
    digitalWrite(m_forceoff_pin, LOW);
}


