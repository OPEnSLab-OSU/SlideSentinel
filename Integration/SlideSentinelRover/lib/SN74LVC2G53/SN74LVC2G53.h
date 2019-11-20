#ifndef _SN74LVC2G53_H_
#define _SN74LVC2G53_H_

#include <Arduino.h>
////////////////////////////////////////////////////
//          SN74LVCG53 Analog Multiplexer
// http://www.ti.com/lit/ds/symlink/sn74lvc2g53.pdf
////////////////////////////////////////////////////

class SN74LVC2G53 {
    public:
        SN74LVC2G53(HardwareSerial &monitor, uint8_t switchPin, uint8_t controlSwitch);
        void currentDirection();
        bool enableSwitch();
        bool disableSwitch();
        void radioToFeather();
        void radioToGNSS();

    private:
        HardwareSerial &m_monitor;
        uint8_t m_switchPin;      // A Pin 2 Enables or disables the switch
        uint8_t m_controlSwitch;  // INH Pin 5 Controls the switch
        int m_switchDir; 

};

// ON PCB INH should be pin 2 GND is on 2 and 4 it should be on 3 and 4
// ^---Followed the YZP package instead of the DCT package
// INH high disables switches
// INH low enables the switches
// Allows on off control 0-Vcc or 0-5V
// Y1 -> Feather Rx
// Y2 -> GNSS Tx
// COM -> Tx from radio
// A  -> Feather A0
// INH pin 2 needs to go to a analog pin on feather or just remain low which i think is the logic Kamron has set up with board

#endif // _SN74LVC2G53_H_