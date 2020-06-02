#ifndef _COMCONTROLLER_H_
#define _COMCONTROLLER_H_

#define REQ 1
#define UPL 2
#define RES 3

#include "BaseModel.h"
#include "Controller.h"
#include "FreewaveRadio.h"
#include "HardwareSerial.h"
#include "RHReliableDatagram.h"
#include "RH_Serial.h"
#include "SN74LVC2G53.h"
#include "SSInterface.h"
#include "Timer.h"
#include <Arduino.h>

class COMController : public Controller {
private:
  SSInterface m_interface;
  Freewave &m_radio;
  SN74LVC2G53 &m_mux;
  Timer m_timer;

  // is the base servicing a rover
  bool m_isServing;
  // the rover currently being serviced
  int m_rover;

  // diagnostics
  int m_num_uploads;
  int m_num_requests;

  char m_buf[MAX_DATA_LEN];

  void m_clear();
  void m_setTimeout(uint16_t timeout);
  void m_setRetries(uint16_t retries);
  void m_reset();
  bool m_available();

  bool m_request(int rover_id, BaseModel &model);
  bool m_upload(int rover_id, BaseModel &model);


  bool m_transaction();

public:
  COMController(Freewave &radio, SN74LVC2G53 &mux, HardwareSerial &serial,
                uint32_t baud, uint8_t clientId, uint8_t serverId,
                uint16_t timeout, uint8_t retries);
  bool init();
  void resetRadio();
  bool listen(BaseModel &model);
  bool timeout();
  void status(BaseModel &model);
  void update(BaseModel &model);
};

#endif // _COMCONTROLLER_H_