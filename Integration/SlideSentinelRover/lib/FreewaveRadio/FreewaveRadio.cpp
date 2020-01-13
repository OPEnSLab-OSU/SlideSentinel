#include "FreewaveRadio.h"

Freewave::Freewave(uint8_t reset, uint8_t cd, bool is_z9c){
    m_reset_pin = reset;
    m_cd_pin = cd;
    m_is_z9c = is_z9c;
    pinMode(m_reset_pin, OUTPUT);
    pinMode(m_cd_pin, INPUT);
}


bool Freewave::channel_busy(){
    if(digitalRead(m_cd_pin))
        Serial.println("true");
    else
        Serial.println("false");
    return digitalRead(m_cd_pin);
}

void Freewave::reset(){
    digitalWrite(m_reset_pin, LOW);
    delay(2000);
    digitalWrite(m_reset_pin, HIGH);
}