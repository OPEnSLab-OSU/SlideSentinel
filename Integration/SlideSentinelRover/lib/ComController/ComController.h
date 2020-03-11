#ifndef _COMCONTROLLER_H_
#define _COMCONTROLLER_H_

#define REQ 1
#define UPL 2
#define RES 3

#include <Arduino.h>
#include "Controller.h"
#include "FreewaveRadio.h"
#include "HardwareSerial.h"
#include "MAX3243.h"
#include "RHReliableDatagram.h"
#include "RH_Serial.h"
#include "SN74LVC2G53.h"
#include "SSInterface.h"

class COMController : public Controller {
private:
  SSInterface m_interface;
  Freewave &m_radio;
  MAX3243 &m_max3243;
  SN74LVC2G53 &m_mux;

  uint16_t m_dropped_pkts; // diagnostic
  uint8_t m_threshold;     // state
  char m_buf[MAX_DATA_LEN];

  bool m_send(char msg[]);
  bool m_receive(char buf[]);
  void m_clearBuffer();

  void m_setTimeout(uint16_t timeout);
  void m_setRetries(uint16_t retries);
  void m_droppedPkt();

public:
  COMController(Freewave &radio, MAX3243 &max3243, SN74LVC2G53 &mux,
                HardwareSerial &serial, uint32_t baud, uint8_t clientId,
                uint8_t serverId, uint16_t timeout, uint8_t retries);
  bool request(SSModel &model);
  bool upload(SSModel &model);
  bool init();
  void status(SSModel &model);
  void update(SSModel &model);
  void resetRadio();
  bool channelBusy();
};

#endif // _COMCONTROLLER_H_