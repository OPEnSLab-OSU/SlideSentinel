#ifndef _VOLTAGEREG_H_
#define _VOLTAGEREG_H_

#include <Arduino.h>

class PoluluVoltageReg {
private:
  uint8_t m_en;

public:
  PoluluVoltageReg(uint8_t en);
  void enable();
  void disable();
};

#endif // _VOLTAGEREG_H_
