// ================================================================
// ===              INCLUDE CONFIGURATION FILE                  ===
// ===    INCLUDE DECLARATIONS, STRUCTS, AND FUNCTIONS FROM     ===
// ===            OTHER FILES AS SET IN CONFIG.H                ===
// ================================================================

// ********************** PROGRAM DEXCRIPTION *********************
// The LOOM_HUB acts as the data manager for the Slide Sentinel project
// It receives and handles uploads of all nmea and accelerometer data,
//  it does not handle any data going to the rovers, but will take
//  data from all nodes, determine the "best" nmea data and upload it
//  in a dump to cellular periodically. Transmission is handled by 
//  a second microprocessor on board the hub which is dedicated to sending 
//  complete RTK transmissions.
// ****************************************************************


// Config has to be first has it hold all user specified options
#include "config.h"

// Preamble includes any relevant subroutine files based
// on options specified in the above config
#include "loom_preamble.h"

#include <Arduino.h>   // required before wiring_private.h
#include "wiring_private.h" // pinPeripheral() function
#include <Wire.h>
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>
//#include <EnableInterrupt.h>
#include "wiring_private.h" // pinPeripheral() function



/* function declarations */ 
void mmaCSVRead(Adafruit_MMA8451 device, String& to_fill, int count);
void configInterrupts(Adafruit_MMA8451 device);
void mmaPrintIntSRC(uint8_t dataRead);
void mmaSetupSlideSentinel();


#define BAUD 57600    // reading and writing occurs at 
#define DEBUG 1       // turn on debug mode
#define DEBUG_SD 1
#define CELLULAR 0
#define ACCEL_INT_PIN A3

Adafruit_MMA8451 mma = Adafruit_MMA8451();
sensors_event_t event; 
String accel_data;
bool accelFlag;
unsigned long int count, timer;

/* RX pin 6, TX pin A5, configuring for rover to GPS UART */
// comment out SERCOM5 definition in variants.cpp
Uart Serial3 (&sercom5, 6, A5, SERCOM_RX_PAD_2, UART_TX_PAD_0);
void SERCOM5_Handler()
{
  Serial3.IrqHandler();
}

/* Interrupt service routine for accelerometer interrupts */
void wakeUpAccel()
{
  detachInterrupt(digitalPinToInterrupt(ACCEL_INT_PIN));
  accelFlag = true;
}

void busyWait(){
  while(1);
}

// ================================================================
// ===                           SETUP                          ===
// ================================================================
void setup()
{
  // LOOM_begin calls any relevant (based on config) LOOM device setup functions
  Loom_begin();
  //setup code here, to run once:
  Serial.begin(115200);         //Opens the main serial port to communicate with the computer
  Serial3_setup();
    
  Serial.println("Adafruit MMA8451 test!");

  mmaSetupSlideSentinel();
  
  accelFlag = false;
  configInterrupts(mma);

  digitalWrite(ACCEL_INT_PIN, INPUT_PULLUP); // pulldown interrupt
  attachInterrupt(digitalPinToInterrupt(ACCEL_INT_PIN), wakeUpAccel, LOW);
  Serial.println("Interrupt attached");
  
  mma.readRegister8(MMA8451_REG_TRANSIENT_SRC); //clear the interrupt register
  timer = millis();
  count = 0;
  
//  digitalWrite(13, INPUT);
//  attachInterrupt(digitalPinToInterrupt(13), busyWait) // infinite loop to disconnect devi
  //Any custom setup code
}

// ================================================================
// ===                        MAIN LOOP                         ===
// ================================================================
void loop()
{
  
        accel_data = "";
        mmaCSVRead(mma, accel_data, count);
        uint8_t dataRead = mma.readRegister8(MMA8451_REG_TRANSIENT_SRC); //clear the interrupt register
        if(accelFlag){
            Serial.println("INT TRIGGER");
            digitalWrite(ACCEL_INT_PIN, INPUT_PULLUP);
            accelFlag = false; // reset flag, clear the interrupt
            // reattach the interrupt, can be done anywhere in code, but only after the interrupt has triggered and detached
            EIC->INTFLAG.reg = 0x01ff; // clear interrupt flag pending
            attachInterrupt(digitalPinToInterrupt(ACCEL_INT_PIN), wakeUpAccel, LOW);
            accel_data += ',';
            accel_data += dataRead;
            Serial.println(accel_data);
        }

    
  
} // End loop section


bool sd_save_elem_nodelim(char *file, char* data)
{  
  #if is_lora == 1
    digitalWrite(8, HIGH);  // if using LoRa
  #endif
  SD.begin(chipSelect); // It seems that SD card may become 'unsetup' sometimes, so re-setup
  
  sdFile = SD.open(file, FILE_WRITE);

  if (sdFile) {
    LOOM_DEBUG_Println4("Saving ", data, " to SD file: ", file);
    sdFile.print(data);
  } else {
    LOOM_DEBUG_Println2("Error opening: ", file);
    return false;
  }
  sdFile.close();
  return true;
}

// comment/uncomment these to enable functionality described
/* Transient detection donfiguration for mma accelerometer, use this format and Adafruit_MMA8451::writeRegister8_public to configure registers */
void configInterrupts(Adafruit_MMA8451 device){
    uint8_t dataToWrite = 0;

    // MMA8451_REG_CTRL_REG3
    // Interrupt control register

    dataToWrite = 0;
    dataToWrite |= 0x80;      // FIFO gate option for wake/sleep transition, default 0
    dataToWrite |= 0x40;      // Wake from transient interrupt enable
    //dataToWrite |= 0x20;    // Wake from orientation interrupt enable
    //dataToWrite |= 0x10;    // Wake from Pulse function enable
    //dataToWrite |= 0x08;    // Wake from freefall/motion decect interrupt
    //dataToWrite |= 0x02;    // Interrupt polarity, 1 = active high
    dataToWrite |= 0x00;      // (0) Push/pull or (1) open drain interrupt, determines whether bus is driven by device, or left to hang

    device.writeRegister8_public(MMA8451_REG_CTRL_REG3, dataToWrite);

    dataToWrite = 0;

    // MMA8451_REG_CTRL_REG4
    // Interrupt enable register, enables interrupts that are not commented

    //dataToWrite |= 0x80;    // Auto sleep/wake interrupt
    //dataToWrite |= 0x40;    // FIFO interrupt
    dataToWrite |= 0x20;    // Transient interrupt - enabled
    //dataToWrite |= 0x10;    // orientation
    //dataToWrite |= 0x08;    // Pulse interrupt
    //dataToWrite |= 0x04;    // Freefall interrupt
    dataToWrite |= 0x01;    // data ready interrupt, MUST BE ENABLED FOR USE WITH ARDUINO
    device.writeRegister8_public(MMA8451_REG_CTRL_REG4, dataToWrite | 0x01);
    
    dataToWrite = 0;

    // MMA8451_REG_CTRL_REG5
    // Interrupt pin 1/2 configuration register, bit == 1 => interrupt to pin 1
    // see datasheet for interrupt's description, threshold int routed to pin 1
    // comment = int2, uncoment = int1

    //dataToWrite |= 0x80;    // Auto sleep/wake
    //dataToWrite |= 0x40;    // FIFO
    dataToWrite |= 0x20;    // Transient
    //dataToWrite |= 0x10;    // orientation
    //dataToWrite |= 0x08;    // Pulse
    //dataToWrite |= 0x04;    // Freefall
    //dataToWrite |= 0x01;    // data ready

    device.writeRegister8_public(MMA8451_REG_CTRL_REG5, dataToWrite);

    dataToWrite = 0;
    
    // MMA8451_REG_TRANSIENT_CFG
    //dataToWrite |= 0x10;  // Latch enable to capture accel values when interrupt occurs
    dataToWrite |= 0x08;    // Z transient interrupt enable
    dataToWrite |= 0x04;    // Y transient interrupt enable
    dataToWrite |= 0x02;    // X transient interrupt enable
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
void mmaCSVRead(Adafruit_MMA8451 device, String& to_fill, unsigned long int count){
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
void mmaSetupSlideSentinel(){
  if (! mma.begin()) {
    Serial.println("Couldnt start");
    while (1);
  }
  
  Serial.println("MMA8451 found!");

  // library configurations
  mma.setRange(MMA8451_RANGE_2_G);
  mma.setDataRate(MMA8451_DATARATE_6_25_HZ);
  Serial.print("Range = "); Serial.print(2 << mma.getRange()); Serial.println("G");

  while (mma.readRegister8(MMA8451_REG_CTRL_REG2) & 0x40);
}


void mmaPrintIntSRC(uint8_t dataRead){
    if(dataRead & 0x40) Serial.println("Event Active");
    if(dataRead & 0x20){
        Serial.println("\tZ event");
        if(dataRead & 0x10) Serial.println("\t\tZ Negative g");
        else Serial.println("\t\tZ Positive g");
    }
    if(dataRead & 0x08){
        Serial.println("\tY event");
        if(dataRead & 0x04) Serial.println("\t\tY Negative g");
        else Serial.println("\t\tY Positive g");
    }
    if(dataRead & 0x02){
        Serial.println("\tX event");
        if(dataRead & 0x01) Serial.println("\t\tX Negative g");
        else Serial.println("\t\tX Positive g");
    }
}

void Serial3_setup() {
  Serial3.begin(115200);          //tx from rover to pin 6
  digitalWrite(6,INPUT_PULLUP);
  // Assign pins 6 & A5 SERCOM functionality, internal function
  pinPeripheral(6, PIO_SERCOM);  //Private functions for serial communication
  pinPeripheral(A5, PIO_SERCOM_ALT);
}
