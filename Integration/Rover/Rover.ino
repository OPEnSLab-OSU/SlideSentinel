// ================================================================
// ===              INCLUDE CONFIGURATION FILE                  ===
// ===    INCLUDE DECLARATIONS, STRUCTS, AND FUNCTIONS FROM     ===
// ===            OTHER FILES AS SET IN CONFIG.H                ===
// ================================================================

// ********************** PROGRAM DEXCRIPTION *********************
// This is the "functioning" code for the slide sentinel node with
// working everythurrrrr
// ****************************************************************

// Config has to be first has it hold all user specified options
#include "config.h"

// Preamble includes any relevant subroutine files based
// on options specified in the above config
#include "loom_preamble.h"

#include <Arduino.h>        // required before wiring_private.h
#include "wiring_private.h" // pinPeripheral() function
#include "LowPower.h"       // from sparkfun low power library found here https://github.com/rocketscream/Low-Power
#include "RTClibExtended.h" // from sparkfun low power library found here https://github.com/FabioCuomo/FabioCuomo-DS3231/
#include <Wire.h>
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>
#include <EnableInterrupt.h>
#include "wiring_private.h" // pinPeripheral() function
#include "SlideS_parser.h"
#include <RTClibExtended.h>

/***************************************
 * Notes on Power consumption: 
 * 
 * Low Power:             4.53 mA 
 * Polling for position:  ~170 mA
 * Trasmitting:           ~190 mA (Z9-T require power supply which can supply 800mA, radio nominally draws 50mA during tx)
 * Solar Charging:        Provides up to 180 mA current (direct sunlight on a bright day)
 *                        In shaded enviroments, provides ~25 mA current
 * ************************************/

// Library edits for libraries that don't have #ifdef statements :(

// IMPORTANT: Edit the feather m0 variant.cpp to comment out lines:
// ~/Library/Arduino15/packages/adafruit/hardware/samd/1.2.9/variants/feather_m0/variant.cpp
// Uart Serial5( &sercom5, PIN_SERIAL_RX, PIN_SERIAL_TX, PAD_SERIAL_RX, PAD_SERIAL_TX ) ;
// void SERCOM5_Handler()
// {
//   Serial5.IrqHandler();
// }

// IMPORTANT: Edit the #define SERIAL_BUFFER_SIZE 164 to read #define SERIAL_BUFFER_SIZE 512
//            located in the path /.arduino15/packages/adafruit/hardware/samd/1.3.0/cores/arduino
//            Mac:        ~/Library/Arduino15/packages/adafruit/hardware/samd/1.2.9/cores/arduino/RingBuffer.h

// IMPORTANT: Must include the following line in the RTClibExtended.h file to use with M0:
//#define _BV(bit) (1 << (bit))

#pragma message "SERIAL_BUFFER_SIZE=" SERIAL_BUFFER_SIZE // print the value of the preprocessor defined serial buffer, make sure this is 512

/* function declarations */
void mmaCSVRead(Adafruit_MMA8451 device, String &to_fill, int count);
void configInterrupts(Adafruit_MMA8451 device);
void mmaPrintIntSRC(uint8_t dataRead);
void mmaSetupSlideSentinel();

#define DEBUG 1 // allow printing to serial monitor,
#define DEBUG_SD 1
#define CELLULAR 0

// Define mode constants
#define RTC_MODE 1 // enable RTC interrupts
#define SD_WRITE 1 // enable SD card logging
#define NODE_NUM 0 // ID for node
#define GPS_BUFFER_LEN 500
#define BAUD 115200 // reading and writing occurs at

// ======== Timer periods for different measurement conditions ==========
#define RTC_WAKE_PERIOD 120 // Interval to wake and take sample in Min, reset alarm based on this period (Bo - 5 min), 15 min
#define STANDARD_WAKE 1800 // Length of time to take measurements under periodic wake condition,     5 mines in minutes
#define ALERT_WAKE 1800     // Length of time to take measurements under acceleration wake condition

// ======== Pin Assignments, no need to change ==========
// Other pins in use: 13, 10 for UART
// (can possibly use A5 and/or 13, they are defined as UART but unused in this implementation)
#define ACCEL_EN_PIN A1    // Interrupt driven, connect to switch for toggle, REMOVE
#define GPS_EN_PIN A2      // Attach to RESET on relay
#define GPS_DISABLE_PIN A4 // Attach to SET on relay
#define ALERT_WAKE_PIN A3  // Attach A3 to int1 on accelerometer
#define VBATPIN A7         // Labeled pin 9
#define RTC_WAKE_PIN 5     // Attach to SQW pin on RTC
#define SERIAL2_RX 12      // Rx pin for first serial port
#define SERIAL2_TX 11      // Tx pin for first serial port
#define SERIAL3_RX A5
#define SERIAL3_TX 6
#define BATTERYPIN A0 //ADDED

// Holds an NMEA string
struct nmeaData
{
  char data[GPS_BUFFER_LEN];
  int len;
};

// Object instantiation
Adafruit_MMA8451 mma = Adafruit_MMA8451();
RTC_DS3231 RTC_DS; //Rover code does not use Loom RTC implementation

// ======== Hardware Serial Port 2 ==========
// RX pin 12, TX pin 11, configuring for rover UART
Uart Serial2(&sercom1, SERIAL2_RX, SERIAL2_TX, SERCOM_RX_PAD_3, UART_TX_PAD_0);
void SERCOM1_Handler()
{
  Serial2.IrqHandler();
}

// ======== Hardware Serial Port 3, SLIP Encoding ==========
Uart Serial3(&sercom5, SERIAL3_RX, SERIAL3_TX, SERCOM_RX_PAD_0, UART_TX_PAD_2);
void SERCOM5_Handler()
{
  Serial3.IrqHandler();
}

// Global variables
sensors_event_t event;
struct nmeaData dataIn;
String accel_data;
bool accelFlag;
unsigned long int timer, count, temp_timer, alert_timer;
String RTC_monthString, RTC_dayString, RTC_hrString, RTC_minString, RTC_timeString = "", stringTransmit = "";

const char *CurrentWakeGPS = "CRWKG";
const char *AllLogs = "ALL.TXT";
const char *CurrentWakeState = "CRWKS";

// Declare RTC/Accelerometer specific variables
volatile bool TakeSampleFlag = false; // Flag is set with external Pin A0 Interrupt by RTC
volatile bool alertFlag = false;      // Flag if an alert is triggered and we need to speed up some processes (Phase 2)
volatile bool accelEnFlag = false;
volatile bool accelEn;
volatile bool TimerFlag = false;
volatile bool RTCFlag = false;
volatile bool stateSent = false;

volatile int HR = 8;        // Hr of the day we want alarm to go off
volatile int MIN = 0;       // Min of each hour we want alarm to go off
volatile int awakeFor = 20; // number of seconds to stay awake and take measurements for

/* Interrupt service routine for accelerometer interrupts */
void wakeUpAccel()
{
  detachInterrupt(digitalPinToInterrupt(ALERT_WAKE_PIN));
  accelFlag = true;
}

/**********************************************************************************************
   wakeUp_()
   Description: function that takes place after device wakes up
   See: Interrupt Service Routine (ISR)
   wakeUp_alert
   - sets SampleFlag and alertFlag TRUE;
   wakeUp_RTC
   - sets SampleFlag TRUE upon typical wakeup
**********************************************************************************************/

void wakeUp_alert()
{
  detachInterrupt(digitalPinToInterrupt(ALERT_WAKE_PIN));
  accelFlag = true;
  alertFlag = true;
  TakeSampleFlag = true;
  TimerFlag = true;
}

void wakeUp_RTC()
{
  detachInterrupt(digitalPinToInterrupt(RTC_WAKE_PIN));
  TimerFlag = true;
  RTCFlag = true;
}

//no need to detach interrupt because interrupt is rising/falling and will only be triggered when not in standby
void accel_toggle()
{
  accelEnFlag = true;
}

void busyWait()
{
  while (1)
    ;
}

// ================================================================
// ===                           SETUP                          ===
// ================================================================
void setup()
{
  delay(5000);
  // LOOM_begin calls any relevant (based on config) LOOM device setup functions
  Loom_begin();
  Serial.begin(9600);

  memset(dataIn.data, '\0', GPS_BUFFER_LEN + 1);
  dataIn.len = 0;

  // see pin definitions
  Serial3Setup();
  Serial2Setup();

#if DEBUG
  //setupPrint();
#else
  delay(15000);
#endif

  mmaSetupSlideSentinel();

  accelFlag = false;
  configInterrupts(mma);

  digitalWrite(ALERT_WAKE_PIN, INPUT_PULLUP); // LOW interrupt
  attachInterrupt(digitalPinToInterrupt(ALERT_WAKE_PIN), wakeUpAccel, LOW);
  Serial.println("Interrupt attached");

  mma.readRegister8(MMA8451_REG_TRANSIENT_SRC); //When this register is read it clears the interrupt for the transient detection
  timer = millis();
  count = 0;

  /*initialize RTC*/
#if RTC_MODE == 1
  //RTC stuff init//
#if DEBUG == 1
  Serial.println("Setting up RTC");
#endif
  initializeRTC();

#if DEBUG == 1
  Serial.print("Alarm set to go off every ");
  Serial.print(RTC_WAKE_PERIOD);
  Serial.println(" min from program time");
#endif

#endif //RTC_MODE == 1

  timer = millis();
  temp_timer = timer;
  alert_timer = timer;

  /*initialize accelerometer and configure settings*/

  initializePins();

  accelEn = digitalRead(ACCEL_EN_PIN);
  accelInt(accelEn);
  gps_on();

  //  Any custom setup code
}

// ================================================================
// ===                        MAIN LOOP                         ===
// ================================================================
void loop()
{
  timerFlagCheck(); // set the timer each time an interrupt is triggered, prolong the wake period
  RTCFlagCheck();   // reset RTC interrupts
  enCheck();        // check accelerometer enable
  alertFlagCheck(); // reset accelerometer interrupts,
  readPSTI();       // read from serial port if available
  tryStandby();
} // End loop section

// comment/uncomment these to enable functionality described
/* Transient detection donfiguration for mma accelerometer, use this format and Adafruit_MMA8451::writeRegister8_public to configure registers */
void configInterrupts(Adafruit_MMA8451 device)
{
  uint8_t dataToWrite = 0;
  // MMA8451_REG_CTRL_REG2
  // sysatem control register 2

  //dataToWrite |= 0x80;    // Auto sleep/wake interrupt
  //dataToWrite |= 0x40;    // FIFO interrupt
  //dataToWrite |= 0x20;    // Transient interrupt - enabled
  //dataToWrite |= 0x10;    // orientation
  //dataToWrite |= 0x08;    // Pulse interrupt
  //dataToWrite |= 0x04;    // Freefall interrupt
  //dataToWrite |= 0x01;    // data ready interrupt, MUST BE ENABLED FOR USE WITH ARDUINO

  // MMA8451_REG_CTRL_REG3
  // Interrupt control register

  dataToWrite = 0;
  dataToWrite |= 0x80; // FIFO gate option for wake/sleep transition, default 0
  dataToWrite |= 0x40; // Wake from transient interrupt enable
  //dataToWrite |= 0x20;    // Wake from orientation interrupt enable
  //dataToWrite |= 0x10;    // Wake from Pulse function enable
  //dataToWrite |= 0x08;    // Wake from freefall/motion decect interrupt
  //dataToWrite |= 0x02;    // Interrupt polarity, 1 = active high
  dataToWrite |= 0x00; // (0) Push/pull or (1) open drain interrupt, determines whether bus is driven by device, or left to hang

  device.writeRegister8_public(MMA8451_REG_CTRL_REG3, dataToWrite);

  dataToWrite = 0;

  // MMA8451_REG_CTRL_REG4
  // Interrupt enable register, enables interrupts that are not commented

  //dataToWrite |= 0x80;    // Auto sleep/wake interrupt
  //dataToWrite |= 0x40;    // FIFO interrupt
  dataToWrite |= 0x20; // Transient interrupt - enabled
  //dataToWrite |= 0x10;    // orientation
  //dataToWrite |= 0x08;    // Pulse interrupt
  //dataToWrite |= 0x04;    // Freefall interrupt
  dataToWrite |= 0x01; // data ready interrupt, MUST BE ENABLED FOR USE WITH ARDUINO
  device.writeRegister8_public(MMA8451_REG_CTRL_REG4, dataToWrite | 0x01);

  dataToWrite = 0;

  // MMA8451_REG_CTRL_REG5
  // Interrupt pin 1/2 configuration register, bit == 1 => interrupt to pin 1
  // see datasheet for interrupt's description, threshold int routed to pin 1
  // comment = int2, uncoment = int1

  //dataToWrite |= 0x80;    // Auto sleep/wake
  //dataToWrite |= 0x40;    // FIFO
  dataToWrite |= 0x20; // Transient
  //dataToWrite |= 0x10;    // orientation
  //dataToWrite |= 0x08;    // Pulse
  //dataToWrite |= 0x04;    // Freefall
  //dataToWrite |= 0x01;    // data ready

  device.writeRegister8_public(MMA8451_REG_CTRL_REG5, dataToWrite);

  dataToWrite = 0;

  // MMA8451_REG_TRANSIENT_CFG
  //dataToWrite |= 0x10;  // Latch enable to capture accel values when interrupt occurs
  dataToWrite |= 0x08; // Z transient interrupt enable
  dataToWrite |= 0x04; // Y transient interrupt enable
  dataToWrite |= 0x02; // X transient interrupt enable
  //dataToWrite |= 0x01;    // High-pass filter bypass
  device.writeRegister8_public(MMA8451_REG_TRANSIENT_CFG, dataToWrite);

  Serial.print("MMA8451_REG_TRANSIENT_CFG: ");
  Serial.println(device.readRegister8(MMA8451_REG_TRANSIENT_CFG), HEX);

  dataToWrite = 0;

  // MMA8451_REG_TRANSIENT_THS
  // Transient interrupt threshold in units of .06g
  //Acceptable range is 1-127
  dataToWrite = 0x01;
  device.writeRegister8_public(MMA8451_REG_TRANSIENT_THS, dataToWrite);

  dataToWrite = 0;

  // MMA8451_REG_TRANSIENT_CT  0x20
  dataToWrite = 0; // value is 0-255 for numer of counts to debounce for, depends on ODR
  device.writeRegister8_public(MMA8451_REG_TRANSIENT_CT, dataToWrite);

  dataToWrite = 0;
}

/* Just a printing function */
void mmaCSVRead(Adafruit_MMA8451 device, String &to_fill, unsigned long int count)
{
  device.getEvent(&event);
  to_fill += count;
  to_fill += ',';
  to_fill = device.x;
  to_fill += ',';
  to_fill += device.y;
  to_fill += ',';
  to_fill += device.z;
}

/* Setup for mma use with Slide Sentinel, other use cases will be pretty similar */
void mmaSetupSlideSentinel()
{
  if (!mma.begin())
  {
    Serial.println("+-");
    while (1)
      ;
  }

  Serial.println("MMA8451 found!");

  // library configurations
  mma.setRange(MMA8451_RANGE_2_G);
  mma.setDataRate(MMA8451_DATARATE_6_25HZ);
  Serial.print("Range = ");
  Serial.print(2 << mma.getRange());
  Serial.println("G");
}

void initializePins()
{
  pinMode(RTC_WAKE_PIN, INPUT_PULLUP); // pull up resistors required for Active-Low interrupts
  pinMode(ALERT_WAKE_PIN, INPUT_PULLUP);
  pinMode(ACCEL_EN_PIN, INPUT_PULLUP); //connect to pull down switch
  pinMode(GPS_EN_PIN, OUTPUT);
  pinMode(BATTERYPIN, INPUT);
  digitalWrite(GPS_EN_PIN, LOW);
  pinMode(GPS_DISABLE_PIN, OUTPUT);
  digitalWrite(GPS_DISABLE_PIN, LOW);
}

/******************
  RTC Subroutines
******************/

void initializeRTC()
{
  // RTC Timer settings here
  if (!RTC_DS.begin())
  {
#if DEBUG == 1
    Serial.println("Couldn't find RTC");
#endif
    while (1)
      ;
  }
  // This may end up causing a problem in practice - what if RTC looses power in field? Shouldn't happen with coin cell batt backup
  if (RTC_DS.lostPower())
  {
#if DEBUG == 1
    Serial.println("RTC lost power, lets set the time!");
#endif
    // following line sets the RTC to the date & time this sketch was compiled
    RTC_DS.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  //clear any pending alarms
  clearRTCAlarm();

  // Query Time and print
  DateTime now = RTC_DS.now();

  //Set SQW pin to OFF (in my case it was set by default to 1Hz)
  //The output of the DS3231 INT pin is connected to this pin
  //It must be connected to arduino Interrupt pin for wake-up
  RTC_DS.writeSqwPinMode(DS3231_OFF);

  //Set alarm1
  clearRTCAlarm();
}

/* RTC helper function */
/* Function to query current RTC time and add the period to set next alarm cycle */
void setRTCAlarm()
{
  DateTime now = RTC_DS.now(); // Check the current time
  // Calculate new time
  MIN = (now.minute() + RTC_WAKE_PERIOD) % 60;                      // wrap-around using modulo every 60 sec
  HR = (now.hour() + ((now.minute() + RTC_WAKE_PERIOD) / 60)) % 24; // quotient of now.min+periodMin added to now.hr, wraparound every 24hrs
#if DEBUG == 1
  Serial.print("RTC Time is: ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.println(now.second(), DEC);
  Serial.print("Resetting Alarm 1 for: ");
  Serial.print(HR);
  Serial.print(":");
  Serial.println(MIN);
#endif

  //Set alarm1
  RTC_DS.setAlarm(ALM1_MATCH_HOURS, MIN, HR, 0); //set your wake-up time here
  RTC_DS.alarmInterrupt(1, true);                //code to pull microprocessor out of sleep is tied to the time -> here
}

/*
Reads an entire block of different NMEA strings, the rover reads in NMEA strings and sipatches them to CRWKG, then when ready to send rewrites these strings to ALL
Notes:  This function needs to be retooled to only grab high quality positional readings. 
        We need longer wake cycles to get steady state 10.0 RTK fixes. Waking for 30 minutes is the best option however the rover produces
        An enormous amount of strings at this wake time the majority of which are garbage. Iterating through all of the generated strings
        takes a substantial amount of time.

        This function needs to be completely redone, to verify and parse things

        BIG ISSUES: 
        1.) With this read policy 6191 PSTI,030 strings out of 6389 come in garbled example: $PSTI,030,081345.000,A,4433.9963884,N,12317$GPGGA,081346.000,4433.9963822,N,12317.6170101,W,4,12,1.4,85.154,M,-23.500,M,,0000*5A
        2.) The above observation occured after writing to both ALL and CRWKG, prior to this change the rover was reading blocks of data here, writing that data to CRWKG then when dispatching all of the data it was verifying and writing good messages to ALL.TXT
        3.) We need to migrate all logging here because it consumes too much time to iterate over CRWKG verify and then write to ALL.TXT while sending.
        4.) New Serial Reading Policy
            a.) Only look for PSTI 030 strings
            b.) Verify it 
            c.) Write it to ALL and CRWKG
*/
void readPSTI()
{
  uint16_t i = 0; //safety index
  char input;
  char buffer[GPS_BUFFER_LEN];
  memset(buffer, '\0', sizeof(buffer));
  if (PSTI(buffer, '$') && PSTI(buffer, 'P') && PSTI(buffer, 'S') && PSTI(buffer, 'T') && PSTI(buffer, 'I') && PSTI(buffer, ',') && PSTI(buffer, '0') && PSTI(buffer, '3') && PSTI(buffer, '0'))
  {
    while (Serial2.available())
    {
      input = Serial2.read();
      i++; 
      if (input == 13)
      { //read till <CR>
        append(buffer, '\n');
        //verify the NMEA checksum, if its good write the string to SD
        if (verifyChecksum(buffer))
        {
          sd_save_elem_nodelim((char *)CurrentWakeGPS, buffer);
          sd_save_elem_nodelim((char *)AllLogs, buffer);
        };
        memset(buffer, '\0', sizeof(buffer));
        return;
      }
      //this simply protects the code in case a <CR> doesnt get received, we cannot go over the buffer limit
      if(i == GPS_BUFFER_LEN){
        memset(buffer, '\0', sizeof(buffer));
        return; 
      }
      append(buffer, input);
    }
  }
  memset(buffer, '\0', sizeof(buffer));
}


bool PSTI(char buf[], char chr)
{
  char input;
  if (Serial2.available())
  {
    input = Serial2.read();
    if (input == chr)
    {
      append(buf, input);
      return true;
    }
    else
      return false;
  }
}

/* RTC helper function */
/* When exiting the sleep mode we clear the alarm */
void clearRTCAlarm()
{
  //clear any pending alarms
  RTC_DS.armAlarm(1, false);
  RTC_DS.clearAlarm(1);
  RTC_DS.alarmInterrupt(1, false);
  RTC_DS.armAlarm(2, false);
  RTC_DS.clearAlarm(2);
  RTC_DS.alarmInterrupt(2, false);
}

/* Debugging and upload helper,
   Allow uploads by waiting for serial monitor to open,
   prevent device from going to standby too soon
*/

void setupPrint()
{
  while (!Serial)
    ; //Won't start anything until serial is open, comment these lines out if powering from battery
  Serial.println("***** Interrupt Test *****");

  // Delay is required to allow the USB interface to be active during
  // sketch upload process

  Serial.println("Entering test mode in:");
  for (int i = 10; i > 0; i--)
  {
    Serial.print(i);
    Serial.println(" s");
    delay(1000);
  }
}

void accelInt(bool switch_state)
{
  detachInterrupt(digitalPinToInterrupt(ACCEL_EN_PIN));
  delay(10);
  if (switch_state)
  {
    attachInterrupt(digitalPinToInterrupt(ACCEL_EN_PIN), accel_toggle, FALLING);
#if DEBUG
    Serial.println("Switch on, interrupt falling");
#endif
    accelEn = true;
    interruptReset();
  }
  else
  {
    attachInterrupt(digitalPinToInterrupt(ACCEL_EN_PIN), accel_toggle, RISING);
#if DEBUG
    Serial.println("Switch off, interrupt rising");
#endif
    accelEn = false;
  }
}

void interruptReset()
{
  detachInterrupt(digitalPinToInterrupt(ALERT_WAKE_PIN));
  detachInterrupt(digitalPinToInterrupt(RTC_WAKE_PIN));
  EIC->INTFLAG.reg = 0x01ff; // clear interrupt flags pending
  delay(10);                 // GPS switch will trigger accel interrupt if no delay
  attachInterrupt(digitalPinToInterrupt(ALERT_WAKE_PIN), wakeUp_alert, LOW);
  attachInterrupt(digitalPinToInterrupt(RTC_WAKE_PIN), wakeUp_RTC, LOW);
}

void timerFlagCheck()
{
  if (TimerFlag)
  { //triggered with every wake-up interrupt

    //**************** IMPORTANT, DO NOT EDIT INTERRUPT ISRs ****************
    //clear interrupt registers, attach interrupts EVERY TIME THE INTERRUPT IS CALLED
    interruptReset();

    timer = millis();
    TimerFlag = false;
  }
}

void RTCFlagCheck()
{
  if (RTCFlag)
  {
    clearRTCAlarm();
    Serial.println("Processor wake from RTC");
    DateTime now = RTC_DS.now();
    uint8_t mo = now.month();
    uint8_t d = now.day();
    uint8_t h = now.hour();
    uint8_t mm = now.minute();

    RTC_monthString = String(mo, DEC);
    RTC_dayString = String(d, DEC);
    RTC_hrString = String(h, DEC);
    RTC_minString = String(mm, DEC);
    RTC_timeString = RTC_hrString + ":" + RTC_minString + "_" + RTC_monthString + "/" + RTC_dayString;

#if DEBUG == 1
    Serial.println(RTC_timeString);
#endif
    RTCFlag = false;
    alertFlag = false;
    accelFlag = false;
    awakeFor = STANDARD_WAKE;
  }
}

void alertFlagCheck()
{
  if (alertFlag && accelEn)
  {
    Serial.println("Processor wake from accelerometer");
    uint8_t dataRead = mma.readRegister8(MMA8451_REG_TRANSIENT_SRC); //clear the interrupt register
    mmaPrintIntSRC(dataRead);
    if (!stateSent)
    {
      stateSent = true;
      sendState(mma, true);
    }
    accelFlag = false; // reset flag, clear the interrupt
    interruptReset();
    alertFlag = false;
    alert_timer = millis();
    awakeFor = ALERT_WAKE;
  }
  if (alertFlag)
  {
    Serial.println("Accelerometer interrupt detected");
    uint8_t dataRead = mma.readRegister8(MMA8451_REG_TRANSIENT_SRC); //clear the interrupt register
    mmaPrintIntSRC(dataRead);

    accelFlag = false; // reset flag, clear the interrupt
    interruptReset();
    alertFlag = false;
    RTCFlag = false;
  }
}

void enCheck()
{
  if (accelEnFlag)
  {
    detachInterrupt(digitalPinToInterrupt(ACCEL_EN_PIN));
    accelInt(digitalRead(ACCEL_EN_PIN));
    Serial.println("Enable switch interrupt activated");
    accelEnFlag = false;
  }
}

void resetFlags()
{
  TimerFlag = false;
  alertFlag = false;
  accelFlag = false;
  TakeSampleFlag = false;
}

void sendState(Adafruit_MMA8451 device, bool accel)
{
  char msg[150];
  memset(msg, '\0', sizeof(msg));
  String reading;

  device.getEvent(&event);
  if (accel)
    strcat(msg, "/Accel");
  else
    strcat(msg, "/State");
  strcat(msg, ",");
  reading = "0";
  strcat(msg, reading.c_str());
  strcat(msg, ",");
  reading = device.x;
  strcat(msg, reading.c_str()); // accel x
  strcat(msg, ",");
  reading = device.y;
  strcat(msg, reading.c_str()); // accel y
  strcat(msg, ",");
  reading = device.z;
  strcat(msg, reading.c_str()); // accel z
  getVoltage(msg);

  //add a checksum to the message
  strcat(msg, ",");
  addChecksum(msg);
  sendMessage(msg, Serial3);
}

void getVoltage(char msg[])
{
  int raw;
  float voltage;
  char buf[10];
  memset(buf, '\0', sizeof(buf)); //clear the buffer
  float divider_const = 1.12;
  raw = analogRead(BATTERYPIN);
  raw = map(raw, 0, 4095, 0, 3700);
  voltage = ((float)raw / 1000);
  snprintf(buf, sizeof(buf), "%f", voltage);
  strcat(msg, ",");
  strcat(msg, buf); // Battery voltage
}

void tryStandby()
{
  if (millis() - timer > 1000 * awakeFor)
  { //delay, then sleep
    processGPS((char *)CurrentWakeGPS, NODE_NUM, Serial3);
    Serial.println("GPS data written out");
    sendState(mma, false);
    delay(10000);
    gps_off();
    delay(100);
    //reset all flags
    resetFlags();

    // process strings that are in the read cycle folder

    // Fill and send a state packet
    setRTCAlarm(); //reset alarm to go off one wake period from sleeping

    Serial.println("STANDBY");
    Serial.end();
    USBDevice.detach();
    while (Serial2.available())
    {
      readPSTI();
    } // load the rest of the gps strings that are ready

    delay(20);        // delay is so that the gps switch doesn't trigger accelerometer wake
    interruptReset(); //clear interrupt registers, attach interrupts
    LowPower.standby();
    // ========================================================================================
    // ====================== Sleep here and wait for int (accel or RTC) ======================
    USBDevice.attach();
    gps_on();
    stateSent = false;
#if DEBUG == 1
    //digitalWrite(LED_BUILTIN, HIGH);
    delay(5000); // give user 5s to close and reopen serial monitor!
#endif
    clearRTCAlarm(); //prevent double trigger of alarm interrupt
    Serial.begin(115200);
    Serial.println("WAKE");
  }
}

void Serial2Setup()
{
  Serial2.begin(BAUD);

  // Assign pins 11,12 SERCOM functionality, internal function
  pinPeripheral(SERIAL2_TX, PIO_SERCOM); //Private functions for serial communication
  pinPeripheral(SERIAL2_RX, PIO_SERCOM);
}

void Serial3Setup()
{
  // Assign pins 10 & 13 SERCOM functionality, internal function
  Serial3.begin(115200);
  pinPeripheral(SERIAL3_TX, PIO_SERCOM); //Private functions for serial communication
  pinPeripheral(SERIAL3_RX, PIO_SERCOM_ALT);
}

void mmaPrintIntSRC(uint8_t dataRead)
{
  if (dataRead & 0x40)
    Serial.println("Event Active");
  if (dataRead & 0x20)
  {
    Serial.println("\tZ event");
    if (dataRead & 0x10)
      Serial.println("\t\tZ Negative g");
    else
      Serial.println("\t\tZ Positive g");
  }
  if (dataRead & 0x08)
  {
    Serial.println("\tY event");
    if (dataRead & 0x04)
      Serial.println("\t\tY Negative g");
    else
      Serial.println("\t\tY Positive g");
  }
  if (dataRead & 0x02)
  {
    Serial.println("\tX event");
    if (dataRead & 0x01)
      Serial.println("\t\tX Negative g");
    else
      Serial.println("\t\tX Positive g");
  }
}

// Turn the GPS module on (consumes ~150 mA)
void gps_on()
{
  digitalWrite(GPS_EN_PIN, HIGH);
  delay(10);
  digitalWrite(GPS_EN_PIN, LOW);
}

// Turn off the gps module, save power during sleep
// Functions affect physical latch on feather relay
void gps_off()
{
  digitalWrite(GPS_DISABLE_PIN, HIGH);
  delay(10);
  digitalWrite(GPS_DISABLE_PIN, LOW);
}

/*
void readSerial()
{
  // read data if data is available and buffer isn't full
  if (Serial2.available())
  {
    if (dataIn.len < GPS_BUFFER_LEN)
    {
      dataIn.data[dataIn.len] = Serial2.read();
      dataIn.len += 1;
    }
    // write date from buffer to SD if buffer is full
    else
    {
      dataIn.data[GPS_BUFFER_LEN] = 0;
      sd_save_elem_nodelim((char *)CurrentWakeGPS, dataIn.data);
      sd_save_elem_nodelim((char *)AllLogs, dataIn.data);
      memset(dataIn.data, '\0', GPS_BUFFER_LEN + 1);
      dataIn.len = 0;
      dataIn.data[dataIn.len] = Serial2.read();
      dataIn.len += 1;
    }
  }
  // write data to SD if serial is not available and buffer has data
  else if (dataIn.len)
  {
    dataIn.data[dataIn.len] = 0; // ensure data is c string
    sd_save_elem_nodelim((char *)CurrentWakeGPS, dataIn.data);
    sd_save_elem_nodelim((char *)AllLogs, dataIn.data);
    memset(dataIn.data, '\0', GPS_BUFFER_LEN + 1);
    dataIn.len = 0;
  }
}
*/