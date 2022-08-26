#include "config_2.0.0.h"
#include "network_config_2.0.0.h"
// #include "pcb_2.0.0.h"
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "FeatherTrace.h"
#include "Rover.h"

Rover rover;
// ConManager manager;
// FSController fsController(SD_CS, SD_RST);
// SdFat SD;
// File myFile;
// GNSSController *gnssController;
// static GNSSController _gnssController(Serial1, 115200, GNSS_RX, GNSS_TX, INIT_LOG_FREQ); 
Uart Serial2(&sercom1, GNSS_RX, GNSS_TX, SERCOM_RX_PAD_3, UART_TX_PAD_0);
void SERCOM1_Handler() { Serial2.IrqHandler(); }


static GNSSController _gnssController(Serial2, GNSS_BAUD, GNSS_RX, GNSS_TX,
                                        INIT_LOG_FREQ); 
GNSSController *gnssController;
                          
void setup() {
  Serial.begin(115200); //functions rely on serial being begun

  Serial.println("1");
  delay(5000);
  gnssController = &_gnssController;
  gnssController->init(); //this is the true cause

  SPI.begin(); //TODO line is critical for relay driver. should move to rover, like rover.spiBegin();

  Serial.println("2");
  delay(100);
  // rover.powerDownRadio();
  // delay(3000);
<<<<<<< HEAD
=======
  // rover.initRTC(); could break here
  rover.powerRadio();
  rover.initRHParams();
  // rover.initRadio(); //something breaks here
    Serial.println("3");
>>>>>>> fe35a69d19dc3702d63f0dfc4b37b4370bfb051f

  Serial.println("3");
  // SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; //enable deep sleep mode

  //Serial1.begin(115200);
<<<<<<< HEAD

=======
  rover.powerGNSS();
  rover.setRS232(true);
>>>>>>> fe35a69d19dc3702d63f0dfc4b37b4370bfb051f
  // Serial1.begin(19200);
}

enum State { WAKE, DEBUG, HANDSHAKE, PREPOLL, UPDATE, POLL, UPLOAD, SLEEP };        //enums for rover state

static State state = WAKE;

void loop() {
  // delay(50);
  /* Print out rover diagnostic information if 1 has been typed */
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == '1') {
      Serial.println("------- ROVER DIAG --------");
      Serial.println();
    }
  }

  /* Starting state. The flow of the program. */

  switch(state) {
    case WAKE:
      Serial.println("WAKE mode...");

      /****** RADIO *******/
      rover.powerRadio();
      rover.initRHParams();

      /******* GNSS *******/
      // delay(10);
      // gnssController->poll();
      // if(gnssController->isNewData()){
      //   Serial.println(gnssController->populateGNSS());   
      // }
      // else{
      //   Serial.println("No data");
      // }


      /******* RTC *******/
      rover.debugRTCPrint(); // Turns on RTC for correct timestamp

      // /******* SD CARD ********/      
      // if (!fsController.init()) { // Initializes SD card
      //   Serial.println("Failed to initialize SD card...");
      // }

      // if (fsController.check_init()) { // Checks if initialized
      //   fsController.setupWakeCycle(rover.getTimeStamp(), gnssController->populateGNSS()); // Responsible for creating files 
      // }


      state = HANDSHAKE;
      break;
    case HANDSHAKE:
      rover.packageData(Rover::DataType::UPLOAD);
      if(rover.transmit()){
        Serial.println("Message sent successfully!");
      }
      else {
        delay(500);
        state = HANDSHAKE;
      }

      state = PREPOLL;
      break;
    case PREPOLL:
      // rover.scheduleAlarm(600); // 10 minute timer 
      rover.powerGNSS();
      rover.setRS232(true);
      rover.setMux(Rover::RadioToGNSS); // Inits multiplexer
      state = POLL;
      break;
    case POLL:
      gnssController->poll();
      if(gnssController->isNewData()){
        Serial.println(gnssController->populateGNSS());   
      }
      // else{
      //   Serial.println("No data");
      // }


      state = POLL;
      break;
    // case UPLOAD: 
    //   state = SLEEP;
    //   break;
    // case SLEEP: 
    //   state = WAKE;
    //   break;
  }
}
