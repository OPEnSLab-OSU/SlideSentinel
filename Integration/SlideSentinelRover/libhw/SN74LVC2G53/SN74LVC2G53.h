#ifndef _SN74LVC2G53_H_
#define _SN74LVC2G53_H_

#include <Arduino.h>

class SN74LVC2G53 {
private:
  int m_sel;
  int m_inh;

public:
  SN74LVC2G53(int sel, int inh);
  void comY1();
  void comY2();
  void disable();
  void enable();
};

#endif // _SN74LVC2G53_H_
