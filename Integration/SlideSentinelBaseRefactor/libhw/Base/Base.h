#ifndef _Base_H_
#define _Base_H_

#include "MAX4280.h"
#include "SN74LVC2G53.h"
#include "pcb_2.0.0.h"
#include "network_config_2.0.0.h"
#include <RadioManager.h>

#include <RHReliableDatagram.h>
#include <RH_Serial.h>
#include <ArduinoJson.h>
/**
 * @brief The Base class is responsible for controlling all subparts of the base.
 *  
 */

class Base {

    public:
        Base();

        
        /* Data Struct for rover info that gets sent to base. */
        struct BaseInfo{

            /* @var id ID used by RadioHead library, used by the Rovers to designate who is being contacted*/
            int id;

            /* @var radioBaud Baud rate used by Freewave radio, should always be 115200 unless otherwise specified.*/
            int radioBaud;

            /* @var init_retries Used by RadioHead to determine how many times to resend messages.*/
            int init_retries;

            /* @var timeoute Time waited until a message is considered failed.*/
            int timeout;
        };

        /* State of all relays/timers/multiplexer/etc */
        struct RoverDiagnostics {
            
        };
        RoverDiagnostics *rovers;

        /* Use in the setMux() function */
        enum MuxFormat {
            RTCMOutToRadioRx = 0,
            FeatherTxToRadioRx = 1
        };

        // Wait for data to be sent from a rover to the base
        void wait_for_request();

        // Print the current diagnostic information about the base station
        void print_diagnostics();

    private:
        BaseInfo m_baseInfo;                    // Base info that is sent back to the rover during handshake
        MAX4280 m_max4280;                      // Relay driver, used to power on relays controlling GNSS/Radio
        SN74LVC2G53 m_multiplexer;              // Multiplexer for redirecting data from the radio to GNSS and the Feather
        RadioManager m_RManager;                // RadioHead wrapper class for managing radio communication


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

#endif // _Base_H_
