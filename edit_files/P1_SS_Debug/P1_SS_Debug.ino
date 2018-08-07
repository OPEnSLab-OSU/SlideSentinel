<<<<<<< HEAD:edit_files/P1_SS_Debug/P1_SS_Debug.ino
// For Feather M0 needs work on the RTC and accelerometer interrupt routines
=======
//# For Feather M0 needs work on the RTC and accelerometer interrupt routines
>>>>>>> master:edit_files/P1_SS_Debug/P1_SS_Debug.ino
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
      to the user.  This sketch demonstrates how to make a interrupt configuration function

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

// Define macro constants
#define DEBUG 1  //test to allow print to serial monitor
#define RTC_MODE 1 //enable RTC 
#define SD_WRITE 0  //enable SD card logging (too much for 32u4 processor memory)

// ======= BOARD SPECIFIC SETTINGS ======

//NOTE: Must include the following line in the RTClibExtended.h file to use with M0:
//#define _BV(bit) (1 << (bit))
#include <RTClibExtended.h>

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>

#ifdef __SAMD21G18A__
#define is_M0
#endif

#ifdef __AVR_ATmega32U4__
#define is_32U4
#pragma message("Warning: 32u4 can only interface with one Decagon device on pin 10")
#endif

//===== LoRa Initializations =====

#ifdef is_M0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#endif

#ifdef is_32U4
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7
#endif

#define SERVER_ADDRESS 2

//battery voltage read pin
#ifdef is_M0
#define VBATPIN A7
#endif

#ifdef is_32U4
#define VBATPIN A9
#endif

float measuredvbat;

// ======== General Settings ==========

#define RF95_FREQ 915.0  // Change to 434.0 or other frequency, must match RX's freq!
#define SAMPLE_SIZE 5  // how many samples to take for average value

// Create instances of sensors
LIS3DH myIMU(I2C_MODE, 0x19); //Default accel constructor is I2C, addr 0x19.
RH_RF95 rf95(RFM95_CS, RFM95_INT);  //instance of LoRA
RHReliableDatagram manager(rf95, SERVER_ADDRESS);  //LoRa message verification

RTC_DS3231 RTC_DS; //instance of DS3231 RTC

// ========= Declare Global Variables ==============

int transmitBufLen; // length of transmit buffer
const int ID = 100; // id unique to device
<<<<<<< HEAD:edit_files/P1_SS_Debug/P1_SS_Debug.ino
String IDString, NEMA_string, X_string, Y_string, Z_string, RTC_monthString, RTC_dayString, RTC_hrString, RTC_minString, RTC_secString, RTC_timeString = "", stringTransmit = "";
=======
String IDString, NMEA_string, X_string, Y_string, Z_string, RTC_monthString, RTC_dayString, RTC_hrString, RTC_minString, RTC_timeString = "", stringTransmit = "";
>>>>>>> master:edit_files/P1_SS_Debug/P1_SS_Debug.ino

// Declare RTC/Accelerometer specific variables
volatile bool TakeSampleFlag = false; // Flag is set with external Pin A0 Interrupt by RTC
volatile bool AlertFlag = false; // Flag if an alert is triggered and we need to speed up some processes (Phase 2)
volatile int HR = 8; // Hr of the day we want alarm to go off
volatile int MIN = 0; // Min of each hour we want alarm to go off
volatile int WakePeriodMin = 1;  // Period of time to take sample in Min, reset alarm based on this period (Bo - 5 min)
<<<<<<< HEAD:edit_files/P1_SS_Debug/P1_SS_Debug.ino
volatile int count = 10; //number of seconds to wait before running loop()
const byte wakeUpPin = 12;  // attach to SQW pin on RTC
const byte alertPin = 6;  // attach to int1 on accelerometer
=======
const byte wakeUpPin = 11;  // attach to SQW pin on RTC
const byte alertPin = 12;  // attach to int1 on accelerometer
volatile int count = 5; //number of seconds to wait before interrupt configurations
>>>>>>> master:edit_files/P1_SS_Debug/P1_SS_Debug.ino
uint8_t dataRead; // for acceleromter interrupt register

/**********************************************************************************************
   wakeUp()
   Description: function that takes place after device wakes up
   See: Interrupt Service Routine (ISR)
   wakeUp_alert
   - sets SampleFlag and AlertFlag TRUE;
   wakeUp_RTC
   - sets SampleFlag TRUE upon typical wakeup
**********************************************************************************************/
void wakeUp_alert()
{
  AlertFlag = true;
  TakeSampleFlag = true;
  myIMU.readRegister(&dataRead, LIS3DH_INT1_SRC);//cleared by reading
<<<<<<< HEAD:edit_files/P1_SS_Debug/P1_SS_Debug.ino
  detachInterrupt(digitalPinToInterrupt(wakeUpPin));
=======
#if DEBUG
  Serial.println("Alert recognized");
  delay(1000); //time to print
#endif
>>>>>>> master:edit_files/P1_SS_Debug/P1_SS_Debug.ino
}

void wakeUp_RTC()
{
#if DEBUG
  Serial.println("RTC wake recognized");
  delay(1000); //time to print
#endif
  TakeSampleFlag = true;
  detachInterrupt(digitalPinToInterrupt(wakeUpPin));
  }
/**********************************************************************************************
   setup()
   States: setup
   Preconditions: custom settings saved as global variables; takes place after a reset or power on
   Postcondition: alarmFlag set low; AlertFlag set low
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
#if DEBUG
<<<<<<< HEAD:edit_files/P1_SS_Debug/P1_SS_Debug.ino
  setupPrint(); //give the device time to wake up and upload sketch if necessary
=======
  //delay code used from rocketscream lowpower library M0 standby example
  while(!Serial);
    Serial.println("***** Interrupt Test *****");
    
    // ***** IMPORTANT *****
    // Delay is required to allow the USB interface to be active during
    // sketch upload process
    Serial.println("Entering standby mode in:");
    for (count; count > 0; count--)
    {
      Serial.print(count);  
      Serial.println(" s");
      delay(1000);
    }
>>>>>>> master:edit_files/P1_SS_Debug/P1_SS_Debug.ino
#ifdef is_M0
  Serial.println("Is M0");

#endif

#ifdef is_32u4
  Serial.println("Is 32u4");
#endif
#endif

  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  /*manually reset LoRa*/
  digitalWrite(RFM95_RST, LOW);   delay(10);
  digitalWrite(RFM95_RST, HIGH);  delay(10);

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

  pinMode(wakeUpPin, INPUT_PULLUP);  // set pin for alarm interrupt
  pinMode(alertPin, INPUT_PULLUP);  // enable pull up resistor for accelerometer interrupt

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

  myIMU.readRegister(&dataRead, LIS3DH_INT1_SRC);//cleared by reading

  /*initialize RTC*/
#if RTC_MODE == 1
  //RTC stuff init//
  InitializeRTC();
#if DEBUG == 1
  Serial.print("Alarm set to go off every "); Serial.print(WakePeriodMin); Serial.println(" min from program time");
#endif
  delay(1000);
#endif
}

void loop() {

  //local variables
  float acc_x = 0, acc_y = 0, acc_z = 0;
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  /* prep for sleep - don't go to sleep if RTC not set*/
#if RTC_MODE == 1 //set up RTC interrupts
  // Enter into Low Power mode here[RTC]:
  // Enter power down state with ADC and BOD module disabled.
  // Enable SQW pin interrupt

#ifdef is_32u4
  //CURRENTLY ONLY SET UP FOR RTC INTS
  //needed to assign interrupts to pins
  // enable interrupt for PCINT7...
  pciSetup(11);
  pciSetup(12);
  delay(1000);

  // Wake up when wakeUp pin is low or on rising edge of alertPin .
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  // <----  Wait in sleep here until pin interrupt
  // On Wakeup, proceed from here:
  PCICR = 0x00;         // Disable PCINT interrupt
  clearAlarmFunction(); // Clear RTC Alarm
#endif //is_32u4

#ifdef is_M0
//  //SETUP FOR RTC AND ACCELEROMETER INTS
//  attachInterrupt(digitalPinToInterrupt(alertPin), wakeUp_alert, LOW);
//  attachInterrupt(digitalPinToInterrupt(wakeUpPin), wakeUp_RTC, LOW);
#if DEBUG
  Serial.println("STANDBY");
#endif
  Serial.end(); //end ALL sercoms before the device goes to sleep
  USBDevice.detach();
 
  delay(100);
<<<<<<< HEAD:edit_files/P1_SS_Debug/P1_SS_Debug.ino
  digitalWrite(LED_BUILTIN, LOW); //LED off when processor is asleep

  //SETUP FOR RTC AND ACCELEROMETER INTS

  
  attachInterrupt(digitalPinToInterrupt(alertPin), wakeUp_alert, LOW);
  attachInterrupt(digitalPinToInterrupt(wakeUpPin), wakeUp_RTC, LOW);
  
  LowPower.standby();
=======
  LowPower.idle(IDLE_2);
>>>>>>> master:edit_files/P1_SS_Debug/P1_SS_Debug.ino
  // <----  Wait in sleep here until pin interrupt
  // On Wakeup, proceed from here:
  
  // Disable external pin interrupt on wake up pin.
 
  detachInterrupt(digitalPinToInterrupt(alertPin));
  detachInterrupt(digitalPinToInterrupt(wakeUpPin));

  //reconfigure serial
  USBDevice.attach();
  Serial.begin(9600);
  digitalWrite(LED_BUILTIN, HIGH); //LED active with processor wake
  
  clearAlarmFunction();
  
#if DEBUG
  //5 seconds for user to restart serial monitor while debugging
  //this is required due to attach() and detach() of USBDevice
  delay(5000); 
  Serial.println("AWAKE!");
#endif
  
#endif //is_M0

#else
  // no sleep mode and reset the TakeSampleFlag to true
  delay(3000); // period in DEBUG mode to wait between samples
  TakeSampleFlag = 1;
#endif  //RTC_MODE

  /*while awake do the following*/
  if (TakeSampleFlag)
  {
    /* wake up devices here if previously in low-power mode */
#if RTC_MODE == 1
    //write power-on functions here

    // get RTC timestamp and ID string
    DateTime now = RTC_DS.now();
    uint8_t mo = now.month();
    uint8_t d = now.day();
    uint8_t h = now.hour();
    uint8_t mm = now.minute();
    uint8_t sec = now.second();

    RTC_monthString = String(mo, DEC);
    RTC_dayString = String(d, DEC);
    RTC_hrString = String(h, DEC);
    RTC_minString = String(mm, DEC);
    RTC_secString = String(sec, DEC);
    RTC_timeString = RTC_hrString + ":" + RTC_minString + ":" + RTC_secString + "_" + RTC_monthString + "/" + RTC_dayString;

#if DEBUG == 1
    Serial.println(RTC_timeString);
#endif

#endif

    IDString = String(ID, DEC);  //changes ID int into string

    //get accelerometer data - all parameters
    if (AlertFlag) {
      //can do something special here if woken up by accelerometer
      acc_x = myIMU.readFloatAccelX();
      acc_y = myIMU.readFloatAccelY();
      acc_z = myIMU.readFloatAccelZ();
      AlertFlag = false; //Clear Accelerometer Interrupt Flag
#if DEBUG == 1
      Serial.println("Microprocessor was awakened by accelerometer!");
      delay(1000);

#endif
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
      AlertFlag = false; //Clear Accelerometer Interrupt Flag
    } // end of typical accelerometer measurements

#if DEBUG == 1
    // Check registers and interrupt status
    uint8_t dataRead;
    Serial.print("LIS3DH_INT1_SRC: 0x");
    myIMU.readRegister(&dataRead, LIS3DH_INT1_SRC);//cleared by reading
    Serial.println(dataRead, HEX);
    Serial.println("Decoded events:");
    if (dataRead & 0x40) Serial.println("Interrupt Active");
    if (dataRead & 0x20) Serial.println("Z high");
    if (dataRead & 0x10) Serial.println("Z low");
    if (dataRead & 0x08) Serial.println("Y high");
    if (dataRead & 0x04) Serial.println("Y low");
    if (dataRead & 0x02) Serial.println("X high");
    if (dataRead & 0x01) Serial.println("X low");
    Serial.println();
#endif // Debug

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

#endif // Debug

    //get NEMA data - add code here

    /* power down devices before transmit */
    // write code for powering on devices here

    /* begin transmit section here */
    //************************************************************
    //************************************************************
    
    //concatenate message
    stringTransmit = String(IDString + "," + RTC_timeString + "," + X_string + "," + Y_string + "," + Z_string + "\0"); //our concatenated string for transmit
    Serial.println(stringTransmit);
    
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
    // Reset alarm1 for next period
    setAlarmFunction();

    delay(75);  // delay so serial stuff has time to print out all the way

    TakeSampleFlag = false; // Clear Alarm Flag

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

/* Function: configInterrupts
 * Use: set the necessary registers on a sparkfun LIS3DH accelerometer to send
 *      an interrupt signal from I1
 * Precondoitions: Device is initialized using I2C constructor
 */
 
void configInterrupts()
{
  //uint8_t dataToWrite = 0;
  
  //LIS3DH_INT1_CFG   
  //dataToWrite |= 0x80;//AOI, 0 = OR 1 = AND
  //dataToWrite |= 0x40;//6D, 0 = interrupt source, 1 = 6 direction source
  //Set these to enable individual axes of generation source (or direction)
  // -- high and low are used generically
  //dataToWrite |= 0x20;//Z high
  //dataToWrite |= 0x10;//Z low
  //dataToWrite |= 0x08;//Y high
  //dataToWrite |= 0x04;//Y low
  //dataToWrite |= 0x02;//X high
  //dataToWrite |= 0x01;//X low
  myIMU.writeRegister(LIS3DH_INT1_CFG, 0x5F);
  
  //LIS3DH_INT1_THS   
  //dataToWrite = 0;
  //Provide 7 bit value, 0x7F always equals max range by accelRange setting
  //dataToWrite |= 0x10; //0x10 -> 1/8 range
  myIMU.writeRegister(LIS3DH_INT1_THS, 0x10);
  
  //LIS3DH_INT1_DURATION  
  //dataToWrite = 0;
  //minimum duration of the interrupt
  //LSB equals 1/(sample rate)
  //dataToWrite |= 0x01; // 1 * 1/100 s = 10ms
  myIMU.writeRegister(LIS3DH_INT1_DURATION, 0x0A);

  //LIS3DH_CLICK_CFG   
  //dataToWrite = 0;
  //Set these to enable individual axes of generation source (or direction)
  // -- set = 1 to enable
  //dataToWrite |= 0x20;//Z double-click
  //dataToWrite |= 0x10;//Z click
  //dataToWrite |= 0x08;//Y double-click 
  //dataToWrite |= 0x04;//Y click
  //dataToWrite |= 0x02;//X double-click
  //dataToWrite |= 0x01;//X click
  myIMU.writeRegister(LIS3DH_CLICK_CFG, 0x15);
  
  //LIS3DH_CLICK_SRC
  //dataToWrite = 0;
  //Set these to enable click behaviors (also read to check status)
  // -- set = 1 to enable
  //dataToWrite |= 0x20;//Enable double clicks
  //dataToWrite |= 0x04;//Enable single clicks
  //dataToWrite |= 0x08;//sine (0 is positive, 1 is negative)
  //dataToWrite |= 0x04;//Z click detect enabled
  //dataToWrite |= 0x02;//Y click detect enabled
  //dataToWrite |= 0x01;//X click detect enabled
  myIMU.writeRegister(LIS3DH_CLICK_SRC, 0x07);
  
  //LIS3DH_CLICK_THS   
  //dataToWrite = 0;
  //This sets the threshold where the click detection process is activated.
  //dataToWrite = 0x80 //keep interrupt high for duration of latency window
  //Provide 7 bit value, 0x7F always equals max range by accelRange setting
  //dataToWrite |= 0x0A; // ~1/16 range
  myIMU.writeRegister(LIS3DH_CLICK_THS, 0x0A);

  //LIS3DH_TIME_LIMIT  
  //dataToWrite = 0;
  //Time acceleration has to fall below threshold for a valid click.
  //LSB equals 1/(sample rate)
  //dataToWrite |= 0x08; // 8 * 1/50 s = 160ms
  myIMU.writeRegister(LIS3DH_TIME_LIMIT, 0x08);
  
  //LIS3DH_TIME_LATENCY
  //dataToWrite = 0;
  //hold-off time before allowing detection after click event
  //1 LSB equals 1/(sample rate)
  //dataToWrite |= 0x01; // 1 * 1/100 s = 10ms
  myIMU.writeRegister(LIS3DH_TIME_LATENCY, 0x10);
  
  //LIS3DH_TIME_WINDOW 
  //dataToWrite = 0;
  //hold-off time before allowing detection after click event
  //LSB equals 1/(sample rate)
  //dataToWrite |= 0x10; // 16 * 1/100 s = 160ms
  myIMU.writeRegister(LIS3DH_TIME_WINDOW, 0x10);

  //LIS3DH_CTRL_REG1   
  //dataToWrite |= 0x50;// most significant nibble controls data rate, 5 = 100Hz
  //dataToWrite |= 0x08;//Low power enable
  //dataToWrite |= 0x04;//Z enable
  //dataToWrite |= 0x02;//Y enable
  //dataToWrite |= 0x01;//X enable
  myIMU.writeRegister(LIS3DH_CTRL_REG1, 0x5F);

  //LIS3DH_CTRL_REG3
  //Choose source for pin 1
  //dataToWrite = 0;
  //dataToWrite |= 0x80; //Click detect on pin 1
  //dataToWrite |= 0x40; //AOI1 event (Generator 1 interrupt on pin 1)
  //dataToWrite |= 0x20; //AOI2 event ()
  //dataToWrite |= 0x10; //Data ready
  //dataToWrite |= 0x04; //FIFO watermark
  //dataToWrite |= 0x02; //FIFO overrun
  myIMU.writeRegister(LIS3DH_CTRL_REG3, 0xC0);

  //LIS3DH_CTRL_REG5
  //Int1 latch interrupt and 4D on  int1 (preserve fifo en)
  //myIMU.readRegister(&dataToWrite, LIS3DH_CTRL_REG5);
  //dataToWrite &= 0xF3; //Clear bits of interest
  //dataToWrite |= 0x08; //Latch interrupt (Cleared by reading int1_src)
  //dataToWrite |= 0x04; //Pipe 4D detection from 6D recognition to int1?
  myIMU.writeRegister(LIS3DH_CTRL_REG5, 0x08);
  
  //LIS3DH_CTRL_REG6
  //Choose source for pin 2 and both pin output inversion state
  //dataToWrite = 0;
  //dataToWrite |= 0x80; //Click int on pin 2
  //dataToWrite |= 0x40; //Generator 1 interrupt on pin 2
  //dataToWrite |= 0x10; //boot status on pin 2
  //dataToWrite |= 0x02; //invert both outputs
  myIMU.writeRegister(LIS3DH_CTRL_REG6, 0xC2); //C0 cends HIGH interrupt, C2 sends low interrupt
}




/******************
  RTC Subroutines
******************/

void InitializeRTC()
{
  // RTC Timer settings here
  if (! RTC_DS.begin()) {
#if DEBUG == 1
    Serial.println("Couldn't find RTC");
#endif
    while (1);
  }
  // This may end up causing a problem in practice - what if RTC looses power in field? Shouldn't happen with coin cell batt backup
  if (RTC_DS.lostPower()) {
#if DEBUG == 1
    Serial.println("RTC lost power, lets set the time!");
#endif
    // following line sets the RTC to the date & time this sketch was compiled
    RTC_DS.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  //clear any pending alarms
  clearAlarmFunction();

  // Query Time and print
  DateTime now = RTC_DS.now();
#if DEBUG == 1
  Serial.print("RTC Time is: ");
  Serial.print(now.hour(), DEC); Serial.print(':'); Serial.print(now.minute(), DEC); Serial.print(':'); Serial.println(now.second(), DEC);
#endif
  //Set SQW pin to OFF (in my case it was set by default to 1Hz)
  //The output of the DS3231 INT pin is connected to this pin
  //It must be connected to arduino Interrupt pin for wake-up
  RTC_DS.writeSqwPinMode(DS3231_OFF);

  //Set alarm1
  setAlarmFunction();
}


/* RTC helper function */
/* Function to query current RTC time and add the period to set next alarm cycle */
void setAlarmFunction()
{
  DateTime now = RTC_DS.now(); // Check the current time
  // Calculate new time
  MIN = (now.minute() + WakePeriodMin) % 60; // wrap-around using modulo every 60 sec
  HR  = (now.hour() + ((now.minute() + WakePeriodMin) / 60)) % 24; // quotient of now.min+periodMin added to now.hr, wraparound every 24hrs
#if DEBUG == 1
  Serial.print("Resetting Alarm 1 for: "); Serial.print(HR); Serial.print(":"); Serial.println(MIN);
#endif

  //Set alarm1
  RTC_DS.setAlarm(ALM1_MATCH_HOURS, MIN, HR, 0);   //set your wake-up time here
  RTC_DS.alarmInterrupt(1, true);  //code to pull microprocessor out of sleep is tied to the time -> here
}


/* RTC helper function */
/* When exiting the sleep mode we clear the alarm */
void clearAlarmFunction()
{
  //clear any pending alarms
  RTC_DS.armAlarm(1, false);
  RTC_DS.clearAlarm(1);
  RTC_DS.alarmInterrupt(1, false);
  RTC_DS.armAlarm(2, false);
  RTC_DS.clearAlarm(2);
  RTC_DS.alarmInterrupt(2, false);
  //opens up RTC to interrupts again
  uint8_t dataRead;
  myIMU.readRegister(&dataRead, LIS3DH_INT1_SRC);//cleared by reading
}

/* Debugging and upload helper,
 * Allow uploads by waiting for serial monitor to open, 
 * prevent device from going to standby too soon
 */

void setupPrint()
{
  while(!Serial);     //Won't start anything until serial is open, comment these lines out if powering from battery
    Serial.println("***** Interrupt Test *****");
    
    // ***** IMPORTANT *****
    // Delay is required to allow the USB interface to be active during
    // sketch upload process
    
    Serial.println("Entering test mode in:");
    for (count; count > 0; count--)
    {
      Serial.print(count);
      Serial.println(" s");
      delay(1000);
    }
}



/************************************************************************************************
   ISR pin assignments - for 32u4 ONLY
************************************************************************************************/
/* Wakeup in SQW ISR */
/* Function to init PCI interrupt pin */
/* Pulled from: https://playground.arduino.cc/Main/PinChangeInterrupt */

#ifdef is_32u4
void pciSetup(byte pin) // allows you to change variables
{
  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
  PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
  PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

ISR (PCINT0_vect) // handle pin change interrupt for D8 to D13 here
{
  if (digitalRead(11) == LOW)  //trigger wake when alarm time
    TakeSampleFlag = true;
  if (digitalRead(12) == HIGH) // triggers wake when accelerometer interrupt
  {
    AlertFlag = true;
    TakeSampleFlag = true;
  }

}
#endif //is_32u4











