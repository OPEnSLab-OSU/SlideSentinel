#include "SN74LVC2G53.h"

SN74LVC2G53::SN74LVC2G53(uint8_t sel, uint8_t inh) {
    m_sel_pin = sel; 
    m_inh_pin = inh;
    pinMode(sel, OUTPUT);
    if(m_inh_pin >= 0){
        pinMode(m_inh_pin, OUTPUT);
    }
}

void SN74LVC2G53::comY1() {
    digitalWrite(m_sel_pin, LOW);
}

void SN74LVC2G53::comY2() {
    digitalWrite(m_sel_pin, HIGH);
}

void SN74LVC2G53::enable() {
    digitalWrite(m_inh_pin, LOW);
}

void SN74LVC2G53::disable() {
    digitalWrite(m_inh_pin, HIGH);
}
