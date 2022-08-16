#include "Base.h"


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

/**
 * Listen for a radio request from the Rover with data
 */ 
bool Base::waitForRequest(){
        // Reroute data from the radio to the feather
        setMux(FeatherTxToRadioRx);

        Serial.println("[Base] Waiting for data from rovers...");
        // Wait for a packet and if it is not received increase the dropped packet count
        if(!m_RManager.waitForPacket()){

            // Increase the dropped packet count by one
            m_baseDiagnostics.setDroppedPkts(m_baseDiagnostics.droppedPkts() + 1);
            Serial.println("[Base] Packet was not received in the expected interval!");
            return false;
        }else{
            Serial.println("[Base]Not false!!!");
        }

        return m_RManager.readHeader();
}

/**
 * Initialize all aspects of the base, currently checks the sd if it's been initialized
 */ 
bool Base::initBase(){

    // Attempt to initialize the SD card
    if(!m_sdManager.initSD()){

        return false;
    }

    return true;
}

/**
 * Check if the SD card is initialized if not reinitialize it
 */ 
void Base::checkSD(){
    if(!m_sdManager.checkSD()){
        m_sdManager.initSD();
    }
}

/**
 * Reads bytes in from the Serial bus to print out requested data
 */ 
void Base::debugInformation(){

    // Check for user input on serial to request information about the base
    if(Serial.available()){
        char cmd = Serial.read();
        if(cmd == '1'){
            printDiagnostics();
        }
    }
}

/**
 * Print the base's diagnostic and just general information to the serial bus
 */ 
void Base::printDiagnostics(){

    //Print Basic Configuration Information
    Serial.println("\n**** Configuration ****");
    Serial.println("\tBase ID: " + String(m_baseInfo.id));
    Serial.println("\tRetry Count: " + String(m_baseInfo.init_retries));
    Serial.println("\tRadio Baud Rate: " + String(m_baseInfo.radioBaud));
    Serial.println("*************************");

    // Print out the diagnostics to serial
    m_baseDiagnostics.print_serial();
}

/**
 * Print out the most recent rover packet 
 */ 
void Base::printMostRecentPacket(){
    Serial.println("\n**** Rover Packet ****");
    Serial.println("Rover Addr: " + String(m_RManager.getMostRecentRover()));
    serializeJsonPretty(m_RManager.getRoverPacket(), Serial);
    Serial.println("*************************");
}

/**
 * Power on the radio to transmit data
 */ 
void Base::powerRadio(){
    Serial.println("[Base] Powering radio on.");
    m_max4280.assertRail(0);
}

/**
 * Power down the radio to conserve energy
 */ 
void Base::powerDownRadio(){
    Serial.println("[Base] Powering radio down.");
    m_max4280.assertRail(1);
}

/**
 * Power up the GNSS board to get fixes 
 */ 
void Base::powerGNSS(){
    Serial.println("[Base] Powering GNSS on.");
    m_max4280.assertRail(2);
}

/**
 * Power down the GNSS board
 */ 
void Base::powerDownGNSS(){
    Serial.println("[Base] Powering GNSS down.");
    m_max4280.assertRail(3);
}

/**
 * Set the current path the multiplexer is using is it feather ot radio or RTC to Radio
 */ 
void Base::setMux(MuxFormat format){
    if(format == RTCMOutToRadioRx){
        m_multiplexer.comY1();          
    }else if(format == FeatherTxToRadioRx){
        m_multiplexer.comY2();
    }
    

}
