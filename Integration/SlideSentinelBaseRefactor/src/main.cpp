#include "Base.h"
#include "FeatherTrace.h"
#include <Arduino.h>

Base base;

void setup(){
    // Turn on the builtin led on the board
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(LED_BUILTIN, HIGH);

    Serial.begin(115200); // Start our monitor serial at 115200 baud
    delay(3500); // Wait for data to propigate

    Serial.println("[Main] Initializing Setup...");

    // Start SPI
    SPI.begin();

    // Initialize the components used by the base
    base.initBase();
    base.setMux(Base::MuxFormat::FeatherTxToRadioRx);
    base.powerRadio();
    base.powerGNSS();
}

// Enum to track the currrent state the Base is in, default to waiting for data
enum State {HANDSHAKE, UPDATE, POLL, UPLOAD, WAIT};
static State state = WAIT;

void loop(){

    // Reinit the SD card if necessary
    base.checkSD();

    // Checks if there are requests to print debug information
    base.debugInformation();

    // Main control loop managing which state the base currently exists in
    switch (state)
    {
        /* Wait for data from the rovers*/
        case WAIT: MARK;
            // Default operating mode, in this mode we simply wait for data to be received from the rovers
            if(base.waitForRequest()){

                // Print out the packet received by the base
                base.printMostRecentPacket();
                base.setMux(Base::MuxFormat::RTCMOutToRadioRx);
                //at this point, base should be emitting corrections
            }
            break;
        
        /* Transition to RTK fix mode */
        case UPDATE: MARK;
            break;
    }
}