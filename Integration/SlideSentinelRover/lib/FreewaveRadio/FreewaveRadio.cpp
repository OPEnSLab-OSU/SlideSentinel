#include "FreewaveRadio.h"

Freewave::Freewave(HardwareSerial* device, uint8_t reset, uint8_t cd, bool is_z9c){
    m_reset_pin = reset;
    m_cd_pin = cd;
    m_is_z9c = is_z9c;
    m_serial = device;
    pinMode(m_reset_pin, OUTPUT);
    pinMode(m_cd_pin, INPUT);
}



void Freewave::read(){
    
}


void Freewave::send(){

}


bool Freewave::channel_busy(){

}

void Freewave::reset(){

}