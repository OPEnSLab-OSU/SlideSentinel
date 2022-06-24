#ifndef _Rover_H_
#define _Rover_H_
#include "MAX4280.h"
#include "SN74LVC2G53.h"
#include "pcb_2.0.0.h"
#include "network_config_2.0.0.h"
#include <RHReliableDatagram.h>
#include <RH_Serial.h>
#include <ArduinoJson.h>
#include <GNSSController.h>
#include "SPI.h"
#include <RTClib.h>
/**
 * @brief The Rover class is responsible for controlling all subparts of the rover.
 *  
 */

class Rover {

public:
    Rover(int radioType);
    Rover();
    bool listen();

    /* Data Struct for rover info that gets sent to base. */
    struct RoverInfo {
        /* @var id ID used by RadioHead library, sent to Base to determine which rover is contacting it.*/
        int id;
        /* @var radioBaud Baud rate used by Freewave radio, should always be 115200 unless otherwise specified.*/
        int radioBaud;
        /* @var init_retries Used by RadioHead to determine how many times to resend messages.*/
        int init_retries;
        /* @var timeoute Time waited until a message is considered failed.*/
        int timeout;
        /* @var serverAddr Server address that the rover sends a RadioHead message to. This should be 0 unless multiple bases in the same area.*/
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
    
    /* Powers radio via relay, called in wake cycle in main */
    void wake();

    /* Called after radio has been enabled in the HANDSHAKE section. Attempts to make contact with base. If successful:
    transition to polling mode, if not: power down and set short wake timer if base is busy. */
    bool request();

    /* Send a test message */
    void sendManualMsg(char* msg);

    /* Tells the max4820 to disable the radio relay. */
    void powerDownRadio();

    /* Tells the max4820 to enable the radio relay. */
    void powerRadio();

    /* Tells the max4820 to enable the GNSS relay. */
    void powerGNSS();

    /* Tells the max4820 to disable the GNSS relay. */
    void powerDownGNSS();

    /* Sets the mutliplexer to Radio->Feather or Radio->GNSS depending on success of Base contact */
    void setMux(MuxFormat format);

    /* Initialize RadioHead objects */
    void initRadio();

    /* Debug function for RTC to print out time*/
    void debugRTCPrint();

    /* prints time from real time clock*/
    void printRTCTime();


    
private:
    RoverInfo m_rovInfo;            //Rover info that is sent over during handshake, like rover ID
    MAX4280 m_max4280;              //Relay driver, used to power on relays controlling GNSS/Radio
    SN74LVC2G53 m_multiplex;        //Multiplexer used for redirecting information from radio rx to GNSS and radio rx to Feather
    HardwareSerial &m_serial;       //Reference to a serial interface object
    RH_Serial m_RHSerialDriver;             //Driver class for radio communication. Uses serial pins for feather.
    RHReliableDatagram m_RHManager;         //RadioHead communication manager class
    RTC_DS3231 m_RTC;               //Real time clock object


    /*  A message consists of an: ID, TYPE, MSG
        The definitions are as such:
            ID: ID of the rover sending the message
            TYPE: Message type, such as REQUEST, UPLOAD
            MSG: data upload, eg: "152.21312,12.12312, etc"
     */
    DynamicJsonDocument m_RHMessage;


    
};

#endif  _Rover_H_
