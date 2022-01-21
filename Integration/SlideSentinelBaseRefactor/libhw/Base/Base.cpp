#include "Base.h"
#include "network_config_2.0.0.h"

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

        // Increase the total count for number of packets we expected to have
        m_baseDiagnostics.totalPacketsExpected++;

        // Reroute data from the radio to the feather
        setMux(FeatherTxToRadioRx);

        Serial.println("[Base] Waiting for data from rovers...");
        // Wait for a packet and if it is not received increase the dropped packet count
        if(!m_RManager.waitForPacket()){
            m_baseDiagnostics.droppedPackets++;
            Serial.println("[Base] Packet was not received in the expected interval!");
            return false;
        }

        return m_RManager.readHeader();
}

bool Base::initBase(){

    // Initialize the SD card
    if(!m_sdManager.initSD()){
        Serial.println("[Base] Failed to initialize SD card!");
        return false;
    }

    // Using an if here cause more initilization will be added later
}


void Base::print_diagnostics(){

    //Print Basic Configuration Information
    Serial.println("--- Configuration ---");
    Serial.println("Base ID: " + String(m_baseInfo.id));
    Serial.println("Retry Count: " + String(m_baseInfo.init_retries));
    Serial.println("Radio Baud Rate: " + String(m_baseInfo.radioBaud));

    Serial.println("\n--- Diagnostics ---");
    Serial.println("Dropped Packets: " + String(m_baseDiagnostics.droppedPackets));
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
