#include "FreewaveRadio.h"

Freewave::Freewave(uint8_t reset, uint8_t cd, bool is_z9c) :
    m_rst( reset ), m_cd( cd ), m_z9c( is_z9c )
{
    pinMode(m_rst, OUTPUT);
    pinMode(m_cd, INPUT);
    digitalWrite(m_rst, HIGH);
}


bool Freewave::channel_busy(){
    return digitalRead(m_cd);
}

bool Freewave::getZ9C(){
    return m_z9c;
}

void Freewave::reset(){
    digitalWrite(m_rst, LOW);
    delay(2000);
    digitalWrite(m_rst, HIGH);
}