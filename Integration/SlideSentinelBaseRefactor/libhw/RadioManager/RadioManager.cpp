#include "RadioManager.h"

/* 
bool SSInterface::receivePacket(char *buffer) {
  clearSerial();
  if (!_receive())
    return false;

  _readHeader(m_buf);

  for (int i = 0; i < m_inFrag; i++) {
    if (!_receive())
      return false;
    memcpy(buffer + (i * m_blen), m_buf, m_blen);
  }
  return true;
}

bool SSInterface::_receive() {
  _clearBuffer();
  uint8_t len = RH_SERIAL_MAX_MESSAGE_LEN;
  if (m_manager.recvfromAckTimeout((uint8_t *)m_buf, &len, m_timeout, &m_from))
    return true;
  return false;
}

void SSInterface::_clearBuffer() {
  memset(m_buf, '\0', sizeof(char) * RH_SERIAL_MAX_MESSAGE_LEN);
}

// TODO also check if the key exists in the document even though it should
bool SSInterface::_readHeader(char *buf) {
  StaticJsonDocument<RH_SERIAL_MAX_MESSAGE_LEN> doc;
  auto error = deserializeJson(doc, buf);
  if (error)
    return false;
  m_inFrag = doc[FRAGMENT_NUM];
  m_type = doc[TYPE];
  if (m_base)
    m_clientId = doc[ROVER_ID];
  return true;
}
 */ 

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
    // Convert the uint8_t array to a char* and then serialize it to a JSON object
    char* data = (char* )recvBuffer;

    // Clear the current JSON buffer, and then write to it
    parsedDoc.clear();
    auto error = serializeJson(&parsedDoc, data);

    // Return the status of the serialization
    return !error;
}