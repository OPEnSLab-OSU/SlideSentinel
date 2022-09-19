#include "config_2.0.0.h"
#include "network_config_2.0.0.h"
// #include "pcb_2.0.0.h"
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "FeatherTrace.h"
#include "Rover.h"



/* Ran on first bootup of Main. Pass in */
Uart Serial2(&sercom1, GNSS_RX, GNSS_TX, SERCOM_RX_PAD_3, UART_TX_PAD_0);
void SERCOM1_Handler() { Serial2.IrqHandler(); }

Rover rover(Serial2);

// static GNSSController _gnssController(Serial2, GNSS_BAUD, GNSS_RX, GNSS_TX,
//                                         INIT_LOG_FREQ); 
// GNSSController *gnssController;
                          
void setup() {
  rover.setMux(Rover::MuxFormat::RadioToFeather);
  rover.setRS232(true);
  Serial.begin(115200); //functions rely on serial being begun
  Serial.println("1");
  Serial1.begin(115200);
  Serial1.println("exit");
  /*Radio Debug*/
  // while(true){
  //   if(Serial1.available()){
  //     Serial.print((char)Serial1.read());
  //   }else{
  //     Serial1.println("Ping");
  //   }
  // }
  /*End Radio Debug*/
  delay(5000);
  // gnssController = &_gnssController;
  // gnssController->init(); //this is the true cause

  SPI.begin(); //TODO line is critical for relay driver. should move to rover, like rover.spiBegin();

  rover.powerRadio();

  rover.powerGNSS();
  Serial1.begin(115200);
  rover.initRover();

}

enum State { WAKE, DEBUG, HANDSHAKE, PREPOLL, UPDATE, POLL, UPLOAD, SLEEP };        //enums for rover state

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

  switch(state) {
    case DEBUG:
      // if(rover.sd)
      delay(500);
      break;
    case WAKE:
      digitalWrite(LED_BUILTIN,HIGH);

      Serial.println("WAKE mode...");

      /****** RADIO *******/
      rover.powerRadio();
      rover.setMux(Rover::MuxFormat::RadioToFeather);

      rover.setFeatherTimerLength(10*1000);
      rover.startFeatherTimer();
      while(!rover.isFeatherTimerDone());
      // delay(5000);
      Serial.println("Radio warmup completed");

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

      state = HANDSHAKE;
      break;
    case HANDSHAKE:
      rover.packageData(Rover::DataType::REQUEST);
      if(rover.transmit()){
        if(rover.waitAndReceive()){
          if(rover.getMessageType() == "INIT_RTK_TYPE"){
            Serial.println("Successfully transitioning to rtk mode");
            state = PREPOLL;
            break;
          }else{
            Serial.println("Unexpected response to rtk request...");
            Serial.println("******************************");
            Serial.println(rover.getMessageType());
            Serial.println(rover.getMessageBody());
            Serial.println("******************************");
          }
        }else{
          Serial.println("No message receieved after rtk request... Transitioning to sleep");
          delay(1000);
          // state = SLEEP;
        }
          
      }
      Serial.println("Transitioning to handshake");
      
      state = HANDSHAKE;
      //state = PREPOLL;
      break;
    case PREPOLL:
      // rover.scheduleAlarm(600); // 10 minute timer 
      rover.setFeatherTimerLength(1000*20);
      rover.powerGNSS();
      rover.setMux(Rover::RadioToGNSS); // Inits multiplexer
      rover.startFeatherTimer();
      // rover.setFeatherTimerLength(1000*60*10); //convert to seconds->minutes->chosen length
      state = POLL;
      break;

    case POLL:
      if(!rover.isFeatherTimerDone()){//check if timer is completed
        // gnssController->poll();
        rover.poll();
        // if(gnssController->isNewData()){
        //   Serial.println(gnssController->populateGNSS());   
        // }
      }else{
        state = UPLOAD;
        break;
      }

      state = POLL;
      break;
    case UPLOAD: 
      rover.setMux(Rover::MuxFormat::RadioToFeather);
      rover.packageData(Rover::DataType::UPLOAD);
      Serial.println("Uploading");
      if(rover.transmit()){
        Serial.println("Data uploaded successfully!!!");
      }else{
        Serial.println("Catastrophic upload failure...");
      }
      state = SLEEP;
      break;
    case SLEEP: 
      rover.powerDownGNSS();
      rover.powerDownRadio();
      // rover.scheduleSleepAlarm();
      rover.scheduleAlarm(20);
      digitalWrite(LED_BUILTIN,LOW);

      rover.attachAlarmInterrupt();
      rover.toSleep();
      state = WAKE;
      break;
  }
}
