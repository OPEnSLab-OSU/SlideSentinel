#include "Base.h"
#include "network_config.h"

/* Ran on first bootup of Main*/
Base::Base() : m_max4280(MAX_CS, &SPI),
               m_multiplexer(SPDT_SEL, -1),
               m_RManager(),
               m_sdManager(SD_CS, 10){
    
    m_baseInfo.id = SERVER_ADDR;
    m_baseInfo.init_retries = INIT_RETRIES;
    m_baseInfo.timeout = INIT_TIMEOUT;
    m_baseInfo.radioBaud = RADIO_BAUD;
}

bool Base::wait_for_request(){
        // Reroute data from the radio to the feather
        setMux(FeatherTxToRadioRx);

        Serial.println("[Base] Waiting for data from rovers...");
        // Wait for a packet and if it is not received increase the dropped packet count
        if(!m_RManager.waitForPacket()){

            // Increase the dropped packet count by one
            m_baseDiagnostics.setDroppedPkts(m_baseDiagnostics.droppedPkts() + 1);
            Serial.println("[Base] Packet was not received in the expected interval!");
            return false;
        }

        return m_RManager.readHeader();
}

bool Base::initBase(){

    // Attempt to initialize the SD card
    if(!m_sdManager.initSD()){
        return false;
    }


    return true;
}


void Base::print_diagnostics(){

    //Print Basic Configuration Information
    Serial.println("\n**** Configuration ****");
    Serial.println("\tBase ID: " + String(m_baseInfo.id));
    Serial.println("\tRetry Count: " + String(m_baseInfo.init_retries));
    Serial.println("\tRadio Baud Rate: " + String(m_baseInfo.radioBaud));
    Serial.println("*************************");

    // Print out the diagnostics to serial
    m_baseDiagnostics.print_serial();
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
    if(format == RTCMOutToRadioRx){
        m_multiplexer.comY1();          
    }else if(format == FeatherTxToRadioRx){
        m_multiplexer.comY2();
    }
    

}
