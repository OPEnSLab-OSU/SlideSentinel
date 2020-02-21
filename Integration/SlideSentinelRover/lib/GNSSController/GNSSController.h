#ifndef _GNSSCONTROLLER_H_
#define _GNSSCONTROLLER_H_

#include <Arduino.h>
#include "ArduinoJson.h"
#include "HardwareSerial.h"
#include "SwiftPiksi.h"
#include "Controller.h"
#include "wiring_private.h" // Pin peripheral

// State: m_logFreq
// TODO getBest() function 
class GNSSController : public Controller {

private:
  HardwareSerial *m_serial; 
  int m_baud;
  uint8_t m_rx;
  uint8_t m_tx;
  uint32_t m_logFreq;
  void m_GNSSread();
  uint8_t m_getMode();
  void m_getModeStr(msg_pos_llh_t pos_llh, char rj[]);
  bool m_compare();
  void m_setBest();
  void m_isFixed(uint8_t &flag);

  msg_pos_llh_t m_pos_llh;
  msg_baseline_ned_t m_baseline_ned;
  msg_vel_ned_t m_vel_ned;
  msg_dops_t m_dops;
  msg_gps_time_t m_gps_time;
  uint8_t m_mode;

public:
  GNSSController(HardwareSerial *serial, uint32_t baud, uint8_t rx, uint8_t tx);
  bool init();
  uint8_t poll(JsonDocument &doc);
  void update(JsonDocument &doc);
  void status(uint8_t verbosity, JsonDocument &doc);
};

#endif // _GNSSCONTROLLER_H_
