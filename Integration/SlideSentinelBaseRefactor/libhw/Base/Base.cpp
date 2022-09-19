#include "Base.h"


/* Ran on first bootup of Main*/
Base::Base() : m_max4820(MAX_CS, &SPI),
               m_multiplexer(SPDT_SEL, -1),
               m_RadioManager(),
               m_sdManager(SD_CS, 10),
               m_JSONData(1024){
    
    m_baseInfo.id = SERVER_ADDR;
    m_baseInfo.init_retries = INIT_RETRIES;
    m_baseInfo.timeout = INIT_TIMEOUT;
    m_baseInfo.radioBaud = RADIO_BAUD;
}

/**
 * Listen for a radio request from the Rover with data
 */ 
bool Base::waitForRequest(){

        Serial.println("[Base] Waiting for data from rovers...");
        // Wait for a packet and if it is not received increase the dropped packet count
        if(!m_RadioManager.waitForPacket()){

            // Increase the dropped packet count by one
            m_baseDiagnostics.setDroppedPkts(m_baseDiagnostics.droppedPkts() + 1);
            Serial.println("[Base] Packet was not received in the expected interval!");
            return false;
        }
        else{
            Serial.println("[Base] Packet Received!");
            //printMostRecentPacket();
        }

       return m_RadioManager.readHeader();
}

/**
 * Initialize all aspects of the base, currently checks the sd if it's been initialized
 */ 
bool Base::initBase(){

    // Switch the mux to communicate with the radio
    setMux(FeatherTxToRadioRx);



    // Initialize the radio
    m_RadioManager.initRadio();

    // Attempt to initialize the SD card
    if(!m_sdManager.initSD()){

        return false;
    }

    return true;
}

/**
 * Package data into a Json document to send to the Rover
 */ 
void Base::packageData(DataType packType){
    JsonObject RHJson = m_JSONData.to<JsonObject>();

    switch(packType){
        case REQUEST: 
            RHJson["TYPE"] = "REQUEST";
            RHJson["MSG"] = "RTK_REQUEST";
            break;
        case UPLOAD:
            RHJson["TYPE"] = "UPLOAD";

            // Take the message in as an object to create a new GNSS data object
            // m_gnss.populateGNSSMessage(RHJson["MSG"].as<JsonObject>()); premerge 8/16
            RHJson["MSG"]= "PLACEHOLDER";
            break;
        case ALERT:
            RHJson["TYPE"] = "ALERT";
            RHJson["MSG"] = "UNSPECIFIED_ALERT";
            break;

    }
}


/**
 * Transmit the packaged data to the rover
 */ 
bool Base::transmit(){

    // Set the serial Mux to talk to the Radio
    setMux(FeatherTxToRadioRx);
    delay(5);

    // Serialize the Json to string
    String processedRHMessage;
    serializeJson(m_JSONData, processedRHMessage);
    Serial.println(processedRHMessage);

    // Send the packet to the rover that just requested data
    return m_RadioManager.sendPacket(processedRHMessage, m_RadioManager.getMostRecentRover());
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
    Serial.println("\n*************************");

    // Print out the diagnostics to serial
    m_baseDiagnostics.print_serial();
}

/**
 * Print out the most recent rover packet 
 */ 
void Base::printMostRecentPacket(){
    Serial.println("\n**** Rover Packet ****");
    Serial.println("Rover Addr: " + String(m_RadioManager.getMostRecentRover()));
    serializeJsonPretty(m_RadioManager.getRoverPacket(), Serial);
    Serial.println("\n*************************");
}

/** 
 * Returns the messages'type 
 * 
*/
String Base::getMessageType(){
    
    return m_RadioManager.getRoverPacket()["TYPE"];
}

String Base::getMessage(){
    return m_RadioManager.getRoverPacket()["MSG"];
}

/**
 * Power on the radio to transmit data
 */ 
void Base::powerRadio(){
    Serial.println("[Base] Powering radio on.");
    m_max4820.assertRail(0);
}

/**
 * Power down the radio to conserve energy
 */ 
void Base::powerDownRadio(){
    Serial.println("[Base] Powering radio down.");
    m_max4820.assertRail(1);
}

/**
 * Power up the GNSS board to get fixes 
 */ 
void Base::powerGNSS(){
    Serial.println("[Base] Powering GNSS on.");
    m_max4820.assertRail(2);
}

/**
 * Power down the GNSS board
 */ 
void Base::powerDownGNSS(){
    Serial.println("[Base] Powering GNSS down.");
    m_max4820.assertRail(3);
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
