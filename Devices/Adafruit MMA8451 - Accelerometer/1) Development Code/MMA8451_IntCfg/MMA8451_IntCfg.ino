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


Adafruit_MMA8451 mma = Adafruit_MMA8451();
bool accelFlag = false;
int dataRead;
int counter = 0;
sensors_event_t event; 

// Interrupt service routine
void wakeUpAccel()
{
  accelFlag = true;
  mma.readRegister8(MMA8451_REG_TRANSIENT_SRC);//cleared by reading
}

void setup(void) {
  Serial.begin(9600);

  while(!Serial);
  
  Serial.println("Adafruit MMA8451 test!");
  

  if (! mma.begin()) {
    Serial.println("Couldnt start");
    while (1);
  }
  Serial.println("MMA8451 found!");
  
  mma.setRange(MMA8451_RANGE_2_G);
  mma.setDataRate(MMA8451_DATARATE_6_25HZ);
  Serial.print("Range = "); Serial.print(2 << mma.getRange()); Serial.println("G");

  while (mma.readRegister8(MMA8451_REG_CTRL_REG2) & 0x40);

  mma.writeRegister8_public(MMA8451_REG_CTRL_REG3, 0x41);
  Serial.print("dataToWrite: "); 
  Serial.println(0x41, HEX);
  dataRead = mma.readRegister8(MMA8451_REG_CTRL_REG3);
  Serial.print("MMA8451_REG_CTRL_REG3: "); 
  Serial.println(dataRead, HEX);

  digitalWrite(ACCEL_EN_PIN, INPUT_PULLUP);
  
  accelFlag = false;
  configInterrupts(mma);
  attachInterrupt(digitalPinToInterrupt(ACCEL_EN_PIN), wakeUpAccel, LOW);
  Serial.println("Int attached");
  mma.readRegister8(MMA8451_REG_TRANSIENT_SRC); //clear the interrupt register
}

void loop() {
  // Read the 'raw' data in 14-bit counts
//        mma.getEvent(&event);
//        Serial.print(millis()); Serial.print(","); 
//        Serial.print(mma.x); Serial.print(",");
//        Serial.print(mma.y); Serial.print(",");
//        Serial.println(mma.z);
    if(accelFlag){
    
        Serial.println("Interrupt triggered");   
        accelFlag = false;
    }
}

// comment/uncomment these to enable functionality
// Transient detection donfiguration for mma accelerometer
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
    //TESTdataToWrite |= 0x01;    // (0) Push/pull or (1) open drain interrupt, determines whether bus is driven by device, or left to hang

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

