#ifndef _FREEWAVERADIO_H_
#define _FREEWAVERADIO_H_

#include <Arduino.h>
#include "RHReliableDatagram.h"
#include "RH_Serial.h"

class Freewave {
    public: 
        Freewave(HardwareSerial* device, 
                 uint8_t reset, 
                 uint8_t cd, 
                 uint8_t client, 
                 uint8_t server,
                 bool is_z9c);

        bool available(); 
        bool read(uint8_t* buf, uint8_t len, uint8_t *from);
        bool send(uint8_t* data, uint8_t len);
        bool channel_busy();
        void reset(); // Inheritance issue diamond of dooom, multiple classes will use relay and max4280

    protected:
        RH_Serial m_device;
        RHReliableDatagram m_manager;

    private:
        uint8_t m_reset_pin;
        uint8_t m_cd_pin;
        uint8_t m_CLIENT_ADDRESS;
        uint8_t m_SERVER_ADDRESS;
        bool m_is_z9c;
       
        
};

#endif // _FREEWAVERADIO_H_

