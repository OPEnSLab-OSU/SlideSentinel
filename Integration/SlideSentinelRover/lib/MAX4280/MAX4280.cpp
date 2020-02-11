#include "MAX4280.h"

MAX4280::MAX4280(uint8_t cs, SPIClass *spi) {
  m_cs = cs;
  m_clear = 0x00;
  m_spi = spi;
  pinMode(m_cs, OUTPUT);
}

void MAX4280::assertRail(uint8_t num) {
  digitalWrite(m_cs, LOW);
  m_spi->transfer(_BV(num));
  digitalWrite(m_cs, HIGH);
  delay(8);
  digitalWrite(m_cs, LOW);
  m_spi->transfer(m_clear);
  digitalWrite(m_cs, HIGH);
}

void MAX4280::clear() {
  digitalWrite(m_cs, LOW);
  m_spi->transfer(m_clear);
  digitalWrite(m_cs, HIGH);
}
