/**************************************************************************/
/*!
Interrupt configuration for mma8451 accelerometer
Important functionality is encoded within the configInterrupts() function
Must use the updated MMA8451 Library included within this repo
*/
/**************************************************************************/

#include <Wire.h>
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>
#include <EnableInterrupt.h>
#include "wiring_private.h" // pinPeripheral() function

#define ACCEL_EN_PIN 10

/* function declarations */ 
void mmaCSVRead(Adafruit_MMA8451 device);
void configInterrupts(Adafruit_MMA8451 device);
void mmaSetupSlideSentinel();


/* global variable declarations */ 
Adafruit_MMA8451 mma = Adafruit_MMA8451();
bool accelFlag = false;
unsigned long int read_time;    // timeout for reading strings as not to overload serial monitor
sensors_event_t event; 

/* Interrupt service routine for accelerometer interrupts */
void wakeUpAccel()
{
  accelFlag = true;
  detachInterrupt(digitalPinToInterrupt(ACCEL_EN_PIN));
}

void setup(void) {
  Serial.begin(9600);

  while(!Serial);   // wait for the serial monitor to open before executing any code
  
  Serial.println("Adafruit MMA8451 test!");

  mmaSetupSlideSentinel();
  
  accelFlag = false;
  configInterrupts(mma);

  digitalWrite(ACCEL_EN_PIN, INPUT_PULLUP); // pulldown interrupt
  attachInterrupt(digitalPinToInterrupt(ACCEL_EN_PIN), wakeUpAccel, LOW);
  Serial.println("Interrupt attached");
  
  mma.readRegister8(MMA8451_REG_TRANSIENT_SRC); //clear the interrupt register
  read_time = millis();
}

void loop() {
    // Read the 'raw' data in 14-bit counts, print to serial in CSV format time(ms),X,Y,Z
    if(millis() - read_time > 500 && !accelFlag){ 
        mmaCSVRead(mma);
        read_time = millis();
    }
    
    // perform any bigger interrupt related actions here, this will just print some info to show what interrupted the accel
    if(accelFlag){
        Serial.println("Interrupt triggered");   
        accelFlag = false; // reset flag, clear the interrupt
        mma.readRegister8(MMA8451_REG_TRANSIENT_SRC); //clear the interrupt register
        
        // reattach the interrupt, can be done anywhere in code, but only after the interrupt has triggered and detached
        attachInterrupt(digitalPinToInterrupt(ACCEL_EN_PIN), wakeUpAccel, LOW);
    }
}

// comment/uncomment these to enable functionality described
/* Transient detection donfiguration for mma accelerometer, use this format and Adafruit_MMA8451::writeRegister8_public to configure registers */
void configInterrupts(Adafruit_MMA8451 device){
    uint8_t dataToWrite = 0;

    // MMA8451_REG_CTRL_REG3
    // Interrupt control register

    //dataToWrite |= 0x80;    // FIFO gate option for wake/sleep transition, default 0
    dataToWrite |= 0x40;    // Wake from transient interrupt enable
    //dataToWrite |= 0x20;    // Wake from orientation interrupt enable
    //dataToWrite |= 0x10;    // Wake from Pulse function enable
    //dataToWrite |= 0x08;    // Wake from freefall/motion decect interrupt
    //dataToWrite |= 0x02;    // Interrupt polarity 1 = active high
    dataToWrite |= 0x01;    // (0) Push/pull or (1) open drain interrupt, determines whether bus is driven by device, or left to hang

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
    // Interrupt pin configuration register, select interrupt on pin 0 or pin 1
    // see datasheet for register's description, ths int routed to pin 2

    device.writeRegister8_public(MMA8451_REG_CTRL_REG5, dataToWrite | 0x01);

    dataToWrite = 0;
    
    // MMA8451_REG_TRANSIENT_CFG
    dataToWrite |= 0x10;  // Latch enable to capture accel values when interrupt occurs
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
void mmaCSVRead(Adafruit_MMA8451 device){
    device.getEvent(&event);
    Serial.print(millis()); Serial.print(","); 
    Serial.print(device.x); Serial.print(",");
    Serial.print(device.y); Serial.print(",");
    Serial.println(device.z);
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
  mma.setDataRate(MMA8451_DATARATE_6_25HZ);
  Serial.print("Range = "); Serial.print(2 << mma.getRange()); Serial.println("G");

  while (mma.readRegister8(MMA8451_REG_CTRL_REG2) & 0x40);
}

