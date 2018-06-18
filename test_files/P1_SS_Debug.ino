/*******************************************************************************************
   SlideSentinel: Phase 1 Sensor code
   Author: Marissa Kwon
   5/21/18

   info on devices can be found here:
   LIS3DH: https://learn.sparkfun.com/tutorials/lis3dh-hookup-guide
      SOURCE CODE FROM EXAMPLE: "IntUsage.ino"
      Marshall Taylor @ SparkFun Electronics
      Nov 16, 2016
      https://github.com/sparkfun/LIS3DH_Breakout
      https://github.com/sparkfun/SparkFun_LIS3DH_Arduino_Library

      Description:
      Interrupt support for the LIS3DH is extremely flexible, so configuration must be left
      to the user.  This sketch demonstrates how to make a interrupt configuration function.

      Use configIntterupts() as a template, then comment/uncomment desired options.
      See ST docs for information
      Doc ID 18198 (AN3308): LIS3DHpplication information
      Doc ID 17530: LIS3DH datasheet
**********************************************************************************************/

// Add necessary libraries here
#include <SPI.h>
#include <RH_RF95.h> // Important Example code found at https://learn.adafruit.com/adafruit-rfm69hcw-and-rfm96-rfm95-rfm98-lora-packet-padio-breakouts/rfm9x-test
#include <RHReliableDatagram.h>
#include "LowPower.h" // from sparkfun low power library found here https://github.com/rocketscream/Low-Power
#include "RTClibExtended.h"// from sparkfun low power library found here https://github.com/FabioCuomo/FabioCuomo-DS3231/
#include "SparkFunLIS3DH.h"// from sparkfun accelerometer library found here https://github.com/sparkfun/SparkFun_LIS3DH_Arduino_Library
#include "Wire.h"
#include <SD.h>

// Define macro constants
#define DEBUG 1  //test to allow print to serial monitor
#define RTC_MODE 1  //endble RTC 
#define SD_WRITE 0  //enable SD card logging (too much for 32u4 processor memory)

// Define constants/pins
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7
#define SERVER_ADDRESS 2

#define RF95_FREQ 915.0  // Change to 434.0 or other frequency, must match RX's freq!
#define SAMPLE_SIZE 5  // how many samples to take for average value

// Create instances of sensors
LIS3DH myIMU(I2C_MODE, 0x19); //Default constructor is I2C, addr 0x19.
RH_RF95 rf95(RFM95_CS, RFM95_INT);  //instance of LoRA
RHReliableDatagram manager(rf95, SERVER_ADDRESS);  //LoRa message verification

RTC_DS3231 RTC; //instance of DS3231 RTC

#if DEBUG == 1 && SD_WRITE == 1
File myFile;  //file for SD card
#endif

// Declare global variables
int transmitBufLen; // length of transmit buffer
const int ID = 100; // id unique to device
String IDString, NEMA_string, X_string, Y_string, Z_string, RTC_monthString, RTC_dayString, RTC_hrString, RTC_minString, RTC_timeString = "", stringTransmit = "";

// Declare RTC/Accelerometer specific variables
volatile bool TakeSampleFlag = false; // Flag is set with external Pin A0 Interrupt by RTC
volatile bool AlertFlag = false; // Flag if an alert is triggered and we need to speed up some processes (Phase 2)
volatile int HR = 8; // Hr of the day we want alarm to go off
volatile int MIN = 0; // Min of each hour we want alarm to go off
volatile int WakePeriodMin = 1;  // Period of time to take sample in Min, reset alarm based on this period (Bo - 5 min)
const byte wakeUpPin = 11;  // attach to SQW pin on RTC
const byte alertPin = 0; //13; // attach to int1 on accelerometer


/**********************************************************************************************
   setup()
   States: setup
   Preconditions: custom settings saved as global variables; takes place after a reset or power on
   Postcondition: alarmFlag set low; alertFlag set low
   Description:
        - begin serial
        - assign pinmodes/write to pins
        - assign IMU settings (LIS3DH), init, and configure IMU
        - (add GPS set up)
        - initialize DS3231 (if RTC mode on)
        - init LoRa/reliable datagram and set freq
**********************************************************************************************/

void setup() {
  Serial.begin(9600);
  delay(2000);
  pinMode(wakeUpPin, INPUT_PULLUP);  // set pin for alarm interrupt
  pinMode(alertPin, INPUT_PULLUP);  //set pin for accelerometer interrupt
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  delay(1000); //relax...

  /*manually reset LoRa*/
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  /*check LoRa device and set frequency*/
  while (!manager.init()) {
#if DEBUG == 1
    Serial.println("LoRa manager init failed"); //if print wiring may be wrong
#endif
    while (1);
  }
#if DEBUG == 1
  Serial.println("LoRa radio init OK!");
#endif
  // checks if frequency is initialized
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
#if DEBUG == 1
    Serial.println("setFrequency failed");
#endif
    while (1);
  }
#if DEBUG == 1
  Serial.print("Set Freq to: ");
  Serial.println(RF95_FREQ);
#endif
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips / symbol, CRC on
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);

  /*initialize accelerometer and configure settings*/
#if DEBUG == 1
  Serial.println("Processor came out of reset.\n");
#endif
  //Accel sample rate and range effect interrupt time and threshold values!!!
  myIMU.settings.accelSampleRate = 100;  //Hz.  Can be: 0,1,10,25,50,100,200,400,1600,5000 Hz
  myIMU.settings.accelRange = 2;      //Max G force readable.  Can be: 2, 4, 8, 16
  myIMU.settings.adcEnabled = 0;
  myIMU.settings.tempEnabled = 0;
  myIMU.settings.xAccelEnabled = 1;
  myIMU.settings.yAccelEnabled = 1;
  myIMU.settings.zAccelEnabled = 1;
  //Call .begin() to configure the IMU
  myIMU.begin();

  configInterrupts();

  /*initialize RTC*/
#if RTC_MODE == 1
  //RTC stuff init//
  InitalizeRTC();
#if DEBUG == 1
  Serial.print("Alarm set to go off every "); Serial.print(WakePeriodMin); Serial.println("min from program time");
#endif
  delay(1000);
#endif

#if DEBUG == 1 && RTC == 1
  Serial.print("Initializing SD card...");
  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
#endif

}

void loop() {

  //local variables
  float acc_x = 0, acc_y = 0, acc_z = 0;
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  /* prep for sleep - don't go to sleep if RTC not set*/
#if RTC_MODE == 1 //set up RTC interrupts
  // Enable SQW pin interrupt
  // enable interrupt for PCINT7...
  pciSetup(11);
  delay(1000);

  // Enter into Low Power mode here[RTC]:
  // Enter power down state with ADC and BOD module disabled.
  // Wake up when wake up pin is low.
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  // <----  Wait in sleep here until pin interrupt
  // On Wakeup, proceed from here:
  PCICR = 0x00;         // Disable PCINT interrupt
  clearAlarmFunction(); // Clear RTC Alarm
#else
  /* no sleep mode and reset the TakeSampleFlag to true*/
  delay(1000); // period in DEBUG mode to wait between samples
  TakeSampleFlag = 1;
#endif

  /*while awake do the following*/
  if (TakeSampleFlag)
  {
    /* wake up devices here if previously in low-power mode */
#if RTC_MODE == 1
    //write power-on functions here

    // get RTC timestamp and ID string
    DateTime now = RTC.now();
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
    Serial.print(RTC_timeString);
#endif

#endif

    IDString = String(ID, DEC);  //changes ID int into string

    //get accelerometer data - all parameters
    if (AlertFlag) {
      //can do something special here if woken up by accelerometer
    }
    else {
      //take an average acceleration
      for (int i = 0; i < SAMPLE_SIZE; i++) {
        acc_x += myIMU.readFloatAccelX();
        acc_y += myIMU.readFloatAccelY();
        acc_z += myIMU.readFloatAccelZ();
      }
      acc_x = acc_x / SAMPLE_SIZE;
      acc_y = acc_y / SAMPLE_SIZE;
      acc_z = acc_z / SAMPLE_SIZE;
    } // end of typical accelerometer measurements

    // assign data values to string
    X_string = String(acc_x, 4);
    Y_string = String(acc_y, 4);
    Z_string = String(acc_z, 4);

#if DEBUG == 1
    Serial.print("\nAccelerometer:\n");
    Serial.print(" X = ");
    Serial.println(acc_x, 4);
    Serial.print(" Y = " );
    Serial.println(acc_y, 4);
    Serial.print(" Z = ");
    Serial.println(acc_z, 4);

    ///*
#if SD_WRITE == 1
    myFile = SD.open("datalog.txt", FILE_WRITE);
    if (myFile) {
      myFile.print(RTC_timeString);
      myFile.print(': ');
      myFile.print(acc_x);
      myFile.print(',');
      myFile.print(acc_y);
      myFile.print(',');
      myFile.print(acc_z);
      myFile.println();
      // close the file:
      myFile.close();
      Serial.println("done.");
    } else {
      // if the file didn't open, print an error:
      Serial.println("error opening test.txt");
    }
#endif
    //*/
#endif

    //get NEMA data - add code here

    /* power down devices before transmit */
    // write code for powering on devices here

    /* begin transmit section here */
    //concatenate message
    stringTransmit = String(IDString + "," + RTC_timeString + "," + X_string + "," + Y_string + "," + Z_string + "\0"); //our concatenated string for transmit
    Serial.println(stringTransmit);
/*
    //calculate the length of the transmitBufLen (needed for LoRa)
    transmitBufLen = 1 + (char)stringTransmit.length();  //+1 to account for the end character

    // instantiate a transmit buffer of x len based on len of concat string above
    char transmitBuf[transmitBufLen];

    // converts long string of data into a character array to be transmitted
    stringTransmit.toCharArray(transmitBuf, transmitBufLen);

#if DEBUG == 1
    Serial.println(stringTransmit);
    Serial.println(transmitBufLen);
    Serial.println("Reading...");
    Serial.println(transmitBuf); // print to confirm transmit buff matches above string
    Serial.println("Sending to rf95_server (base station)");
#endif

    // begin sending to data to receiver and listen using ReliableDatagram
    if (manager.sendtoWait((uint8_t*)transmitBuf, transmitBufLen, SERVER_ADDRESS))
      Serial.println("Ok");
    else
      Serial.println("Send Failure");

    // receive message later for GPS position correction (Phase 2)
    if (rf95.waitAvailableTimeout(500))
    {
      // Should be a reply message for us now
      if (rf95.recv(buf, &len))
      {
#if DEBUG == 1
        Serial.println("Got reply: ");
        Serial.println("\nData received:");
        Serial.println((char*)buf);
        Serial.print("RSSI: ");
        Serial.println(rf95.lastRssi(), DEC); // prints RSSI as decimal value
        Serial.println();
#endif
      }
      else //happens when there is a receiver but bad message
      {
        Serial.println("Receive failed");
      }
    }
    else //happens when there is no receiver on the same freq to listen to
    {
      Serial.println("No reply, is there a listener around?");
    }
*/
    // Reset alarm1 for next period
    setAlarmFunction();

    delay(75);  // delay so serial stuff has time to print out all the way

    TakeSampleFlag = false; // Clear Alarm Flag
    AlertFlag = false; //Clear Accelerometer Interrupt Flag

  } // end of TakeSampleFlag tasks
}


/**********************************************************************************************
   Helper Functions - configInterrupts, InitializeRTC, setAlarmFunction, clearAlarmFunction
**********************************************************************************************/

/******************
  Accelerometer
******************/
/* More info about listed data registers on the LISH3DH datasheet: */
/* http://www.st.com/content/ccc/resource/technical/document/datasheet/3c/ae/50/85/d6/b1/46/fe/CD00274221.pdf/files/CD00274221.pdf/jcr:content/translations/en.CD00274221.pdf */

void configInterrupts()
{
  uint8_t dataToWrite = 0;

  //LIS3DH_INT1_CFG
  dataToWrite |= 0x80;//AOI, 0 = OR 1 = AND
  dataToWrite |= 0x40;//6D, 0 = interrupt source, 1 = 6 direction source
  //Set these to enable individual axes of generation source (or direction)
  // -- high and low are used generically
  dataToWrite |= 0x20;//Z high
  //dataToWrite |= 0x10;//Z low
  dataToWrite |= 0x08;//Y high
  //dataToWrite |= 0x04;//Y low
  dataToWrite |= 0x02;//X high
  //dataToWrite |= 0x01;//X low
  myIMU.writeRegister(LIS3DH_INT1_CFG, dataToWrite);

  //LIS3DH_INT1_THS
  dataToWrite = 0;
  //Provide 7 bit value, 0x7F always equals max range by accelRange setting
  dataToWrite |= 0x10; // 1/8 range
  myIMU.writeRegister(LIS3DH_INT1_THS, dataToWrite);

  //LIS3DH_INT1_DURATION
  dataToWrite = 0;
  //minimum duration of the interrupt
  //LSB equals 1/(sample rate)
  dataToWrite |= 0x01; // 1 * 1/50 s = 20ms
  myIMU.writeRegister(LIS3DH_INT1_DURATION, dataToWrite);

  //LIS3DH_CLICK_CFG
  dataToWrite = 1;
  //Set these to enable individual axes of generation source (or direction)
  // -- set = 1 to enable
  //dataToWrite |= 0x20;//Z double-click
  dataToWrite |= 0x10;//Z click
  //dataToWrite |= 0x08;//Y double-click
  dataToWrite |= 0x04;//Y click
  //dataToWrite |= 0x02;//X double-click
  dataToWrite |= 0x01;//X click
  myIMU.writeRegister(LIS3DH_CLICK_CFG, dataToWrite);

  //LIS3DH_CLICK_SRC
  dataToWrite = 1;
  //Set these to enable click behaviors (also read to check status)
  // -- set = 1 to enable
  //dataToWrite |= 0x20;//Enable double clicks
  dataToWrite |= 0x04;//Enable single clicks
  //dataToWrite |= 0x08;//sine (0 is positive, 1 is negative)
  dataToWrite |= 0x04;//Z click detect enabled
  dataToWrite |= 0x02;//Y click detect enabled
  dataToWrite |= 0x01;//X click detect enabled
  myIMU.writeRegister(LIS3DH_CLICK_SRC, dataToWrite);

  //LIS3DH_CLICK_THS
  dataToWrite = 0;
  //This sets the threshold where the click detection process is activated.
  //Provide 7 bit value, 0x7F always equals max range by accelRange setting
  dataToWrite |= 0x0A; // ~1/16 range
  myIMU.writeRegister(LIS3DH_CLICK_THS, dataToWrite);

  //LIS3DH_TIME_LIMIT
  dataToWrite = 0;
  //Time acceleration has to fall below threshold for a valid click.
  //LSB equals 1/(sample rate)
  dataToWrite |= 0x08; // 8 * 1/50 s = 160ms
  myIMU.writeRegister(LIS3DH_TIME_LIMIT, dataToWrite);

  //LIS3DH_TIME_LATENCY
  dataToWrite = 0;
  //hold-off time before allowing detection after click event
  //LSB equals 1/(sample rate)
  dataToWrite |= 0x08; // 4 * 1/50 s = 160ms
  myIMU.writeRegister(LIS3DH_TIME_LATENCY, dataToWrite);

  //LIS3DH_TIME_WINDOW
  dataToWrite = 0;
  //hold-off time before allowing detection after click event
  //LSB equals 1/(sample rate)
  dataToWrite |= 0x10; // 16 * 1/50 s = 320ms
  myIMU.writeRegister(LIS3DH_TIME_WINDOW, dataToWrite);

  //LIS3DH_CTRL_REG5
  //Int1 latch interrupt and 4D on  int1 (preserve fifo en)
  myIMU.readRegister(&dataToWrite, LIS3DH_CTRL_REG5);
  dataToWrite &= 0xF3; //Clear bits of interest
  dataToWrite |= 0x08; //Latch interrupt (Cleared by reading int1_src)
  //dataToWrite |= 0x04; //Pipe 4D detection from 6D recognition to int1?
  myIMU.writeRegister(LIS3DH_CTRL_REG5, dataToWrite);

  //LIS3DH_CTRL_REG3
  //Choose source for pin 1
  dataToWrite = 0;
  //dataToWrite |= 0x80; //Click detect on pin 1
  dataToWrite |= 0x40; //AOI1 event (Generator 1 interrupt on pin 1)
  dataToWrite |= 0x20; //AOI2 event ()
  //dataToWrite |= 0x10; //Data ready
  //dataToWrite |= 0x04; //FIFO watermark
  //dataToWrite |= 0x02; //FIFO overrun
  myIMU.writeRegister(LIS3DH_CTRL_REG3, dataToWrite);

  //LIS3DH_CTRL_REG6
  //Choose source for pin 2 and both pin output inversion state
  dataToWrite = 0;
  dataToWrite |= 0x80; //Click int on pin 2
  dataToWrite |= 0x40; //Generator 1 interrupt on pin 2
  //dataToWrite |= 0x10; //boot status on pin 2
  //dataToWrite |= 0x02; //invert both outputs
  myIMU.writeRegister(LIS3DH_CTRL_REG6, dataToWrite);
}


/******************
  RTC Subroutines
******************/

void InitalizeRTC()
{
  // RTC Timer settings here
  if (! RTC.begin()) {
#if DEBUG == 1
    Serial.println("Couldn't find RTC");
#endif
    while (1);
  }
  // This may end up causing a problem in practice - what if RTC looses power in field? Shouldn't happen with coin cell batt backup
  if (RTC.lostPower()) {
#if DEBUG == 1
    Serial.println("RTC lost power, lets set the time!");
#endif
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  //clear any pending alarms
  clearAlarmFunction();

  // Query Time and print
  DateTime now = RTC.now();
#if DEBUG == 1
  Serial.print("RTC Time is: ");
  Serial.print(now.hour(), DEC); Serial.print(':'); Serial.print(now.minute(), DEC); Serial.print(':'); Serial.print(now.second(), DEC); Serial.println();
#endif
  //Set SQW pin to OFF (in my case it was set by default to 1Hz)
  //The output of the DS3231 INT pin is connected to this pin
  //It must be connected to arduino Interrupt pin for wake-up
  RTC.writeSqwPinMode(DS3231_OFF);

  //Set alarm1
  setAlarmFunction();
}


/* RTC helper function */
/* Function to query current RTC time and add the period to set next alarm cycle */
void setAlarmFunction()
{
  DateTime now = RTC.now(); // Check the current time
  // Calculate new time
  MIN = (now.minute() + WakePeriodMin) % 60; // wrap-around using modulo every 60 sec
  HR  = (now.hour() + ((now.minute() + WakePeriodMin) / 60)) % 24; // quotient of now.min+periodMin added to now.hr, wraparound every 24hrs
#if DEBUG == 1
  Serial.print("Resetting Alarm 1 for: "); Serial.print(HR); Serial.print(":"); Serial.println(MIN);
#endif

  //Set alarm1
  RTC.setAlarm(ALM1_MATCH_HOURS, MIN, HR, 0);   //set your wake-up time here
  RTC.alarmInterrupt(1, true);

}


/* RTC helper function */
/* When exiting the sleep mode we clear the alarm */
void clearAlarmFunction()
{
  //clear any pending alarms
  RTC.armAlarm(1, false);
  RTC.clearAlarm(1);
  RTC.alarmInterrupt(1, false);
  RTC.armAlarm(2, false);
  RTC.clearAlarm(2);
  RTC.alarmInterrupt(2, false);
}

/************************************************************************************************
   ISR pin assignments
************************************************************************************************/
/* Wakeup in SQW ISR */
/* Function to init PCI interrupt pin */
/* Pulled from: https://playground.arduino.cc/Main/PinChangeInterrupt */

void pciSetup(byte pin)
{
  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
  PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
  PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

/* Use one Routine to handle each group */
// macro (global interrupt)

ISR (PCINT0_vect) // handle pin change interrupt for D8 to D13 here
{
  if (digitalRead(11) == LOW)
    TakeSampleFlag = true;
}
// triggers wake when accelerometer interrupt








