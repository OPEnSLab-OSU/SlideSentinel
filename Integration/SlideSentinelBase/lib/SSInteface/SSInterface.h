#ifndef _SSINTERFACE_H_
#define _SSINTERFACE_H_

// NOTE The header packet must be lower than RH_SERIAL_MAX_MESSAGE_LEN
// This library sends a header with the message type, the rover ID, and the
// number of incoming message fragments

#define ROVER_ID "ID"
#define TYPE "TYPE"
#define FRAGMENT_NUM "FRAG"

#include <Arduino.h>
#include "ArduinoJson.h"
#include "HardwareSerial.h"
#include "RHReliableDatagram.h"
#include "RH_Serial.h"

class SSInterface {
private:
  HardwareSerial &m_serial;
  uint32_t m_baud;
  uint8_t m_clientId;
  uint8_t m_serverId;
  RH_Serial m_driver;
  RHReliableDatagram m_manager;
  uint16_t m_timeout;
  uint8_t m_retries;

  uint8_t m_outFrag;
  uint8_t m_inFrag;
  uint8_t m_type;
  bool m_base;
  uint8_t m_blen;

  char m_buf[RH_SERIAL_MAX_MESSAGE_LEN];
  void _clearBuffer();
  bool _send();
  bool _receive();
  void _header(const int type);
  bool _readHeader(char *buf);
  void _addType(JsonDocument &doc, const int type);
  void _addId(JsonDocument &doc);
  void _addFragment(JsonDocument &doc);
  void _setOutFrag(char *buf);
  void _serializePkt(JsonDocument &doc);

public:
  SSInterface(HardwareSerial &serial, uint32_t baud, uint8_t clientId,
              uint8_t serverId, uint16_t timeout, uint8_t retries, bool isBase);
  void init();
  void setTimeout(uint16_t timeout);
  void setRetries(uint16_t retries);
  bool sendPacket(const int type, char *packet);
  bool receivePacket(char *buffer);
  bool available();
  int getTimeout();
  int getRetries();
  int getType();
  int getId();
};

#endif // _SSINTERFACE_H_
