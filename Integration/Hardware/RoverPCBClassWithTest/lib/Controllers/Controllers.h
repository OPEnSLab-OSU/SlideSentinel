#ifndef _CONTROLLERS_H_
#define _CONTROLLERS_H_

#include <Arduino.h>
#include "MAX3243.h"
#include "SN74LVC2G53.h"
#include <math.h>

// Naming conventions can be changed, I realized this a little later
// But these controllers are for rover, base can use some of the classes
// But they were created with only rover functionality in mind

// This class sets up TTL or RS232 Communication
class SerialCommunication : private MAX3243{
    public:
        SerialCommunication(uint8_t FORCEOFF_N);
        void setRS232();
        void setTTL();
        void printSerialCommunicationType();

    private:
        uint8_t m_s_comm_type;
}

class Radio : private Freewave, private SN74LVC2G53 {
    public:
        Radio(HardwareSerial* device, 
              uint8_t reset, 
              uint8_t cd, 
              uint8_t client,
              uint8_t server,
              bool is_z9c,
              uint8_t SPDT_SEL);

        void radioToFeather();
        void radioToGNSS();
        void sendMessage(uint8_t* mssg, uint8_t len);
        void receiveMessage(uint8_t* mssg);
        void sendFromFile(); // Maybe add this in, take max message length and split into double msg arr, within freewave make a splitmsgfunction

    private:
        void m_sendLargeMessage(uint8_t *mssg, uint8_t len);
        bool m_getRadioRxDirection();

}

#endif // _CONTROLLERS_H_