#include "FreewaveRadio.h"

Freewave::Freewave(uint8_t reset, bool is_z9c)
  : m_rst(reset), m_z9c(is_z9c) 
{
  pinMode(m_rst, OUTPUT);
  digitalWrite(m_rst, HIGH);
}

bool Freewave::getZ9C() { return m_z9c; }

void Freewave::reset() {
  digitalWrite(m_rst, LOW);
  delay(2000);
  digitalWrite(m_rst, HIGH);
}