#include "RadioManager.h"

RadioManager::RadioManager() : m_RHSerialDriver(Serial1),
                               m_RHManager(m_RHSerialDriver, SERVER_ADDR){}

/**
 * Overwrite the recvBuffer with null bytes to clear it before new data is written to it
 */ 
void RadioManager::clearBuffer(){
    memset(recvBuffer, '\0', sizeof(recvBuffer)); 
}

/**
 * Clear out the Serial1 buffer from the radio
 */ 
void RadioManager::clearSerial(){
    while(Serial1.available()){
        Serial1.read();
    }
}

/**
 * Wait for a packet from the rover
 */ 
bool RadioManager::waitForPacket(){
    clearSerial(); // Clear the serial buffer to wash out any remaining junk
    clearBuffer(); // Clear out the buffer that we are gonna write the data to

    uint8_t messageSize = RH_SERIAL_MAX_MESSAGE_LEN;
    return m_RHManager.recvfromAckTimeout((uint8_t *)recvBuffer, &messageSize, INIT_TIMEOUT, &fromAddr);
}

/**
 * Read and parse the data out of the buffer into a useful format
 */ 
bool RadioManager::readHeader(){
    // Clear the current JSON buffer, and then write to it

    parsedDoc.clear();

    // Serialize the values received from the radio into the JSON document given the capacity of the buffer
    auto error = serializeJson(parsedDoc, (char* )recvBuffer, sizeof(recvBuffer));

    // Return the status of the serialization
    return !error;
}

/**
 * Return the address of the most recent rover
 */ 
int RadioManager::getMostRecentRover(){
    return fromAddr;
}

/**
 * Serialize the JSON object and then return the string representation
 */ 
StaticJsonDocument<RH_SERIAL_MAX_MESSAGE_LEN> RadioManager::getRoverPacket(){
    return parsedDoc;
}