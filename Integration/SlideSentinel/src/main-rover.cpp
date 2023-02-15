#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "FeatherTrace.h"
#include "Rover.h"

// If set to true shorten times to match that of a GNSS sim fix
#define SIMULATION_MODE false

/* Initialize the Rover class to use Serial2 as its communication */
Uart Serial2(&sercom1, GNSS_RX, GNSS_TX, SERCOM_RX_PAD_3, UART_TX_PAD_0);
void SERCOM1_Handler() { Serial2.IrqHandler(); }

Rover rover(Serial2);
                          
void setup() {

  //Show rover is on
  digitalWrite(LED_BUILTIN,HIGH);

  /* Set serial to radio, enable RS232 level shifter and start user monitor*/
  rover.setMux(Rover::MuxFormat::RadioToFeather);

  Serial.begin(115200); 
 // rover.waitForSerial();
  Serial.println("1");

  SPI.begin(); //TODO line is critical for relay driver. should move to rover, like rover.spiBegin();

  // Turn on the Radio and GNSS
  rover.powerRadio();
  rover.powerGNSS();
  
  // Initialize software rover components
  rover.initRover();

}

/**
 * Enum for the rover state
 * WAKE - Waking up from sleep, reinitialize
 * HANDSHAKE - Send a request to the base to transition to RTK mode
 * PREPOLL - Sent the pause to allow for an RTK fix
 * POLL - Retrieve data from the GNSSController allowing for enough time to get a fix
 * UPLOAD - Send the data collected to the base
 * SLEEP - Enter a low power state and wait for RTC wake
*/
enum State { WAKE, HANDSHAKE, PREPOLL, POLL, UPLOAD, SLEEP, DEBUG, DEBUG_RADIO};        

// Current state of the rover
static State state = WAKE;

void loop() {

  /* Checks if input has been sent and debug information needs to bre printed*/
  rover.debugInformation();

  switch(state) {

    /* Initialize / Wakeup */
    case WAKE:

      /* Turn power LED and print that the device is now awake*/
      digitalWrite(LED_BUILTIN,HIGH);

      Serial.println("[Rover] Entering Wake State");
      Serial.println("[Rover] Powering up radio, this will take about 20 seconds...");

      /****** RADIO *******/
      rover.powerRadio();
      rover.setMux(Rover::MuxFormat::RadioToFeather);

      // Wait 20 seconds for the radio to warmup
      rover.setFeatherTimerLength(20*1000);
      rover.startFeatherTimer();
      while(!rover.isFeatherTimerDone());
      Serial1.println("exit");  //ensure radio is out of programming mode

      Serial.println("[Rover] Radio warmup completed!");

      /******* RTC *******/
      rover.setRTCTime(); // Turns on RTC for correct timestamp

      state = HANDSHAKE;
      break;
    
    /* Request a RTK transition from the base*/
    case HANDSHAKE:

      // Package a REQUEST message
      rover.packageData(Rover::DataType::REQUEST);

      // Transmit the request and upon a successful transmission we want to wait for a response
      if(rover.transmit()){

        // Wait for base response
        if(rover.waitAndReceive()){

          // If the response was a "INIT_RTK_TYPE" we know that we are in contact with the base
          if(rover.getMessageType() == "INIT_RTK_TYPE"){
            Serial.println("[Rover] Successfully transitioning to RTK mode");
            state = PREPOLL;
            break;
          
          // If not we inform the user that the response was un expected and print the contents of that message
          }else{
            Serial.println("[Rover] Unexpected response to rtk request");
            Serial.println("******************************");
            Serial.println(rover.getMessageType());
            Serial.println(rover.getMessageBody());
            Serial.println("******************************");
          }
        }else{
          Serial.println("[Rover] No message received after rtk request... Transitioning to sleep");
          delay(1000);
          state = SLEEP;
        }
          
      }

      //If we haven't already transitioned out of handshake we want to run it again
      Serial.println("[Rover] Transitioning to handshake");
      
      state = HANDSHAKE;
      break;
    case PREPOLL:
      
      // Set a timer for 20 seconds and power the GNSS on 10 minutes, or if sim just 10
      #if SIMULATION_MODE == true 
      rover.setFeatherTimerLength(10000);
      #else
      rover.setFeatherTimerLength(1000*60*5);
      #endif
      
      rover.powerGNSS();
      rover.setMux(Rover::RadioToGNSS);
      rover.startFeatherTimer();

      state = POLL;
      break;

    case POLL:

      // Poll GNSS data until timer is done then transition to upload
      if(!rover.isFeatherTimerDone()){
        // Poll until the timer is done, this will stop getting new data when a fix is acquired we just want to keep everything in sync
        rover.poll();
      }else{
        state = UPLOAD;
        break;
      }

      state = POLL;
      break;
    case UPLOAD: 

      // Set the serial mux to direct communication to the Radio
      rover.setMux(Rover::MuxFormat::RadioToFeather);
      delay(50);

      // Package data for upload (serializes GNSS data into the MSG)
      rover.packageData(Rover::DataType::UPLOAD);
      Serial.println("[Rover] Uploading data to base...");

      // Transmit the packaged data to the base
      if(rover.transmit()){
        Serial.println("[Rover] Data uploaded successfully!");
      }else{
        Serial.println("[Rover] Catastrophic upload failure.");
      }

      state = SLEEP;
      break;
    case SLEEP: 

      // Power down GNSS and RADIO
      rover.powerDownGNSS();
      rover.powerDownRadio();

      #if SIMULATION_MODE == true 
        // Schedule Alarm 15 seconds from now
        rover.scheduleAlarm(15);
      #else
        // Schedule Alarm for 15 minutes from now
        rover.scheduleSleepAlarm();
      #endif

      // Disable power light
      digitalWrite(LED_BUILTIN,LOW);

      // Attach to the interrupt and enter sleeping waiting for interrupt
      rover.attachAlarmInterrupt();
      rover.toSleep();
      state = WAKE;
      break;
    
    /* Test GNSS fix + transmit */
    case DEBUG:
      rover.setMux(Rover::MuxFormat::RadioToGNSS);
      
      rover.setFeatherTimerLength(10*1000);
      rover.startFeatherTimer();

      while(!rover.isFeatherTimerDone()){
        rover.poll();
      }

      rover.setMux(Rover::MuxFormat::RadioToFeather);
      rover.packageData(Rover::DataType::UPLOAD);

      rover.transmit();

      
      // if(rover.sd)
      delay(500);
      break;
    
    /* Send data to radio if not receiving data*/
    case DEBUG_RADIO:
      while(true){
        if(Serial1.available()){
          Serial.print((char)Serial1.read());
        }else{
          Serial1.println("Ping");
        }
      }
      break;
  }
}
