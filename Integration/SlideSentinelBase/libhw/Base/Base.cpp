#include "Rover.h"

/* Ran on first bootup of Main*/
Base::Base() : m_max4280(MAX_CS, &SPI){
    m_baseInfo.id = SERVER_ADDR;
    m_baseInfo.init_retries = INIT_RETRIES;
    m_baseInfo.timeout = INIT_TIMEOUT;
    m_baseInfo.radioBaud = RADIO_BAUD;
}

void Base::request(){
        
}

void Base::powerRadio(){
    Serial.println("Powering radio on.");
    m_max4280.assertRail(0);
}

void Base::powerDownRadio(){
    Serial.println("Powering radio down.");
    m_max4280.assertRail(1);
}

void Base::powerGNSS(){
    Serial.println("Powering GNSS on.");
    m_max4280.assertRail(2);
}

void Base::powerDownGNSS(){
    Serial.println("Powering GNSS down.");
    m_max4280.assertRail(3);
}

void Base::setMux(MuxFormat format){
    if(format == RadioToFeather){
        m_multiplexer.comY1();          //Radio->Feather
    }else if(format == RadioToGNSS){
        m_multiplexer.comY2();
    }

}
