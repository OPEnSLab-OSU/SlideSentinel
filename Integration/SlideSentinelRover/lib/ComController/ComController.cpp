#include "ComController.h"
#include "Console.h"
#define DEBUG true

ComController::ComController(Freewave* freewave, MAX3243* max3243, SN74LVC2G53* sn74, HardwareSerial* port)
    : m_freewave(freewave)
    , m_max3243(max3243)
    , m_sn74(sn74)
    , radio(port, 0)
    {
        
    }

bool ComController::sendMessage(char* mssg, uint8_t toAddress){
    return radio.sendtoWait((uint8_t*) mssg, sizeof(mssg), toAddress);
}

bool ComController::recvMessage(char* mssg, uint8_t *fromAddress){
    uint8_t len, dest, id, flags;
    
    if(port.available()){
        return radio.recvfromAckTimeout((uint8_t*)mssg, &len, m_timeout, &fromAddress, &dest, &id, &flags);
    }
}

void ComController::setTimeOut(uint8_t time){
    m_timeout = time;
    console.debug("Timeout set to");
    //console.debug(time);
}

void ComController::enableMAX(){
    m_max3243.enable();
    console.debug("MAX3243 Enabled");
}

void ComController::disableMAX(){
    m_max3243.disable();
    console.degug("MAX3243 Disabled");
}

void ComController::enableSN(){
    m_sn74.enable();
    console.debug("SN74 Enabled");
}

void ComController::disableSN(){
    m_sn74.disable();
    console.debug("SN74 Disabled");
}