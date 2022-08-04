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
SSModel model;
ConManager manager;
FSController fsController(SD_CS, SD_RST);
SdFat SD;
File myFile;
GNSSController *gnssController;
static GNSSController _gnssController(Serial1, 115200, GNSS_RX, GNSS_TX, INIT_LOG_FREQ); 


void setup() {
  Serial.begin(115200); //functions rely on serial being begun
  SPI.begin();
  
  // Serial.println("Initializing card...");
  // if (!SD.begin(SD_CS)) {
  //   Serial.println("Initialization failed...");
  // }
  // Serial.println("Card initialized...");

  gnssController = &_gnssController;

  Serial1.begin(115200);
}

enum State { WAKE, DEBUG, HANDSHAKE, PREPOLL, UPDATE, POLL, UPLOAD, SLEEP };        //enums for rover state

static State state = DEBUG;

bool hasBeenCalled = false;

void loop() {
  Serial.println("Test");
  delay(1000);
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
    case DEBUG:
      Serial.println("DEBUG mode...");
      rover.wake();

      state = WAKE;
      break;

    /* Wake up from sleep */
    case WAKE: 

      rover.debugRTCPrint();
      Serial.println("WAKE mode...");
      //gnssController.populateGNSSMessage();
      //gnssController.populateGNSSMessage_Ben();

      if (fsController.init()) {
        Serial.println("Initialized SD card...");
      }
      else {
        Serial.println("Failed to initialize SD card...");
      }

      fsController.setupWakeCycle(rover.getTimeStamp(), gnssController->getFormat());
      //rover.getTimeStamp();
      

      state = HANDSHAKE;
      break;

    /* Request communication from base, then return to sleep or intialize RTK process */
    case HANDSHAKE: //MARK;
      Serial.println("HANDSHAKE mode...");

     
      // fsController.logData(model.toDiag());
      // fsController.logData(model.toProp());

      fsController.logData(model.toData(model.getProp(THRESHOLD)));

      state = SLEEP;
      break;

    case PREPOLL: MARK;

    case POLL: MARK;

    case UPLOAD: MARK;

    case SLEEP: MARK;

      Serial.println("SLEEP mode...");
      state = DEBUG;
      
      break;
  }
}
