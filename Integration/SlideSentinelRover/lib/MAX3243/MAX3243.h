#ifndef _MAX3243_H_
#define _MAX3243_H_

#include <Arduino.h>

class MAX3243 {
public:
  MAX3243(uint8_t forceoff_n);
  void enable();
  void disable();

private:
  uint8_t m_forceoff;
};

#endif // _MAX3243_H_
