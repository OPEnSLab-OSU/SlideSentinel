#ifndef _COMCONTROLLER_H_
#define _COMCONTROLLER_H_

#include <Arduino.h>
#include "MAX3243.h"
#include "SN74LVC2G53.h"
#include "FreewaveRadio.h"
#include "RHReliableDatagram.h"

class ComController{
private:
    // Hardware 
    Freewave* m_freewave();
    MAX3243* m_max3243(); 
    SN74LVC2G53* m_sn74();
    RHReliableDatagram radio(); 

    // Variables
    uint8_t m_timeout = 500; // In milli-seconds

public:
    ComController(Freewave* freewave, MAX3243* max3243, SN74LVC2G53* sn74,
                  HardwareSerial* port);

    bool sendMessage(char* mssg, uint8_t toAddress);
    bool recvMessage(char* mssg, uint8_t *fromAddress);
    void setTimeOut(uint8_t time); // needs to get set or default timeout is 5 seconds
    void enableMAX();
    void disableMAX();
    void enableSN();
    void disableSN();
}

#endif // _COMCONTROLLER_H_