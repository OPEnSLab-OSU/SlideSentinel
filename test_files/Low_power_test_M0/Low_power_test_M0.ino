// test code that pulls microprocessor out of "stand by" with a signal from the accelerometer

// **** INCLUDES *****
#include "LowPower.h"
#include "SparkFunLIS3DH.h"
#include "Wire.h"
#include "SPI.h"

LIS3DH myIMU(I2C_MODE, 0x19); //Default constructor is I2C, addr 0x19.


// Use pin 11 as wake up pin
const int wakeUpPin = 11;
volatile int count = 10;
volatile bool alert_flag = false;

void setup()
{ // put your setup code here, to run once:
  
  //Serial.begin(9600);
  delay(1000); //relax...
  
  //Serial.println("Processor came out of reset.\n");
  delay(1000); //relax...

  // Configure wake up pin as input.
  // This will consume few uA of current.
  pinMode(wakeUpPin, INPUT_PULLUP);

  
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
  
  configInterrupts(); //interrupt triggers with change in position

  //for debugging
  setupPrint(); //wait for serial to open before starting loop() 

  Serial.println("Ready for interrupt");
  attachInterrupt(digitalPinToInterrupt(wakeUpPin), wakeUp, LOW);

}

void loop()
{
  digitalWrite(LED_BUILTIN, HIGH); //LED active with processor wake

  if(alert_flag)
  {
    //Serial.println("Processor interrupt triggered");    
    alert_flag = false;
    attachInterrupt(digitalPinToInterrupt(wakeUpPin), wakeUp, LOW);
  }


  //handle standby here
 
  //Serial.println("TEST!");

  //Serial.println("STANDBY MODE");
  //Serial.end(); //end ALL sercoms before the device goes to sleep

  // Allow wake up pin to trigger interrupt on rising edge.
  // Wake up when acc input is returning to idle

  delay(1000);
  digitalWrite(LED_BUILTIN, LOW); //LED on when processor is awake
  
  // For M0:
  LowPower.standby();
  //M0 end
  detachInterrupt(digitalPinToInterrupt(wakeUpPin)); //detaching interrupt in ISR causes issues?

  //Serial.begin(9600);
  delay(2000);
  //Serial.print("WOKE");
  
  // For 32u4:
  // Enter power down state with ADC and BOD module disabled.
  // LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  //32u4 end
  

}

void wakeUp()
{
  // Disable external pin interrupt on wake up pin.

    
  // Just a handler for the pin interrupt.
  alert_flag = true;
  uint8_t dataRead;
  myIMU.readRegister(&dataRead, LIS3DH_INT1_SRC);   //cleared by reading
}

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
  //LSB equals 1/(sample rate)
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

void setupPrint()
{
  //while(!Serial);     //Won't start anything until serial is open
    //Serial.println("***** Interrupt Test *****");
    
    // ***** IMPORTANT *****
    // Delay is required to allow the USB interface to be active during
    // sketch upload process
    
    //Serial.println("Entering test mode in:");
    for (count; count > 0; count--)
    {
      //Serial.print(count);
      //Serial.println(" s");
      delay(1000);
    }
}


