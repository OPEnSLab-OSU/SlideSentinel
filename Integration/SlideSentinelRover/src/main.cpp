#include "Battery.h"
#include "COMController.h"
#include "ConManager.h"
#include "Controller.h"
#include "FSController.h"
#include "FreewaveRadio.h"
#include "GNSSController.h"
#include "IMUController.h"
#include "MAX3243.h"
#include "MAX4280.h"
#include "PMController.h"
#include "RTClibExtended.h"
#include "SN74LVC2G53.h"
#include "SSModel.h"
#include "PoluluVRM.h"
#include "config_2.0.0.h"
#include "network_config_2.0.0.h"
// #include "pcb_2.0.0.h"
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "FeatherTrace.h"
#include "Rover.h"


Rover rover;

void setup() {
  Serial.begin(115200);
  delay(3500);           //delay to allow screening in
  Serial.println("Initializing Setup");
  SPI.begin();
  rover.powerDownRadio();
  rover.initRadio();
  // Serial1.begin(115200);
}
enum State { WAKE, HANDSHAKE, UPDATE, POLL, UPLOAD, SLEEP };        //enums for rover state

static State state = WAKE;

void loop() {

  /* Print out rover diagnostic information if 1 has been typed */
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == '1') {
      Serial.println("------- ROVER DIAG --------");
      Serial.println();
    }
  }

  /* Start state */
  switch (state) {

    /* Wake up from sleep */
    case WAKE: 

      //Turns on radio and waits 20 seconds to properly initialize
      rover.wake();
      Serial.println("Radio enabled.");
      rover.debugRTCPrint();
      state = HANDSHAKE;
      break;

    /* Request communication from base, then return to sleep or intialize RTK process */
    case HANDSHAKE: //MARK;

      // 1. Send message to base, radiohead will tell us if it receives it
      // 2. Decide on going to sleep with or without a modified timer, or initialize RTK process
      
      // if(rover.request()){
      //   Serial.println("Transitioning to UPDATE...");

      //   state = UPDATE;
      // }else{
      //   state = SLEEP;
      //   break;
      // }
      // rover.request();
      // Serial1.println("Test");
      state = HANDSHAKE;
      // Serial.println("Transitioning to handshake...");
      // delay(2000);
      rover.listen();
      break;
      

    /* Transition to RTK, turn on GNSS */
    case UPDATE: MARK;

      // 1. Turn on GNSS
      // 2. Set and start gnss timer
      // 3. Set multiplexer so received radio input is piped to GNSS
      
      state = POLL;
      break;

    /* Called continuously, updates GNSS positioning*/
    case POLL: MARK;
      
      // 1. Poll GNSS 
      // 2. Log data if data is at threshold 
      // 3. Check if timer is done, if so, transition
      state = UPLOAD;
      break;

    case UPLOAD: MARK;
      
      // 1. Turn off GNSS
      // 2. Send data to base
    

      state = SLEEP;
      break;

    case SLEEP: MARK;

      // 1. Turn off everything
      // 2. Set wake alarm
      // 3. Wait for interrupt
      
      Serial.println("Transitioning to WAKE...");
      state = WAKE;
      break;
  }
}
