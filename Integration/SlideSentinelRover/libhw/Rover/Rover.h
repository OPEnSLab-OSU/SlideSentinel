#ifndef _Rover_H_
#define _Rover_H_
#include "MAX4280.h"
#include "SN74LVC2G53.h"
#include "pcb_2.0.0.h"
#include "network_config_2.0.0.h"
#include "FreewaveRadio.h"

/**
 * @brief The Rover class is responsible for controlling all subparts of the rover.
 *  
 */

class Rover {

    /* Data Struct for rover info that gets sent to base. */
    struct RoverInfo {
        int id;
        int radioBaud;
        int init_retries;
        int timeout;
        int serverAddr;
    };

    /* State of all relays/timers/multiplexer/etc */
    struct RoverDiagnostics {
        
    };

    /* Use in the setMux() function */
    enum MuxFormat {
        RadioToFeather = 0,
        RadioToGNSS = 1
    };

public:
    Rover();
    void request();


private:
    /* Data structs*/
    RoverInfo m_rovInfo;
    RoverDiagnostics m_rovDiag;

    /* Physical hardware implementations*/ 
    MAX4280 m_max4280;
    FreewaveRadio m_radio;
    SN74LVC2G53 m_multiplexer;


    /* Tells the max4820 to enable the radio relay. */
    void powerRadio();

    /* Tells the max4820 to disable the radio relay. */
    void powerDownRadio();

    /* Tells the max4820 to enable the GNSS relay. */
    void powerGNSS();

    /* Tells the max4820 to disable the GNSS relay. */
    void powerDownGNSS();

    /* Sets the mutliplexer to Radio->Feather or Radio->GNSS depending on success of Base contact */
    void setMux(MuxFormat format);
    
};

#endif // _Rover_H_
