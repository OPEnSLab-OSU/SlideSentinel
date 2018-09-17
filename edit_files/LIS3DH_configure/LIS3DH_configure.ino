/*******************************************************************************************
    SlideSentinel: Phase 1 Sensor code
    Author: Marissa Kwon,
    9/14/18
       This code is meant to compare accelerometer sensitivity to expected sensitivity based on configuration settings.
       This code can also be used to observe current consumption related to high and low resolution settings and sample rates
    Use ctrl-f and search for every "IMPORTANT" comment before compiling and running,
    these include notes for libraries and edits to dependant files.

    IMPORTANT: compatible with edited version of LIS3DH source code supporting changes for enabling/disabling low power mode

    Slide Sentinel master code, fully operational, configure using preprocessor definitions
    info on devices can be found here:
    LIS3DH: https://learn.sparkfun.com/tutorials/lis3dh-hookup-guide
      SOURCE CODE FROM EXAMPLE: "IntUsage.ino"
      Marshall Taylor @ SparkFun Electronics
      Nov 16, 2016
      https://github.com/sparkfun/LIS3DH_Breakout
      https://github.com/sparkfun/SparkFun_LIS3DH_Arduino_Library
      See ST docs for information
      Doc ID 18198 (AN3308): LIS3DHpplication information
      Doc ID 17530: LIS3DH datasheet
**********************************************************************************************/
// necessary libraries here
#include <Arduino.h>
#include <SPI.h>
#include <SparkFunLIS3DH.h> // from sparkfun accelerometer library found here https://github.com/sparkfun/SparkFun_LIS3DH_Arduino_Library
#include <Wire.h>
#include <SD.h>

// Pins
#define SD_CS 10

// Device Settings
#define DUMP_REG 1 // useful for debugging - gives hex values for all registers
// use these if you want to "hardcode" own accelerometer settings or use macros and default configuration function
#define DEFAULT 1
#define CUSTOM_LOW 0 // if true will override default
#define CUSTOM_HIGH 0  // if true will override all other settings
// consts and macros below will be ignored if CUSTOM is set to 1
#define LOW_RES 1 //
#define HI_RES 0 // currently does nothing in code
// Hz.  Can be: 0,1,10,25,50,100,200,400,1600,5000 Hz
const int freq = 100;

uint8_t dataRead;
LIS3DH myIMU(I2C_MODE, 0x19); //Default constructor is I2C, addr 0x19.

/**********************************************************************************************
   setup()
   Description:
        - begin serial
        - assign pinmodes/write to pins
        - assign IMU settings (LIS3DH), init, and configure IMU
**********************************************************************************************/
void setup() {
  Serial.begin(9600);
  delay(1000);
  initializeAccelerometer();

  if (!SD.begin(SD_CS)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }
  Serial.println("SD card initialized.");

#if DUMP_REG // edit later to read other registers needed
  for ( int i = LIS3DH_INT_COUNTER_REG; i <= LIS3DH_INT1_DURATION; i++) {
    Serial.print("0x");
    Serial.print(i, HEX);
    Serial.print(": 0x");
    myIMU.readRegister(&dataRead, i);
    Serial.println(dataRead, HEX);
  }
#endif
}

/**********************************************************************************************
   loop()
   Description:
**********************************************************************************************/
void loop() {
  Serial.println("creating filenames");
  unsigned long startmillis, endmillis;

  startmillis = millis();

  // Get all parameters
  String x_axis = String(myIMU.readFloatAccelX(), 6);
  String y_axis = String(myIMU.readFloatAccelY(), 6);
  String z_axis = String(myIMU.readFloatAccelZ(), 6);

  endmillis =  millis();

  Serial.print("elapsed time to read/convert: ");
  Serial.println(endmillis - startmillis);

  Serial.print("\nAccelerometer:\n");
  Serial.print(" X = ");
  Serial.println(x_axis);
  Serial.print(" Y = ");
  Serial.println(y_axis);
  Serial.print(" Z = ");
  Serial.println(z_axis);

  String myfile = get_filename(); // make SD file based on freq and settings

  startmillis = millis();

  File dataFile = SD.open(myfile, FILE_WRITE);
  if (dataFile) {
    dataFile.print(x_axis);
    dataFile.print(",");
    dataFile.print(y_axis);
    dataFile.print(",");
    dataFile.println(z_axis);
    dataFile.close();
  }

  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  }

  endmillis =  millis();

  Serial.print("elapsed time to write:");
  Serial.println(endmillis - startmillis);
  Serial.println(freq);


  Serial.print("LIS3DH_INT1_SRC: 0x");
  clearInterrupts();
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

  delay(1000);
}

/**********************************************************************************************
   Helper Functions - configInterrupts, initializeRTC, setRTCAlarm, clearRTCAlarm
**********************************************************************************************/

/******************
  Accelerometer
******************/
/* More info about listed data registers on the LISH3DH datasheet: */
/* http://www.st.com/content/ccc/resource/technical/document/datasheet/3c/ae/50/85/d6/b1/46/fe/CD00274221.pdf/files/CD00274221.pdf/jcr:content/translations/en.CD00274221.pdf */

/* Function: configInterrupts
   Use: set the necessary registers on a sparkfun LIS3DH accelerometer to send
        an interrupt signal from I1
   Precondoitions: Device is initialized using I2C constructor
   Description:
   Interrupt support for the LIS3DH is extremely flexible, so configuration must be left
   to the user.  This sketch demonstrates how to make a interrupt configuration function.
   Use configIntterupts() as a template, then comment/uncomment desired options.
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
  myIMU.writeRegister(LIS3DH_CLICK_THS, 0x30);

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
  //myIMU.writeRegister(LIS3DH_CTRL_REG1, 0x97); //disable low power and sample at 1.344 kHz

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

void clearInterrupts() {
  uint8_t dataRead;
  myIMU.readRegister(&dataRead, LIS3DH_INT1_SRC);//cleared by reading
}

void initializeAccelerometer() {
  // Replaced by single write to CTRL_REG1 to disable low power mode
  // Accel sample rate and range effect interrupt time and threshold values with device freq.
  myIMU.settings.accelSampleRate = freq;  // Hz.  Can be: 0,1,10,25,50,100,200,400,1600,5000 Hz
  myIMU.settings.accelRange = 2;         // Max G force readable.  Can be: 2, 4, 8, 16
  myIMU.settings.adcEnabled = 0;
  myIMU.settings.tempEnabled = 0;
  myIMU.settings.xAccelEnabled = 1;
  myIMU.settings.yAccelEnabled = 1;
  myIMU.settings.zAccelEnabled = 1;
#if LOW_RES == 1
  myIMU.settings.lowPowerEnabled = 1;
#endif
#if LOW_RES == 0
  myIMU.settings.lowPowerEnabled = 0;
#endif
  // high resolution in CTRL_REG4 enabled by default ???

  // Call .begin() to configure the IMU and apply settings made
  myIMU.begin(); // NOTE: applies changes made to REG1 and REG4 when myIMU.settings members are changed

  //myIMU.writeRegister(LIS3DH_CTRL_REG1, 0x5F); //default
#if CUSTOM_LOW == 1 // apply no HI RES settings to REG1 and REG4
  myIMU.writeRegister(LIS3DH_CTRL_REG4, 0x00); //disable Hi Res
  myIMU.writeRegister(LIS3DH_CTRL_REG1, 0x5F); //enable low power and sample at 100 Hz
#endif

#if CUSTOM_HIGH == 1 // apply custom settings to REG1 and REG4
  myIMU.writeRegister(LIS3DH_CTRL_REG1, 0x97); //disable low power and sample at 1.344 kHz
  myIMU.writeRegister(LIS3DH_CTRL_REG4, 0x08); //enable high resolution reads
#endif

  configInterrupts(); // see function for interrupt settings
  clearInterrupts(); // start fresh
}

//IMPORTANT: filename before extension CANNOT exceed 8 characters
String get_filename() {
  String temp = "";
  temp += String(freq);
  if (LOW_RES && DEFAULT) {
    temp += "_DLO";
  }
  else if (CUSTOM_LOW) {
    temp += "_CLO";
  }
  else if (CUSTOM_HIGH) {
    temp += "_CHI";
  }
  else {
    temp += "_DEF";
  }

  temp += ".txt";
  return temp;
}

