#pragma once

#include "MAX4820.h"
#include "SN74LVC2G53.h"
#include "base_pcb_config.h"
#include "base_network_config.h"
#include "RadioManager.h"
#include "SDManager.h"
#include "SatComm.h"

#include <RHReliableDatagram.h>
#include <RH_Serial.h>
#include <ArduinoJson.h>
/**
 * @brief The Base class is responsible for controlling all subparts of the base.
 *  
 */

class Base {

    public:

        /* 
            Different Ways to Package Data for transmit
            REQUEST - Notify base that rover is awaiting instructions
            UPLOAD - Upload polled data to base
            ALERT - High priority message such as accelerometer trip
        */
        enum DataType{
            REQUEST,
            UPLOAD,
            ALERT,
            INIT_RTK
        };

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

        /* Use in the setMux() function */
        enum MuxFormat {
            RadioToGNSS = 0,
            RadioToFeather = 1
        };

        /* Wait for data to be sent from a rover to the base */
        bool waitAndReceive(int milliseconds = INIT_TIMEOUT);

        /* returns the message type*/
        String getMessageType();

        /* Get the message sent by the rover */
        String getMessage();

        /* Initialize the components of the base */
        bool initBase();
        
        /* Transmit data to the rover*/
        bool transmit();

        /* Package data for transmit */
        void packageData(DataType packType);

        /* Package data for transmit using arbitrary data */
        void packageData(String type, String body);

        /* Print the current diagnostic information about the base station */
        void printDiagnostics();

        /* Print out the most recent rover packet received by the base */
        void printMostRecentPacket();

        /**
         * Check the current status of the SD card and reinitialize it if necessary '
         * @param reinit Specifies if the SD card should just be checked or if it fails it should attempt to reinit
         * */
        bool checkSD(bool reinit = true);

        /* Logs the most recently received packet to the SD card */
        bool logToSD();
        
        /* Waits for input over serial to output debug information about the base */
        void debugInformation();

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

        /* Upload data to satellite */
        bool uploadToSatComm();
        
        /**
         * Set the SatComm's Serial to the given value
         */ 
        void setSatCommSerial(Uart& serial) { m_satComm.setSerial(serial); Serial.println("[Base] SatComm Serial Bus Assigned!"); };

        /**
         * Expose the SatComm driver to main
         */ 
        SatComm& getSatComm() { return m_satComm; };

        /** FEATHER TIMER FUNCTIONS **/

        /*track start time*/
        void startFeatherTimer();

        /** Sets the timer duration
         * 
         * @param milliseconds The duration in milliseconds the timer will be set to
        */
        void setFeatherTimerLength(int milliseconds);

        /** Returns true if the feather timer duration has exceeded the set timer.
         *  Note: This timer expects the timer length to be set prior to calling this.
         *  TDL: doxygen return format?
        */
        bool isFeatherTimerDone();

        /**
         * QOL Feature waits a set timeout for the serial to be opened before just continuing as normal
         * @param timeout Maximum time to wait before continuing 
        */
        void waitForSerial(int timeout = 10000);

    private:
        BaseInfo m_baseInfo;                    // Base info that is sent back to the rover during handshake
        MAX4820 m_max4820;                      // Relay driver, used to power on relays controlling GNSS/Radio
        SN74LVC2G53 m_multiplexer;              // Multiplexer for redirecting data from the radio to GNSS and the Feather
        RadioManager m_RadioManager;                // RadioHead wrapper class for managing radio communication
        SDManager m_sdManager;                  // SdFat manager class that allows for easy reliable communication with SD cards
        SatComm m_satComm;                      // SatComm manager to allow for control of the sat comm

        DynamicJsonDocument m_JSONData;
        
        Diagnostics  m_baseDiagnostics;         // Diagnostics to track debug information about the base
        Diagnostics *m_roverDiagnostics;        // Pointer array of rover diagnostic information

        /* RTK Poll Variables*/
        unsigned long startTime;
        unsigned long featherTimerLength;

     
    };