#ifndef _GNSSCONTROLLER_H_
#define _GNSSCONTROLLER_H_
#define FIX_MODE_MASK 0b00000111
#define FIX_MODE_CLR 0b11111000

#include <Arduino.h>
#include "Controller.h"
#include "HardwareSerial.h"
#include "SwiftPiksi.h"
#include "wiring_private.h" // Pin peripheral

class GNSSController : public Controller {

private:
  HardwareSerial &m_serial;
  int m_baud;
  uint8_t m_rx;
  uint8_t m_tx;
  int m_logFreq;   // state
  const char *m_FORMAT; // csv format string
  void m_GNSSread();
  uint8_t m_getMode();
  void m_getModeStr(msg_pos_llh_t pos_llh, char rj[]);
  bool m_compare();
  void m_setBest();
  void m_isFixed(uint8_t &flag);
  void m_reset();
  void m_setLogFreq(int logFreq);

  msg_pos_llh_t m_pos_llh;
  msg_baseline_ned_t m_baseline_ned;
  msg_vel_ned_t m_vel_ned;
  msg_dops_t m_dops;
  msg_gps_time_t m_gps_time;
  uint8_t m_mode;

public:
  GNSSController(HardwareSerial &serial, uint32_t baud, uint8_t rx, uint8_t tx,
                 int logFreq);
  bool init();
  uint8_t poll(SSModel &model);
  char *getFormat();
  void status(SSModel &model);
  void update(SSModel &model);
  void flush();
};

#endif // _GNSSCONTROLLER_H_
