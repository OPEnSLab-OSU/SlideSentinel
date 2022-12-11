#include "MAX4820.h"

MAX4820::MAX4820(uint8_t cs, SPIClass *spi)
    : m_cs(cs), m_clear(0x00), m_spi(spi) {
  pinMode(m_cs, OUTPUT);
}

/**
 * Sends command to Max to activate relays.
 * @param uint8_t num: 0 and 1 refer to the Radio on/off, 2 and 3 refers to the GNSS for on/off
*/
void MAX4820::assertRail(uint8_t num) {
  digitalWrite(m_cs, LOW);
  m_spi->transfer(_BV(num));
  digitalWrite(m_cs, HIGH);
  delay(8);
  digitalWrite(m_cs, LOW);
  m_spi->transfer(m_clear);
  digitalWrite(m_cs, HIGH);
}

void MAX4820::clear() {
  digitalWrite(m_cs, LOW);
  m_spi->transfer(m_clear);
  digitalWrite(m_cs, HIGH);
}
