#ifndef _BATTERY_H_
#define _BATTERY_H_

#include <Arduino.h>

class Battery {

public:
  Battery(uint8_t a);
  float read();

private:
  uint8_t m_a;
};

#endif // _BATTERY_H_
