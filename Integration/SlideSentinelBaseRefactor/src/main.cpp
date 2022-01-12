#include "Base.h"
#include "FeatherTrace.h"
#include <Arduino.h>

Base base;

void setup(){
    Serial.begin(115200);
    delay(3500); // Wait for data to propigate
    Serial.println("[Main] Initializing Setup...");
    SPI.begin();
}

// Enum to track the currrent state the Base is in, default to waiting for data
enum State {HANDSHAKE, UPDATE, POLL, UPLOAD, WAIT};
static State state = WAIT;

void loop(){

    // Check for user input on serial to request information about the base
    if(Serial.available()){
        char cmd = Serial.read();
        if(cmd == '1'){
            Serial.println("------- BASE DIAG --------");
            base.print_diagnostics();
        }
    }


    // Main control loop managing which state the base currently exists in
    switch (state)
    {
        case WAIT: MARK;
            // Default operating mode, in this mode we simply wait for data to be received from the rovers

            base.wait_for_request();
            Serial.println("[Main] Waiting for data from rovers...");
            break;
    }
}