#ifndef _FREEWAVERADIO_H_
#define _FREEWAVERADIO_H_

#include <Arduino.h>

class Freewave {

/* Hardware level manager for freewave radio. */

public:
  /*Enum for radio type as each radio handles communication differently. */
  enum RadioType {
    Z9C = 0,    //RS-232
    Z9T = 1,    //TTL
    GXM = 2     //TTL
  };  

  Freewave();

  bool channel_busy();
  void reset();
  RadioType getType();

private:
  uint8_t m_rst;
  uint8_t m_cd;
  bool m_z9c;
  RadioType m_radioType;  //radio type, either Z9-C/T or GXM 

};

#endif // _FREEWAVERADIO_H_
