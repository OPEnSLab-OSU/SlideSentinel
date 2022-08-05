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
 
  gnssController = &_gnssController;
  manager.add(&_gnssController);
  if (!manager.init()) {
    Serial.println("Manager failed to initialize...");
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
  }
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

  /* Start state */
  switch (state) {
    case DEBUG:
      Serial.println("DEBUG mode...");
      rover.wake();

      state = WAKE;
      break;

    /* Wake up from sleep */
    case WAKE: 

      rover.debugRTCPrint(); // Turns on RTC for correct timestamp
      Serial.println("WAKE mode...");
      Serial.println("Initializing SD card...");

      if (!fsController.init()) { // Initializes SD card
        Serial.println("Failed to initialize SD card...");
      }

      if (fsController.check_init()) { // Checks if initialized
        fsController.setupWakeCycle(rover.getTimeStamp(), gnssController->populateGNSS()); // Responsible for creating files 
      }

      state = HANDSHAKE;
      break;

    /* Request communication from base, then return to sleep or intialize RTK process */
    case HANDSHAKE: //MARK;
      Serial.println("HANDSHAKE mode...");

      if (fsController.check_init()) { // Checks if initialized
        fsController.logData(gnssController->populateGNSS()); // Logs GNSS data into SD
        fsController.logDiag(model.toDiag()); // Logs diag to SD
      }

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
