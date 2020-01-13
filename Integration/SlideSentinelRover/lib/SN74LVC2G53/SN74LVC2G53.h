#ifndef _SN74LVC2G53_H_
#define _SN74LVC2G53_H_

#include <Arduino.h>

class SN74LVC2G53 {
    public: 
        SN74LVC2G53(uint8_t sel, uint8_t inh);
        void comY1();
        void comY2();
        void disable(); 
        void enable();
    private: 
        uint8_t m_sel_pin; 
        uint8_t m_inh_pin; 
};

#endif // _SN74LVC2G53_H_

