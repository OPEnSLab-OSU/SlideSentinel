#ifndef _FREEWAVERADIO_H_
#define _FREEWAVERADIO_H_

#include <Arduino.h>

class Freewave {

private:
  const uint8_t m_rst;
  const bool m_z9c;

public:
  Freewave(uint8_t reset, bool is_z9c);
  bool getZ9C();
  void reset();
};

#endif // _FREEWAVERADIO_H_
