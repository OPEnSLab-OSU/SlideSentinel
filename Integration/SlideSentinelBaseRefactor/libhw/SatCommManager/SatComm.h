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
#include "wiring_private.h"
#include "pcb_config.h"

// TODO: Move to main and use a serial setter
Uart SatCommSerial(&sercom1, IRIDIUM_RX, IRIDIUM_TX, SERCOM_RX_PAD_1, UART_TX_PAD_0);

/**
 * Serial interrupt handler
 */ 
void SERCOM1_Handler(){
    SatCommSerial.IrqHandler();
}

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

        /**
         * Ring ISR for when a message is received 
         */ 
        static void handleRing(){
            detachInterrupt(RING_PIN);
            didRing = true;
        };


    private:
        IridiumSBD* m_modem = nullptr;

        /* Get the current verison of the SatComm modem*/
        String getFirmwareVersion();

        // Sets if the ring interrupt was triggered
        static volatile bool didRing;
        
        // Map numbers to stringified error codes
        String initializationErrorCodes[14] = {"ISBD_SUCCESS", "ISBD_ALREADY_AWAKE", "ISBD_SERIAL_FAILURE", "ISBD_PROTOCOL_ERROR", "ISBD_CANCELLED", "ISBD_NO_MODEM_DETECTED", "ISBD_SBDIX_FATAL_ERROR", "ISBD_SENDRECEIVE_TIMEOUT", "ISBD_RX_OVERFLOW", "ISBD_REENTRANT", "ISBD_IS_ASLEEP", "ISBD_NO_SLEEP_PIN", "ISBD_NO_NETWORK", "ISBD_MSG_TOO_LONG"};
};