#ifndef _MAX4280_H_
#define _MAX4280_H_

#include <Arduino.h>
#include <SPI.h>

#define _BV(bit) (1 << (bit))

class MAX4280 {
private:
  uint8_t m_cs;
  uint8_t m_clear;
  SPIClass *m_spi;

public:
  MAX4280(uint8_t cs, SPIClass *spi);
  void assertRail(uint8_t num);
  void clear();
};

#endif // _MAX4280_H_
