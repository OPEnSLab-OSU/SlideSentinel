#ifndef _FREEWAVERADIO_H_
#define _FREEWAVERADIO_H_

#include <Arduino.h>

class Freewave {
    
    public: 
        Freewave(uint8_t reset, uint8_t cd, bool is_z9c);
        bool channel_busy();
        void reset();
    private: 
        uint8_t m_reset_pin;
        uint8_t m_cd_pin;
        bool m_is_z9c;
};

#endif // _FREEWAVERADIO_H_

