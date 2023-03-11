#include "Base.h"
#include "FeatherTrace.h"
#include <Arduino.h>

/* If set to true shorten times to match that of a GNSS sim fix */
#define SIMULATION_MODE false

/* When set to false we will not try to communicate via the SatComm*/
#define SATCOMM_ENABLED false

Base base;

// SatComm Software Serial
Uart SatCommSerial(&sercom1, IRIDIUM_RX, IRIDIUM_TX, SERCOM_RX_PAD_1, UART_TX_PAD_0);

void setup(){
    // Turn on the builtin led on the board
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(LED_BUILTIN, HIGH);

    Serial.begin(115200); // Start our monitor serial at 115200 baud
    base.waitForSerial(); // Wait 10 seconds for the serial monitor to open
    
    Serial.println("[Main] Initializing Setup...");

    // Start SPI
    SPI.begin();

    // Initialize the components used by the base
    base.setSatCommSerial(SatCommSerial);
    base.powerRadio();
    base.powerGNSS();
    base.initBase();
}

/**
 * Enum to track the currrent state the Base is in, default to waiting for data
 * WAIT - In this state we wait for communication with the rover
 * PREPOLL - Setup 10 minute wait for the rover to get an RTK Fix
 * POLL - Wait 10 minutes for the timer to complete and the RTK fix to be done
 * UPLOAD - Receive RTK data and log it to SD / Print to serial
 * */ 
enum State {WAIT, PREPOLL, POLL, UPLOAD, RADIO_DEBUG, SATCOMM_DEBUG, SATCOMM_INIT};
static State state = WAIT;

void loop(){

    // Checks if there are requests to print debug information
    base.debugInformation();

    // Main control loop managing which state the base currently exists in
    switch (state)
    {
        /* Wait for data from the rovers*/
        case WAIT: MARK;

            // Check the status of the SD card and reinitialize if necessary
            base.checkSD();

            // Default operating mode, in this mode we simply wait for data to be received from the rovers
            if(base.waitAndReceive()){
                
                // Print out the packet received by the base
                base.printMostRecentPacket();
                if(base.getMessageType() == "REQUEST"){
                    base.packageData(Base::DataType::INIT_RTK);
                    
                    if(base.transmit()){
                        Serial.println("[Main] Transmit successful! Polling RTK...");
                        state = PREPOLL;
                    }
                    else{
                        Serial.println("[Main] Failed to transmit RTK initialization command!");
                    }

                }
            }
            break;
        
        // Wait for 10 minutes to allow time for the rover to get an RTK fix
        case PREPOLL: MARK;
            Serial.println("[Main] Entering Prepoll, waiting for roughly 10 minutes before continuing...");

            // Start listening for data 1 second before the rover should start transmitting 9.75 mins=585000

            // We may not need to poll for as long 
            #if SIMULATION_MODE == true 
                base.setFeatherTimerLength(9000);
            #else
                base.setFeatherTimerLength(1000*60*4);
            #endif
            base.setMux(Base::MuxFormat::RadioToGNSS);
            base.startFeatherTimer();
            state = POLL;
            break;

        // Once we have waited we transition to the upload state
        case POLL: MARK;
            if(!base.isFeatherTimerDone()){
                state = POLL;
            }
            else{
                Serial.println("Entering Upload State!");
                base.setMux(Base::MuxFormat::RadioToFeather);
                delay(50);
                state = UPLOAD;
            }
            
            break;
        
        // In the upload state we wait for data and check if the data is an upload if so we print the packet and log the data to SDss
        case UPLOAD: MARK;
            if(base.waitAndReceive() && base.getMessageType() == "UPLOAD"){
                base.printMostRecentPacket();
                base.logToSD();

                // Upload to satcomm if enabled
                #if SATCOMM_ENABLED == true
                    base.uploadToSatComm();
                #endif

                Serial.println("[Main] Transitioning back to Wait");
                state = WAIT;
            }else{
                state = UPLOAD;
            }
            
            break;

         // Send and receive dummy data
        case RADIO_DEBUG:
            while(true){
                if(Serial1.available())
                    Serial.print((char)Serial1.read());
                else
                    Serial1.println("Pong");
            }
            break;

         // Send and receive dummy data
        case SATCOMM_DEBUG:
            while(true){
                // Wait for signal and once acquired update time
                if(base.getSatComm().waitForSignal()){
                    base.getSatComm().updateSystemTime();
                    Serial.print("Updated Time: ");
                    Serial.println(base.getSatComm().getCurrentTimeString());
                }
                delay(1000);
            }
           
            break;
        
        // If data is available to read it will read it and if not it will attempt to send an INITIALIZE AT command
        case SATCOMM_INIT:
            while(true){
                if(SatCommSerial.available() > 0){
                    Serial.print((char)SatCommSerial.read());
                }
                else{
                    SatCommSerial.print(F("AT\r"));
                }
            }
            break;
    }
}

/**
 * Serial interrupt handler
 */ 
void SERCOM1_Handler(){
    SatCommSerial.IrqHandler();
}