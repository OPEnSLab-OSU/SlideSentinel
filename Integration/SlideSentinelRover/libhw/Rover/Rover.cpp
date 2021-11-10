#include "Rover.h"



/* Ran on first bootup of Main*/
Rover::Rover() : m_max4280(MAX_CS, &SPI){
    m_rovInfo.id = CLIENT_ADDR;
    m_rovInfo.serverAddr = SERVER_ADDR;
    m_rovInfo.init_retries = INIT_RETRIES;
    m_rovInfo.timeout = INIT_TIMEOUT;
    m_rovInfo.radioBaud = RADIO_BAUD;
}

void Rover::request(){
        
}

void Rover::powerRadio(){
    Serial.println("Powering radio on.");
    m_max4280.assertRail(0);
}

void Rover::powerDownRadio(){
    Serial.println("Powering radio down.");
    m_max4280.assertRail(1);
}

void Rover::powerGNSS(){
    Serial.println("Powering GNSS on.");
    m_max4280.assertRail(2);
}

void Rover::powerDownGNSS(){
    Serial.println("Powering GNSS down.");
    m_max4280.assertRail(3);
}

void Rover::setMux(MuxFormat format){
    if(format == RadioToFeather){

    }else if(format == RadioToGNSS){
        
    }

}