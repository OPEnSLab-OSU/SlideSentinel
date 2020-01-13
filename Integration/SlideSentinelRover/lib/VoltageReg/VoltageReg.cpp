#include "VoltageReg.h"

PoluluVoltageReg::PoluluVoltageReg(uint8_t en){
    m_en_pin = en;
    pinMode(m_en_pin, OUTPUT);
}

void PoluluVoltageReg::enable(){
    digitalWrite(m_en_pin, HIGH);
}

void PoluluVoltageReg::disable(){
    digitalWrite(m_en_pin, LOW);
}