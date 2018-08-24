// ================================================================
// ===              INCLUDE CONFIGURATION FILE                  ===
// ===    INCLUDE DECLARATIONS, STRUCTS, AND FUNCTIONS FROM     ===
// ===            OTHER FILES AS SET IN CONFIG.H                ===
// ================================================================

// Config has to be first has it hold all user specified options
#include "config.h"

// Preamble includes any relevant subroutine files based 
// on options specified in the above config
#include "loom_preamble.h"

#include <Arduino.h>   // required before wiring_private.h
#include "wiring_private.h" // pinPeripheral() function


#define BAUD 57600    // reading and writing occurs at 
#define CELLULAR 0

//===== LoRa Initializations =====
#define CAPACITY 500
#define SERVER_ADDRESS 1
#define VBATPIN A7
#define MAX_LEN 250 

//define string for reading RTK data
uint8_t RTKString[RH_RF95_MAX_MESSAGE_LEN*5];
int len;
int chars_to_send;
int first_index;
int last_payload;
uint8_t rec_from, rec_to, rec_id, rec_flags;
bool is_read = false;
unsigned long bytes_sent, timer_10, bndl_time;
// ================================================================ 
// ===                           SETUP                          ===
// ================================================================ 

void setup() 
{
	// LOOM_begin calls any relevant (based on config) LOOM device setup functions
	Loom_begin();	
  Serial.begin(115200);         //Opens the main serial port to communicate with the computer



  /*check LoRa device and set frequency*/

  len = 0;
  bndl_time = 0;
	// Any custom setup code
}

// ================================================================ 
// ===                        MAIN LOOP                         ===
// ================================================================ 
void loop() 
{    
    if(millis() - bndl_time > 10000){
        //strncpy(tab_id, "SS_ACCEL", 19);
        sprintf(tab_id, "SS_ACCEL");
        char nmeaString[] = "$PSTI,030,192038.000,A,4433.9948388,N,12316.8455605,W,54.605,0.01,0.01,0.02,230818,F,1.0,1.0*37";
        OSCBundle bndl;
        OSCMessage msg;
        msg.setAddress("/nmea");
        msg.add("nmea").add(nmeaString);
    
        bndl.add(msg);
        Serial.println("Added message to bundle");
        log_bundle(&bndl, PUSHINGBOX);
        bndl_time = millis();
        Serial.println("OSCMessage: ");
        print_bundle(&bndl);
        bndl_time = millis();
    }
}









