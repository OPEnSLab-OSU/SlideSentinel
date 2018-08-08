// For Feather M0 needs work on the RTC and accelerometer interrupt routines
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

// Define macro constants
#define DEBUG 1  //test to allow print to serial monitor
#define RTC_MODE 1 //enable RTC 
#define SD_WRITE 0  //enable SD card logging (too much for 32u4 processor memory)

//NOTE: Must include the following line in the RTClibExtended.h file to use with M0:
//#define _BV(bit) (1 << (bit))
#include <RTClibExtended.h>
#include <EnableInterrupt.h>

#ifdef __SAMD21G18A__
#define is_M0
#endif

//===== LoRa Initializations =====

#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define SERVER_ADDRESS 2
#define RF95_FREQ 915.0  // Change to 434.0 or other frequency, must match RX's freq!
#define SAMPLE_SIZE 5  // how many samples to take for average value
#define VBATPIN A7

float measuredvbat;

// ======== General Settings ==========



// Create instances of sensors
LIS3DH myIMU(I2C_MODE, 0x19); //Default accel constructor is I2C, addr 0x19.
RH_RF95 rf95(RFM95_CS, RFM95_INT);  //instance of LoRA
RHReliableDatagram manager(rf95, SERVER_ADDRESS);  //LoRa message verification

RTC_DS3231 RTC_DS; //instance of DS3231 RTC

// ========= Declare Global Variables ==============

int transmitBufLen; // length of transmit buffer
const int ID = 100; // id unique to device
String IDString, NEMA_string, X_string, Y_string, Z_string, RTC_monthString, RTC_dayString, RTC_hrString, RTC_minString, RTC_timeString = "", stringTransmit = "";

// Declare RTC/Accelerometer specific variables
volatile bool TakeSampleFlag = false; // Flag is set with external Pin A0 Interrupt by RTC
volatile bool AlertFlag = false; // Flag if an alert is triggered and we need to speed up some processes (Phase 2)
volatile bool TimerFlag = false;
volatile bool RTCFlag = false;
volatile int HR = 8; // Hr of the day we want alarm to go off
volatile int MIN = 0; // Min of each hour we want alarm to go off
volatile int WakePeriodMin = 1;  // Period of time to take sample in Min, reset alarm based on this period (Bo - 5 min)
volatile int count = 10; //number of seconds to wait before running loop()
const byte wakeUpPin = 12;  // attach to SQW pin on RTC
const byte alertPin = 17;  // attach A3 to int1 on accelerometer
unsigned long timer;
uint8_t dataRead; // for acceleromter interrupt register

//RTK and serial variables

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
  TimerFlag = true;
  myIMU.readRegister(&dataRead, LIS3DH_INT1_SRC);//cleared by reading
}

void wakeUp_RTC()
{
  detachInterrupt(digitalPinToInterrupt(wakeUpPin));
  TimerFlag = true;
  RTCFlag = true;
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
  setupPrint(); //give the device time to wake up and upload sketch if necessary
#else 
  // when not in debug mode, dont wait for serial to start code segment
  // delay 15s for sketch upload
  delay(15000); 
#endif //DEBUG == 1

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

  // check if frequency is initialized
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
  pinMode(wakeUpPin, INPUT_PULLUP);  // pull up resistors required for Active-Low interrupts
  pinMode(alertPin, INPUT_PULLUP);

#if DEBUG == 1
  Serial.println("Processor Setup Successful.\n");
#endif

  //Accel sample rate and range effect interrupt time and threshold values with device freq.
  myIMU.settings.accelSampleRate = 100;  // Hz.  Can be: 0,1,10,25,50,100,200,400,1600,5000 Hz
  myIMU.settings.accelRange = 2;         // Max G force readable.  Can be: 2, 4, 8, 16
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

#endif //RTC_MODE == 1
  timer=millis();
}

void loop() {

  if(TimerFlag){ //triggered with every wake-up interrupt
    
    //**************** VERY IMPORTANT ****************
    //clear interrupt registers, attach interrupts EVERY TIME THE INTERRUPT IS CALLED
    intClearSet();
    
    timer=millis();
    TimerFlag=false;
  }
  
  if(RTCFlag){
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
    delay(75);  // delay so serial stuff has time to print out all the way
    RTCFlag=false;
  }

  if(AlertFlag){
    Serial.println("Processor wake from accelerometer");
    delay(20);
    AlertFlag=false;
  }

  //this is the standby loop, execute last in order
  if(millis()-timer > 10000){ //10s delay, then sleep
    //reset all flags
    TimerFlag = false;
    AlertFlag = false;
    TakeSampleFlag=false;

    Serial.println("STANDBY");    
    setRTCAlarm(); //reset alarm to go off one wake period from sleeping
    Serial.end();
    USBDevice.detach();
    intClearSet(); //clear interrupt registers, attach interrupts
    delay(10);
    #if DEBUG == 1
    digitalWrite(LED_BUILTIN, LOW);
    #endif
    LowPower.standby();
    USBDevice.attach();
    #if DEBUG == 1    
    digitalWrite(LED_BUILTIN, HIGH);
    delay(5000); // give user 5s to close and reopen serial monitor!
    #endif
    clearRTCAlarm(); //prevent double trigger of alarm interrupt
    Serial.begin(115200);
    Serial.println("WAKE");
  }
}


/**********************************************************************************************
   Helper Functions - configInterrupts, InitializeRTC, setRTCAlarm, clearRTCAlarm
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
  clearRTCAlarm();

  // Query Time and print
  DateTime now = RTC_DS.now();

  //Set SQW pin to OFF (in my case it was set by default to 1Hz)
  //The output of the DS3231 INT pin is connected to this pin
  //It must be connected to arduino Interrupt pin for wake-up
  RTC_DS.writeSqwPinMode(DS3231_OFF);

  //Set alarm1
  setRTCAlarm();
}


/* RTC helper function */
/* Function to query current RTC time and add the period to set next alarm cycle */
void setRTCAlarm()
{
  DateTime now = RTC_DS.now(); // Check the current time
  // Calculate new time
  MIN = (now.minute() + WakePeriodMin) % 60; // wrap-around using modulo every 60 sec
  HR  = (now.hour() + ((now.minute() + WakePeriodMin) / 60)) % 24; // quotient of now.min+periodMin added to now.hr, wraparound every 24hrs
#if DEBUG == 1
  Serial.print("RTC Time is: ");
  Serial.print(now.hour(), DEC); Serial.print(':'); Serial.print(now.minute(), DEC); Serial.print(':'); Serial.println(now.second(), DEC);
  Serial.print("Resetting Alarm 1 for: "); Serial.print(HR); Serial.print(":"); Serial.println(MIN);
#endif

  //Set alarm1
  RTC_DS.setAlarm(ALM1_MATCH_HOURS, MIN, HR, 0);   //set your wake-up time here
  RTC_DS.alarmInterrupt(1, true);  //code to pull microprocessor out of sleep is tied to the time -> here
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
  //opens up RTC to interrupts again
  //uint8_t dataRead;
  //myIMU.readRegister(&dataRead, LIS3DH_INT1_SRC);//cleared by reading
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

void intClearSet(){
    detachInterrupt(digitalPinToInterrupt(alertPin));
    detachInterrupt(digitalPinToInterrupt(wakeUpPin));
    delay(20);
    attachInterrupt(digitalPinToInterrupt(alertPin), wakeUp_alert, LOW);
    attachInterrupt(digitalPinToInterrupt(wakeUpPin), wakeUp_RTC, LOW);
}







