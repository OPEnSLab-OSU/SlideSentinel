#ifndef _SN74LVC2G53_H_
#define _SN74LVC2G53_H_

#include <Arduino.h>

class SN74LVC2G53 {
private:
  uint8_t m_sel;
  uint8_t m_inh;

public:
  SN74LVC2G53(uint8_t sel, uint8_t inh);
  void comY1();
  void comY2();
  void disable();
  void enable();
};

#endif // _SN74LVC2G53_H_
