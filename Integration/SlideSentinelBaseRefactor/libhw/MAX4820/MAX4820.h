#ifndef _MAX4820_H_
#define _MAX4820_H_

#include <Arduino.h>
#include <SPI.h>

#define _BV(bit) (1 << (bit))

/*@brief The MAX4820 is a relay driver.*/
class MAX4820 {
  
private:
  uint8_t m_cs;
  uint8_t m_clear;
  SPIClass *m_spi;

public:
  MAX4820(uint8_t cs, SPIClass *spi);
  /* Sends a command to the MAX4820 to turn on the relays.*/
  void assertRail(uint8_t num);
  void clear();
};

#endif // _MAX4280_H_

