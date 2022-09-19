#include "Base.h"
#include "FeatherTrace.h"
#include <Arduino.h>

Base base;

void setup(){
    // Turn on the builtin led on the board
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(LED_BUILTIN, HIGH);

    Serial.begin(115200); // Start our monitor serial at 115200 baud
    while(!Serial); // Wait for data to propigate

    Serial.println("[Main] Initializing Setup...");

    // Start SPI
    SPI.begin();

    // Initialize the components used by the base
    base.powerRadio();
    base.powerGNSS();
    base.initBase();
}

// Enum to track the currrent state the Base is in, default to waiting for data
enum State {HANDSHAKE, UPDATE, PREPOLL, POLL, UPLOAD, WAIT, DEBUG};
static State state = WAIT;

void loop(){

    // Reinit the SD card if necessary
    //base.checkSD();

    // Checks if there are requests to print debug information
    base.debugInformation();

    // Main control loop managing which state the base currently exists in
    switch (state)
    {
        case DEBUG:
            base.checkSD();
            delay(1000);
            break;

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
               // base.setMux(Base::MuxFormat::RTCMOutToRadioRx);
                //at this point, base should be emitting corrections
            }
            break;
        
        case PREPOLL: MARK;
            Serial.println("Entering Prepoll, waiting for roughly 20 seconds before continuing...");
            base.setFeatherTimerLength(1000*20);
            base.setMux(Base::MuxFormat::RTCMOutToRadioRx);
            base.startFeatherTimer();
            state = POLL;
            break;

        case POLL: MARK;
            if(!base.isFeatherTimerDone()){
                state = POLL;
            }
            else{
                Serial.println("Entering Upload State!");
                base.setMux(Base::MuxFormat::FeatherTxToRadioRx);
                state = UPLOAD;
            }

            break;
        
        case UPLOAD: MARK;
            if(base.waitAndReceive()){
                Serial.println("[Main] Transitioning back to Wait");
                //Serial.println(base.getMessage());
                state = WAIT;
            }else{
                state = UPLOAD;
            }
            
            break;
        /* Transition to RTK fix mode */
        case UPDATE: MARK;
            break;
    }
}