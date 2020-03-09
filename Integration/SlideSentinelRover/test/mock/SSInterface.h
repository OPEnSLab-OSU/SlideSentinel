#ifndef _SSINTERFACE_H_
#define _SSINTERFACE_H_

#define ROVER_ID "ID"
#define TYPE "TYPE"
#define FRAGMENT_NUM "NUM"

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
  bool m_base;
  uint8_t m_blen;

  char m_buf[RH_SERIAL_MAX_MESSAGE_LEN];
  char *_getBuf();
  void _clearBuffer();
  bool _send();
  bool _receive();
  void _header(const char *type);
  int _readHeader(char *buf);
  void _addType(JsonDocument &doc, const char *type);
  void _addId(JsonDocument &doc);
  void _addFragment(JsonDocument &doc);
  void _setOutFrag(char *buf);
  void _serializePkt(JsonDocument &doc);

public:
  SSInterface(HardwareSerial &serial, uint32_t baud, uint8_t clientId,
              uint8_t serverId, uint16_t timeout, uint8_t retries, bool isBase);

  bool sendPacket(const char *type, char *packet);
  bool receivePacket(char *buffer);
  void init();
  void setTimeout(uint16_t timeout);
  void setRetries(uint16_t retries);
  void setClient(uint8_t addr);
  void setServer(uint8_t addr);

  bool available();

  int getTimeout();
  int getRetries();
};

#endif // _SSINTERFACE_H_
