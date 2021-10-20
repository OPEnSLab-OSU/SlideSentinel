#ifndef _PMCONTROLLER_H_
#define _PMCONTROLLER_H_
#define MAX_VOLT_LEN 5

#include <Arduino.h>
#include "Battery.h"
#include "Controller.h"
#include "MAX4280.h"
#include "PoluluVRM.h"

class PMController : public Controller {
private:
  MAX4280 &m_max4280;
  PoluluVRM &m_vcc2;
  Battery &m_bat;
  char m_volt[MAX_VOLT_LEN];
  bool m_GNSSRail2;
  bool m_RadioRail2;

public:
  PMController(MAX4280 &max4280, PoluluVRM &vcc2, Battery &bat,
               bool GNSSrail2, bool radioRail2);
  float readBat();
  char* readBatStr();
  void disableGNSS();
  void enableGNSS();
  void disableRadio();
  void enableRadio();
  void sleep();
  bool init();
  void status(SSModel &model);
  void update(SSModel &model);
};

#endif // _PMCONTROLLER_H_