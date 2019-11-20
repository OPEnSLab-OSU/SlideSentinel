#ifndef _SN74LVC2G53_H_
#define _SN74LVC2G53_H_

#include <Arduino.h>
////////////////////////////////////////////////////
//          SN74LVCG53 Analog Multiplexer
// http://www.ti.com/lit/ds/symlink/sn74lvc2g53.pdf
////////////////////////////////////////////////////

class SN74LVC2G53 {
    public:
        // If INH pin is unused and grounded set controlSwitch to -1
        SN74LVC2G53(uint8_t switchPin, uint8_t controlSwitch);
        void printDirection();
        bool enableSwitch();
        bool disableSwitch();
        void COMtoY1(); // Bi-Directional Flow between pins
        void COMtoY2(); // Bi-Directional Flow between pins

    private:
        uint8_t m_switchPin;      // A Pin 2 Switches the flow of mux between Y1 and Y2
        uint8_t m_controlSwitch;  // INH Pin 5 Controls the switch
        int m_switchDir; 
};

#endif // _SN74LVC2G53_H_