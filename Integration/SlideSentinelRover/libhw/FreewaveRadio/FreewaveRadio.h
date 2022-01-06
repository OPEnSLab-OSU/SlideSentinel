#ifndef _FREEWAVERADIO_H_
#define _FREEWAVERADIO_H_

#include <Arduino.h>

class Freewave {

/* Hardware level manager for freewave radio. */

private:
  uint8_t m_rst;
  uint8_t m_cd;
  bool m_z9c;

public:
  Freewave(uint8_t reset, uint8_t cd, bool is_z9c);
  bool getZ9C();
  bool channel_busy();
  void reset();
};

#endif // _FREEWAVERADIO_H_
