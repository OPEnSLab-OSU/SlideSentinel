#include "PMController.h"
#include "Console.h"
#define DEBUG true

PMController::PMController(MAX4280* max4280, PoluluVoltageReg* vcc2, bool GNSSrail2, bool radioRail2){
    m_GNSSRail2 = GNSSrail2;
    m_RadioRail2 = radioRail2;
    m_max4280 = max4280;
    m_vcc2 = vcc2;
    console.debug("PMController Initialized");
}

void PMController::enableGNSS() {
    if(m_GNSSRail2)
        m_vcc2->enable();
    m_max4280->assertRail(2);
    console.debug("GNSS on");
}

void PMController::disableGNSS(){
    if(m_GNSSRail2 && !m_RadioRail2)
        m_vcc2->disable();
    m_max4280->assertRail(3);
    console.debug("GNSS off");
}

void PMController::enableRadio() {
    if(m_RadioRail2)
        m_vcc2->enable();
    m_max4280->assertRail(0);
    console.debug("Radio on");
}

void PMController::disableRadio() {
    if(m_RadioRail2)
        m_vcc2->disable();
    m_max4280->assertRail(1);
    console.debug("GNSS off");
}
