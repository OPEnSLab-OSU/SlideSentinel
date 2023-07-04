#ifndef _MAX3243_H_
#define _MAX3243_H_

#include <Arduino.h>

//Translator chip for rs-232 communication

class MAX3243 {
private:
  uint8_t m_forceoff;

public:
  MAX3243(uint8_t forceoff_n);
  void enable();
  void disable();
};

#endif // _MAX3243_H_
