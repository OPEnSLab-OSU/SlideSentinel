#ifndef _VOLTAGEREG_H_
#define _VOLTAGEREG_H_

#include <Arduino.h>

class PoluluVoltageReg {
    
    public: 
        PoluluVoltageReg(uint8_t en);
        void enable();
        void disable();
    private: 
        uint8_t m_en_pin;
};

#endif // _VOLTAGEREG_H_

