#ifndef _PMCONTROLLER_H_
#define _PMCONTROLLER_H_

#include <Arduino.h>
#include "Battery.h"
#include "Controller.h"
#include "MAX4280.h"
#include "VoltageReg.h"

class PMController : public Controller {
private:
  MAX4280 *m_max4280;
  PoluluVoltageReg *m_vcc2;
  Battery *m_bat;
  bool m_GNSSRail2;
  bool m_RadioRail2;

public:
  PMController(MAX4280 *max4280, PoluluVoltageReg *vcc2, Battery *bat,
               bool GNSSrail2, bool radioRail2);
  void init();
  float readBat();
  void readBatStr(char buf[]);
  void disableGNSS();
  void enableGNSS();
  void disableRadio();
  void enableRadio();
  void sleep();
  void update(JsonDocument &doc);
  void status(uint8_t verbosity, JsonDocument &doc);
};

#endif // _PMCONTROLLER_H_