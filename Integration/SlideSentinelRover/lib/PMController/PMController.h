#ifndef _PMCONTROLLER_H_
#define _PMCONTROLLER_H_

#include <Arduino.h>
#include "VoltageReg.h"
#include "MAX4280.h"
#include "Battery.h"

class PMController {
    private: 
        MAX4280* m_max4280; 
        PoluluVoltageReg* m_vcc2;
        Battery* m_bat;
        bool m_GNSSRail2;
        bool m_RadioRail2;

    public: 
        PMController(MAX4280* max4280, PoluluVoltageReg* vcc2, Battery* bat, bool GNSSrail2, bool radioRail2);
        void init();
        float readBat();
        void readBatStr(char buf[]);
        void disableGNSS();
        void enableGNSS();
        void disableRadio();
        void enableRadio();
        void sleep();
};

#endif // _PMCONTROLLER_H_