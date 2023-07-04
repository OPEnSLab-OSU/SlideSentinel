#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "FeatherTrace.h"
#include "Rover.h"

/*
    @brief The run mode dictates the fucntions that rover will use.

    0: Live Deployment Mode
    1: Radio 2 way communication mode
    2: Radio 2 way reliable (RH) communication mode
    3: Relay diagnostic mode
    4: SD Mode
*/
#define RUN_MODE 1 

// If set to true shorten times to match that of a GNSS sim fix
#define SIMULATION_MODE false

/* Define function signatures */
void setInitialState();


/* Initialize the Rover class to use Serial2 as its communication */
Uart Serial2(&sercom1, GNSS_RX, GNSS_TX, SERCOM_RX_PAD_3, UART_TX_PAD_0);
void SERCOM1_Handler() { Serial2.IrqHandler(); }

Rover rover(Serial2);
int rtkRequestCount = 0;
                          
void setup() {
  switch(RUN_MODE){
    //Show rover is on
    digitalWrite(LED_BUILTIN,HIGH);
    case 0: //Field Deployment/Standard Mode

      /* Set serial to radio, enable RS232 level shifter and start user monitor*/
      rover.setMux(Rover::MuxFormat::RadioToFeather);

      Serial.begin(115200); 
      // rover.waitForSerial();
      Serial.println("1");
      Serial.println("[SETUP] Entering default field deployment setup");

      SPI.begin(); //TODO line is critical for relay driver. should move to rover, like rover.spiBegin();

      // Turn on the Radio and GNSS
      rover.powerRadio();
      rover.powerGNSS();
      
      // Initialize software rover components
      rover.initRover();

      break;

    case 1: //Radio initialization mode
    case 2:

      rover.setMux(Rover::MuxFormat::RadioToFeather);
      Serial.begin(115200);
      Serial.println("[SETUP] Entering Radio Debug Setup");

      SPI.begin();
      rover.powerDownRadio();
      delay(500);
      rover.powerDownGNSS(); //Turn off as we don't need this for radio test
      rover.powerRadio();
      rover.initRoverRadioTest();
      break;
  }
  setInitialState();
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

// Current default state of the rover, depending on RUN_MODE
static State state;
void setInitialState(){
  switch(RUN_MODE){
    case 0:
      state = WAKE;
      Serial.println("[PRELOOP State Switch] Setting state to Wake");
      break;
    case 1:
      state = DEBUG_RADIO;
      Serial.println("[PRELOOP State Switch] Setting state to Radio Debug");
      break;
    default:
      state = WAKE;
      Serial.println("[PRELOOP State Switch] Default: Setting state to Wake");
      break;
  }
}

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

      Serial1.println("exit");  //ensure radio is out of programming mode

      Serial.println("[Rover] Radio warmup completed!");

      /******* RTC *******/
      rover.setRTCTime(); // Turns on RTC for correct timestamp

      #if SIMULATION_MODE == false
        // Schedule Alarm for 15 minutes from now
        rover.scheduleSleepAlarm();
      #endif

      #if SIMULATION_MODE == false
        state = HANDSHAKE;
      #else
        state = PREPOLL;
      #endif
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
            // If the time was sent in the message then we should adjust the RTC to match
            if(rover.getMessageBody().length() > 0){
              Serial.println("[Rover] Matching RTC time with SatComm");
              rover.adjustRTCTime(rover.getMessageBody().c_str());
            }
            
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
          break;
        }
          
      }
      else if(rtkRequestCount == 3){
        Serial.println("[Rover] After 5 retries the rover has failed to initiate a connection to the base and will now enter sleep");
        rtkRequestCount = 0;
        state = SLEEP;
        break;
      }

      //If we haven't already transitioned out of handshake we want to run it again
      Serial.println("[Rover] Transitioning to handshake");
      rtkRequestCount++;
      state = HANDSHAKE;
      break;
    case PREPOLL:
      
      // Set a timer for 20 seconds and power the GNSS on 10 minutes, or if sim just 10
      #if SIMULATION_MODE == true 
      rover.setFeatherTimerLength(10000);
      #else
      rover.setFeatherTimerLength(1000*60*5);
      #endif

      Serial.println("[Rover] Only fixed RTK values will be printed...");
      
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

      // Set the serial mux to direct communication to the Radio, turn the radio back on before uploading
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
      delay(500);
      break;
    
    /* Send data to radio if not receiving data*/
    case DEBUG_RADIO:
      rover.setFeatherTimerLength(3*1000);
      rover.startFeatherTimer(); 

      // Print all available radio messages received, transmit ping ever 3 seconds
      while(true){
        if(Serial1.available()){
          Serial.print((char)Serial1.read());
        }else{
          if(rover.isFeatherTimerDone()){
            Serial1.println("Ping");
            rover.startFeatherTimer();
          }
        }
      }
      break;
  }
}
