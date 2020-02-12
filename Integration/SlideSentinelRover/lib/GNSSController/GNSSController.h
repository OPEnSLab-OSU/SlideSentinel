#ifndef _GNSSCONTROLLER_H_
#define _GNSSCONTROLLER_H_

#include <Arduino.h>
#include "ArduinoJson.h"
#include "Controller.h"
#include "HardwareSerial.h"
#include "SwiftPiksi.h"
#include "wiring_private.h" // Pin peripheral

class GNSSController : public Controller {

private:
  HardwareSerial *m_serial; // Postional data
  int m_baud;
  uint8_t m_rx;
  uint8_t m_tx;
  uint32_t m_logFreq;
  bool init();
  void GNSSread();
  void getModeStr(msg_pos_llh_t pos_llh, char rj[]);
  uint8_t getMode();

public:
  GNSSController(HardwareSerial *serial, uint32_t baud, uint8_t rx, uint8_t tx);
  bool poll(Serial_ &terminal, JsonDocument &doc);
  void update(JsonDocument &doc);
  void status(uint8_t verbosity, JsonDocument &doc);
};

#endif // _GNSSCONTROLLER_H_
