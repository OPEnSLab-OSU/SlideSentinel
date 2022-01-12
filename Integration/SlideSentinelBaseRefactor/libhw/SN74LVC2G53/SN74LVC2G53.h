#ifndef _SN74LVC2G53_H_
#define _SN74LVC2G53_H_

#include <Arduino.h>

/* The SN74LVC2G53 is a multiplexer capable of routing a signal to 2 separate endpoints. */
class SN74LVC2G53 {
private:
  int m_sel;
  int m_inh;

public:
  SN74LVC2G53(int sel, int inh);
  void comY1(); //RTCMout--->RadioRx
  void comY2(); //FeatherTx--->RadioRx
  void disable();
  void enable();
};

#endif // _SN74LVC2G53_H_
