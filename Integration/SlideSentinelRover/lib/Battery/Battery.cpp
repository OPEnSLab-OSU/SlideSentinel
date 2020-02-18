#include "Battery.h"

Battery::Battery(uint8_t a) {
  m_a = a;
  pinMode(m_a, INPUT);
}

float Battery::read() { return analogRead(m_a) * (3.3 / 1023.0); }
