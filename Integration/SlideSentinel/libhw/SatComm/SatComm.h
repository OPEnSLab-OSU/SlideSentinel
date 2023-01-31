///////////////////////////////////////////////////////////////////////////////
///
/// @file		SatComm.h
/// @brief		File containing Satellite Data communication prototypes
/// @author		Will Richards
/// @date		2022
/// @copyright	GNU General Public License v3.0
///
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <IridiumSBD.h>
#include <ArduinoJson.h>
#include <time.h>
#include "wiring_private.h"

#if defined(BASE_BUILD)
    #include "base_pcb_config.h"
#elif defined(ROVER_BUILD)
    #include "rover_pcb_config.h"
#endif


class SatComm{
    
    public:

        /* Construct a new SatComm object*/
        SatComm();

        /* Destructor to clean up dynamically created objects*/
        ~SatComm();

        /**
         * Initialize the Sat Comm device 
         * @return The status of the initialization
         */
        bool initSatComm();

        /**
         *  Checks if we have a satellite signal, NON-BLOCKING!
         *  @return If we have a satellite connection
         */
        bool waitForSignal();

        /* Update the system time */
        bool updateSystemTime();

        /* Transmit the data from the Rockblock to the Satellite */
        bool transmit(JsonObject json);

        /* Get the time recorded at the last update*/
        tm getCurrentTime() { return currentTime; };

        /* Print the current time in UTC*/
        String getCurrentTimeString();

        /**
         * Set the serial to be used by the satcomm
         */ 
        void setSerial(Uart& serial) { SatCommSerial = &serial; };

    private:
        Uart* SatCommSerial = nullptr;
        IridiumSBD* m_modem = nullptr;

        // Time received from the modem
        tm currentTime = {};

        /* Get the current verison of the SatComm modem*/
        String getFirmwareVersion();

        /* Take the given JSON packet and minify it to only use 1 credit of SatComm transmission*/
        String minifyJson(JsonObject json);

        // Map numbers to stringified error codes
        String initializationErrorCodes[14] = {"ISBD_SUCCESS", "ISBD_ALREADY_AWAKE", "ISBD_SERIAL_FAILURE", "ISBD_PROTOCOL_ERROR", "ISBD_CANCELLED", "ISBD_NO_MODEM_DETECTED", "ISBD_SBDIX_FATAL_ERROR", "ISBD_SENDRECEIVE_TIMEOUT", "ISBD_RX_OVERFLOW", "ISBD_REENTRANT", "ISBD_IS_ASLEEP", "ISBD_NO_SLEEP_PIN", "ISBD_NO_NETWORK", "ISBD_MSG_TOO_LONG"};
};