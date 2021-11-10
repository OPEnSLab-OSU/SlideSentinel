#ifndef _FREEWAVERADIO_H_
#define _FREEWAVERADIO_H_

#include <Arduino.h>

/* @brief Freewave Radio, either Z9-C/T, or GXM series, capable of sending and recieving data via the RadioHead library. */
class FreewaveRadio {

  /*Enum for radio type as each radio handles communication differently. */
  enum RadioType{
    Z9C = 0,    //RS-232
    Z9T = 1,    //TTL
    GXM = 2     //TTL
  };    

private:
  uint8_t m_rst;          //reset pin
  uint8_t m_cd;           //carrier detect output
  RadioType m_radioType;  //radio type, either Z9-C/T or GXM  

public:
  FreewaveRadio(uint8_t reset, uint8_t cd, RadioType radioType);
  bool channel_busy();
  void reset();
};

#endif // _FREEWAVERADIO_H_
