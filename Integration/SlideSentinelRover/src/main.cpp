#include "config_2.0.0.h"
#include "network_config_2.0.0.h"
// #include "pcb_2.0.0.h"
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "FeatherTrace.h"
#include "Rover.h"

// Improve:
// - Make these vars accessible in their respective object rather than being global,
//  it's viable right now.
Rover rover;

void setup() {
  Serial.begin(115200); //functions rely on serial being begun
  delay(3500);
  Serial.println("Begginning");
  delay(3500);
  Serial.println("Begginning");
  delay(3500);
  Serial.println("Begginning");
  // rover.powerDownRadio();
  delay(3500);
  // rover.powerRadio();
  // Serial.begin(115200);
  // pinMode(LED_BUILTIN, OUTPUT);
  // digitalWrite(LED_BUILTIN, LOW);
  // delay(3500);           //delay to allow screening in
  // Serial.println("Initializing Setup");
  SPI.begin();


  // rover.powerDownRadio();
  delay(3000);
  rover.initRTC();
  // rover.powerRadio();
  // rover.initRadio();

  // SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; //enable deep sleep mode

  Serial1.begin(115200);

  // Serial1.begin(115200);
}
enum State { WAKE, DEBUG, HANDSHAKE, PREPOLL, UPDATE, POLL, UPLOAD, SLEEP };        //enums for rover state

static State state = DEBUG;
// GNSSController gnss1(Serial1, 115200, 12,11,30);

bool hasBeenCalled = false;

void loop() {
  Serial.println("Test");
  delay(1000);
  // if(!hasBeenCalled){
  //   // gnss1.init();
  //   hasBeenCalled = true;
  // }
  
  // if(Serial1.peek() != -1){
  //   Serial.println(Serial1.read());
  // }

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
      rover.wake();
      rover.powerRadio();
      delay(1000);
      Serial.println("Radio enabled.");

      state = SLEEP;
      break;

    /* Wake up from sleep */
    case WAKE: 

      //Turns on radio and waits 20 seconds to properly initialize
     // rover.wake();
      Serial.println("Radio enabled.");
      state = HANDSHAKE;
      break;

    /* Request communication from base, then return to sleep or intialize RTK process */
    case HANDSHAKE: //MARK;
      delay(1000);


      // 1. Send message to base, radiohead will tell us if it receives it
      // 2. Decide on going to sleep with or without a modified timer, or initialize RTK process

      // if(rover.request()){

      //   Serial.println("Contact established... transitioning to poll");
      //   state = POLL;

      // }else{

      //   Serial.println("Unable to communicate with base.");
      //   state = SLEEP;  //transistion to sleep upon failed communication

      // }
      // break;

    /* Transition to RTK, turn on GNSS */
    case PREPOLL: MARK;

      // 1. Turn on GNSS
      // 2. Set and start gnss timer
      // 3. Set multiplexer so received radio input is piped to GNSS
      
      // rover.powerGNSS();
      // rover.scheduleRTKAlarm(DS3231_A1_Minute);
      // rover.setMux(Rover::RadioToGNSS);
      
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
      rover.powerDownGNSS();
      rover.powerDownRadio();

      // rover.scheduleSleepAlarm();
      rover.scheduleAlarm(15); //15 second manual alarm
      rover.attachAlarmInterrupt();

     
      rover.toSleep();
      
      
      Serial.println("Transitioning to WAKE...");
      // state = WAKE;
      state=DEBUG;
      
      break;
  }
}
