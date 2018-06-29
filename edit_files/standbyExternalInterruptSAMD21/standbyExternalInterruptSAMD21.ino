// **** INCLUDES *****
#include "LowPower.h"

// External interrupt on pin 0 (use pin 0 to 24, except pin 4 on Arduino Zero)
const int pin = 0;
unsigned char count = 10;

void setup()
{
	// Wait for serial USB port to open
	while(!Serial);
	Serial.println("***** ATSAMD21 Standby Mode Example *****");
	
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
  // *********************
    
  // External interrupt on pin (example: press of an active low button)
  // A pullup resistor is used to hold the signal high when no button press
  attachInterrupt(pin, blink, LOW);
}

void loop() 
{
	Serial.println("Entering standby mode.");
	Serial.println("Apply low signal to wake the processor.");
	Serial.println("Zzzz...");
	// Detach USB interface
	USBDevice.detach();
  // Enter standby mode
  LowPower.standby();  
  // Attach USB interface
  USBDevice.attach();
  // Wait for serial USB port to open
  while(!Serial);
  // Serial USB is blazing fast, you might miss the messages
  delay(1000);
  Serial.println("Awake!");
  Serial.println("Send any character to enter standby mode again");
  // Wait for user response
  while(!Serial.available());
  while(Serial.available() > 0)
  {
		Serial.read();
	}
}

void blink(void)
{

}
