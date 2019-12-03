#ifndef _FREEWAVERADIO_H_
#define _FREEWAVERADIO_H_

#include <Arduino.h>
#include "RHReliableDatagram.h"
#include "RH_Serial.h"

class Freewave {
    
    public: 
        Freewave(HardwareSerial* device, uint8_t reset, uint8_t cd, bool is_z9c);
        bool available(); 
        void read();
        void send();
        bool channel_busy();
        void reset();
    private: 
        uint8_t m_reset_pin;
        uint8_t m_cd_pin;
        bool m_is_z9c;
        RH_Serial m_device;
};

#endif // _FREEWAVERADIO_H_

