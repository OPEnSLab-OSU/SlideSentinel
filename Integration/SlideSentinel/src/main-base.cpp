#include "Base.h"
#include "FeatherTrace.h"
#include <Arduino.h>

Base base;

// TODO: Move to main and use a serial setter
Uart SatCommSerial(&sercom1, IRIDIUM_RX, IRIDIUM_TX, SERCOM_RX_PAD_1, UART_TX_PAD_0);

void setup(){
    // Turn on the builtin led on the board
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(LED_BUILTIN, HIGH);

    Serial.begin(115200); // Start our monitor serial at 115200 baud
    while(!Serial); // Wait for data to propagate
    SatCommSerial.begin(19200);

    while(true){
        if(SatCommSerial.available() > 0){
            Serial.print((char)SatCommSerial.read());
        }
        else{
            SatCommSerial.print(F("AT\r"));
        }
    }

    Serial.println("[Main] Initializing Setup...");

    // Start SPI
    SPI.begin();

    // Initialize the components used by the base
    base.setSatCommSerial(SatCommSerial);
    base.powerRadio();
    base.powerGNSS();
    base.initBase();
}

// Enum to track the currrent state the Base is in, default to waiting for data
enum State {HANDSHAKE, UPDATE, PREPOLL, POLL, UPLOAD, WAIT, RADIO_DEBUG, SATCOMM_DEBUG};
static State state = SATCOMM_DEBUG;

void loop(){

    // Checks if there are requests to print debug information
    base.debugInformation();

    // Main control loop managing which state the base currently exists in
    switch (state)
    {
        /* Wait for data from the rovers*/
        case WAIT: MARK;
            base.checkSD();

            // Default operating mode, in this mode we simply wait for data to be received from the rovers
            if(base.waitAndReceive()){
                
                // Print out the packet received by the base
                base.printMostRecentPacket();
                if(base.getMessageType() == "REQUEST"){
                    base.packageData("INIT_RTK_TYPE", "");
                    
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
        
        case PREPOLL: MARK;
            Serial.println("[Main] Entering Prepoll, waiting for roughly 20 seconds before continuing...");
            base.setFeatherTimerLength(1000*60*10);
            base.setMux(Base::MuxFormat::RadioToGNSS);
            base.startFeatherTimer();
            state = POLL;
            break;

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
        
        case UPLOAD: MARK;
            if(base.waitAndReceive() && base.getMessageType() == "UPLOAD"){
                base.printMostRecentPacket();
                base.logToSD();
                Serial.println("[Main] Transitioning back to Wait");
                state = WAIT;
            }else{
                state = UPLOAD;
            }
            
            break;
        /* Transition to RTK fix mode */
        case UPDATE: MARK;
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
                    Serial.println("Updated Time: " + base.getSatComm().getCurrentTimeString());
                }
                delay(1000);
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