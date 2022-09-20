#include "RadioManager.h"

RadioManager::RadioManager() : m_RHSerialDriver(Serial1),
                               m_RHManager(m_RHSerialDriver, SERVER_ADDR){}

/**
 * Initialize the radio
 */ 
bool RadioManager::initRadio(){
    Serial1.begin(115200);
    Serial1.println("exit");
    delay(50);
    m_RHManager.setTimeout(INIT_TIMEOUT);
    m_RHManager.setRetries(INIT_RETRIES);
    return m_RHManager.init();
}

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
 * Sends a specified message to a given address
 * @param message Message to transmit to radio
 * @param addr Address to send to
 */ 
bool RadioManager::sendPacket(String message, int addr){
    char messageArray[RH_SERIAL_MAX_MESSAGE_LEN];
    message.toCharArray(messageArray, RH_SERIAL_MAX_MESSAGE_LEN);
    return m_RHManager.sendtoWait((uint8_t*)messageArray, message.length(), addr);
}

/**
 * Wait for a packet from the rover
 */ 
bool RadioManager::waitForPacket(int milliseconds){
    clearSerial(); // Clear the serial buffer to wash out any remaining junk
    clearBuffer(); // Clear out the buffer that we are gonna write the data to

    uint8_t messageSize = RH_SERIAL_MAX_MESSAGE_LEN;
    //Serial.println(fromAddr);
    return m_RHManager.recvfromAckTimeout((uint8_t *)recvBuffer, &messageSize, milliseconds, &fromAddr);
}

/**
 * Read and parse the data out of the buffer into a useful format
 */ 
bool RadioManager::readHeader(){
    // Clear the current JSON buffer, and then write to it
    parsedDoc.clear();

    // Serialize the values received from the radio into the JSON document given the capacity of the buffer
    auto error = deserializeJson(parsedDoc, (char* )recvBuffer, sizeof(recvBuffer));
   
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