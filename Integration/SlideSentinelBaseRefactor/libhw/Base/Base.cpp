#include "Base.h"
#include "network_config_2.0.0.h"

/* Ran on first bootup of Main*/
Base::Base() : m_max4280(MAX_CS, &SPI),
               m_multiplexer(SPDT_SEL, -1){
    
    m_baseInfo.id = SERVER_ADDR;
    m_baseInfo.init_retries = INIT_RETRIES;
    m_baseInfo.timeout = INIT_TIMEOUT;
    m_baseInfo.radioBaud = RADIO_BAUD;
}

void Base::wait_for_request(){
        // Reroute data from the radio to the feather
        setMux(RadioToFeather);
}


void Base::print_diagnostics(){

    //Print Basic Configuration Information
    Serial.println("Configuration: ");
    Serial.println("Base ID: " + String(m_baseInfo.id));
    Serial.println("Retry Count: " + String(m_baseInfo.init_retries));
    Serial.println("Radio Baud Rate: " + String(m_baseInfo.radioBaud));

    //TODO: Print current diagnostic info
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
