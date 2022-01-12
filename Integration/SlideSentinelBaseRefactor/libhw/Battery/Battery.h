#ifndef _BATTERY_H_
#define _BATTERY_H_

#include <Arduino.h>

class Battery {
private:
  uint8_t m_a;

public:
  Battery(uint8_t a);
  float read();
};

#endif // _BATTERY_H_
