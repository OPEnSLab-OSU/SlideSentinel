#include "config_2.0.0.h"
#include "network_config_2.0.0.h"
// #include "pcb_2.0.0.h"
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "FeatherTrace.h"
#include "Rover.h"

Rover rover;
SSModel model;
// ConManager manager;
// FSController fsController(SD_CS, SD_RST);
// SdFat SD;
// File myFile;
GNSSController *gnssController;
static GNSSController _gnssController(Serial1, 115200, GNSS_RX, GNSS_TX, INIT_LOG_FREQ); 


void setup() {
  Serial.begin(115200); //functions rely on serial being begun
  SPI.begin();


  // rover.powerDownRadio();
  delay(3000);
  rover.initRTC();
  // rover.powerRadio();
  rover.initRadio();

  // SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; //enable deep sleep mode

  //Serial1.begin(115200);

  Serial1.begin(19200);
}

enum State { WAKE, DEBUG, HANDSHAKE, PREPOLL, UPDATE, POLL, UPLOAD, SLEEP };        //enums for rover state

static State state = DEBUG;

void loop() {
  delay(1000);
  /* Print out rover diagnostic information if 1 has been typed */
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == '1') {
      Serial.println("------- ROVER DIAG --------");
      Serial.println();
    }
  }

  /* Starting state. The flow of the program. */
  switch (state) {
    case DEBUG:
      // rover.wake();
      rover.powerRadio();
      delay(1000);
      Serial.println("Radio enabled.");

      rover.sendManualMsg("Testing 123");
      

      state = DEBUG;
      break;

    /* Wake up from sleep */
    case WAKE: 
      rover.debugRTCPrint(); // Turns on RTC for correct timestamp
      Serial.println("WAKE mode...");
      Serial.println("Initializing SD card...");

      // if (!fsController.init()) { // Initializes SD card
      //   Serial.println("Failed to initialize SD card...");
      // }

      // if (fsController.check_init()) { // Checks if initialized
      //   fsController.setupWakeCycle(rover.getTimeStamp(), gnssController->populateGNSS()); // Responsible for creating files 
      // }

      state = HANDSHAKE;
      break;

    case HANDSHAKE: //MARK;
      delay(1000);

      Serial.println("I'm in handshake");
      rover.sendManualMsg("Hello");

      // 1. Send message to base, radiohead will tell us if it receives it
      // 2. Decide on going to sleep with or without a modified timer, or initialize RTK process
      rover.packageData(Rover::DataType::REQUEST);
      // if(rover.transmit()){

      //   Serial.println("Contact established... transitioning to poll");
      //   state = POLL;

      // }else{

      //   Serial.println("Unable to communicate with base.");
      state = SLEEP;  //transistion to sleep upon failed communication

      // }
      break;

    case PREPOLL: MARK;

    case POLL: MARK;

    case UPLOAD: MARK;
      
      // 1. Turn off GNSS
      // 2. Send data to base
      rover.packageData(Rover::DataType::UPLOAD);
      if(rover.transmit()){
        Serial.println("WARNING: Unsure if base received transmission");
      }

      state = SLEEP;
      break;

    case SLEEP: MARK;

      // 1. Turn off everything
      // 2. Set wake alarm
      // 3. Wait for interrupt
      rover.powerDownGNSS();
      rover.powerDownRadio();

      // rover.scheduleSleepAlarm();
      rover.scheduleAlarm(15*60); //15 minute alarm
      rover.attachAlarmInterrupt();

     
      rover.toSleep();
      
      
      Serial.println("Transitioning to WAKE...");
      // state = WAKE;
      state=DEBUG;
      
      break;
  }
}