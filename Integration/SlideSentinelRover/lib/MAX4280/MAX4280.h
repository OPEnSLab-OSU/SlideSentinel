#ifndef _MAX4280_H_
#define _MAX4280_H_

#include <Arduino.h>
#include <SPI.h>

#define _BV(bit) (1 << (bit))

class MAX4280 {

public:
  MAX4280(uint8_t cs, SPIClass *spi);
  void assertRail(uint8_t num);
  void clear();

private:
  uint8_t m_clear;
  uint8_t m_cs;
  SPIClass *m_spi;
};

#endif // _MAX4280_H_
