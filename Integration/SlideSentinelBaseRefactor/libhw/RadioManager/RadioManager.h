#ifndef _RADIOMANGER_H_
#define _RADIOMANGER_H_
#include <RHReliableDatagram.h>
#include <RH_Serial.h>
#include <ArduinoJson.h>

#include "network_config_2.0.0.h"


class RadioManager{
    public:

        /* Parse the integer data received from the radio into JSON, and write it to the JSON object passed in*/
        bool readHeader();    
        /* Wait the given timeout length for a new packet to arrive from the rovers */ 
        bool waitForPacket();

        /* Send a packet to a specified client address */
        bool sendPacket();

        /**
         * Initialize the Serial and the Radio interface
         */  
        void initRadio();

    private:

        RadioManager();
    
        uint8_t recvBuffer[RH_MAX_MESSAGE_LEN]; // Buffer that the recieved message will be written into
        uint8_t fromAddr;                       // Address the message was received from
        
        StaticJsonDocument<RH_SERIAL_MAX_MESSAGE_LEN> parsedDoc;

        RH_Serial m_RHSerialDriver;             // Driver class for radio communication. Uses serial pins for feather.
        RHReliableDatagram m_RHManager;         // RadioHead communication manager class

        void clearBuffer();                     // Clear the receive buffer
        void clearSerial();                     // Read all data out of the serial buffer so we are working with a clean slate

};

#endif // _RADIOMANGER_H_