#pragma once
#include <RHReliableDatagram.h>
#include <RH_Serial.h>
#include <ArduinoJson.h>

#if defined(BASE_BUILD)
    #include "../Base/base_network_config.h"
#elif defined(ROVER_BUILD)
    #include "../Rover/rover_network_config.h"
#endif


class RadioManager{
    public:

        /* Default constructor initializes the Radio on the device */ 
        RadioManager();

        /* Parse the integer data received from the radio into JSON, and write it to the JSON object passed in*/
        bool readHeader();    

        /* Wait the given timeout length for a new packet to arrive from the rovers */ 
        bool waitForPacket(int milliseconds = INIT_TIMEOUT);

        /* Send a packet to a specified client address */
        bool sendPacket(String message, int addr);

        /* Initialize the Serial and the Radio interface */  
        bool initRadio();

        /* Return the address of the most recent rover packet */
        int getMostRecentRover();

        /* Serialize the JSON packet received from the rover and return it*/
        StaticJsonDocument<RH_SERIAL_MAX_MESSAGE_LEN> getRoverPacket();
        uint8_t recvBuffer[RH_SERIAL_MAX_MESSAGE_LEN];              // Buffer that the recieved message will be written into

    private:

        uint8_t fromAddr;                                           // Address the message was received from
        
        StaticJsonDocument<RH_SERIAL_MAX_MESSAGE_LEN> parsedDoc;    // JSON document the size of the max message length that will store serializedJson data

        RH_Serial m_RHSerialDriver;                                 // Driver class for radio communication. Uses serial pins for feather.
        RHReliableDatagram m_RHManager;                             // RadioHead communication manager class

        void clearBuffer();                                         // Clear the receive buffer
        void clearSerial();                                         // Read all data out of the serial buffer so we are working with a clean slate

};