#ifndef _COMCONTROLLER_H_
#define _COMCONTROLLER_H_

#include <Arduino.h>
#include "ArduinoJson.h"
#include "Controller.h"
#include "FreewaveRadio.h"
#include "HardwareSerial.h"
#include "MAX3243.h"
#include "RHReliableDatagram.h"
#include "RH_Serial.h"
#include "SN74LVC2G53.h"

// TODO implement message fragmentation
// TODO Node ID's
// State: m_timeout, retries

class COMController : public Controller {
private:
  Freewave *m_radio;
  MAX3243 *m_max3243;
  SN74LVC2G53 *m_mux;
  HardwareSerial *m_serial;
  RH_Serial *m_driver;
  RHReliableDatagram *m_manager;
  uint32_t m_baud;
  uint8_t m_clientId;
  uint8_t m_serverId;
  const char *m_RTS;
  const char *m_ACK_ERR;
  const char *m_REP_ERR;
  const char *m_SER_ERR;
  char m_buf[RH_SERIAL_MAX_MESSAGE_LEN];

  bool m_send(char msg[]);
  bool m_receive(char buf[]);
  void m_createRTS(JsonDocument &doc);
  void m_clearBuffer();

public:
  COMController(State *state, Freewave *radio, MAX3243 *max3243, SN74LVC2G53 *mux,
                HardwareSerial *serial, uint32_t baud, uint8_t clientId,
                uint8_t serverId);
  bool request(JsonDocument &doc);
  bool upload(JsonDocument &doc);
  bool init();
  void status(uint8_t verbosity, JsonDocument &doc);
  void resetRadio();
  bool channelBusy();
};

#endif // _COMCONTROLLER_H_