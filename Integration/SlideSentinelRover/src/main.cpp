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

// Improve:
// - Make these vars accessible in their respective object rather than being global,
//  it's viable right now.
Rover rover;
RTC_DS3231 m_RTC_main;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  delay(3500);           //delay to allow screening in
  Serial.println("Initializing Setup");
  SPI.begin();
  rover.powerDownRadio();
  rover.initRadio();

  m_RTC_main.begin();
  m_RTC_main.adjust(DateTime(F(__DATE__), F(__TIME__)));//set date-time manualy:yr,mo,dy,hr,mn,sec
  // SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; //enable deep sleep mode

  Serial1.begin(115200);

  // Serial1.begin(115200);
}
enum State { WAKE, HANDSHAKE, UPDATE, POLL, UPLOAD, SLEEP };        //enums for rover state

static State state = WAKE;
GNSSController gnss1(Serial1, 115200, 12,11,30);

bool hasBeenCalled = false;

void loop() {
  if(!hasBeenCalled){
    gnss1.init();
    hasBeenCalled = true;
  }
  
  // if(Serial1.peek() != -1){
  //   Serial.println(Serial1.read());
  // }
  gnss1.poll();
  Serial.println(gnss1.m_pos_llh.lat, 10);
  Serial.println(gnss1.m_pos_llh.lon, 10);
  Serial.println(gnss1.m_pos_llh.height);

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
     // rover.wake();
      Serial.println("Radio enabled.");
      rover.debugRTCPrint();
      state = HANDSHAKE;
      break;

    /* Request communication from base, then return to sleep or intialize RTK process */
    case HANDSHAKE: //MARK;
      delay(1000);
      //rover.printRTCTime();
      rover.printRTCTime_Ben(m_RTC_main);
      rover.timeDelay(m_RTC_main);
      rover.rtc_alarm(m_RTC_main);
      //state = HANDSHAKE;

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
     // rover.listen();
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
      __WFI(); //wait for interrupt
      
      Serial.println("Transitioning to WAKE...");
      state = WAKE;
      break;
  }
}
