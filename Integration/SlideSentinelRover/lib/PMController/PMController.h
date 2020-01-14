#ifndef _PMCONTROLLER_H_
#define _PMCONTROLLER_H_

#include <Arduino.h>
#include "VoltageReg.h"
#include "MAX4280.h"

class PMController {
    private: 
        MAX4280* m_max4280; 
        PoluluVoltageReg* m_vcc2;
        bool m_GNSSRail2;
        bool m_RadioRail2;
    public: 
        PMController(MAX4280* max4280, PoluluVoltageReg* vcc2, bool GNSSrail2, bool radioRail2);
        void disableGNSS();
        void enableGNSS();
        void disableRadio();
        void enableRadio();
};

#endif // _PMCONTROLLER_H_