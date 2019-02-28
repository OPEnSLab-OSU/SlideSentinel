// Config has to be first has it hold all user specified options
#include "config.h"

// Preamble includes any relevant subroutine files based
// on options specified in the above config
#include "loom_preamble.h"
#include "wiring_private.h" // pinPeripheral() function
#include <Wire.h>
#include <SLIPEncodedSerial.h>



#define SERIAL3_RX    A5
#define SERIAL3_TX    6


Uart Serial3 (&sercom5, SERIAL3_RX, SERIAL3_TX, SERCOM_RX_PAD_0, UART_TX_PAD_2);
void SERCOM5_Handler()
{
  Serial3.IrqHandler();
}

SLIPEncodedSerial SLIPSerial(Serial3);

void PrintBund(OSCMessage &msg)
{
    print_message(&msg);
}

void setup()
{
  Serial.begin(115200);
  SLIPSerial.begin(9600);
  pinPeripheral(SERIAL3_TX, PIO_SERCOM);  //Private functions for serial communication
  pinPeripheral(SERIAL3_RX, PIO_SERCOM_ALT);
}

void loop() {

  OSCBundle bundleIN;
  int size;


  // first SLIPSerial.available checks if serial port is taking data
  if(SLIPSerial.available()){
    Serial.print("Message received");
    // wait for the end of the packet to be received
    while(!SLIPSerial.endofPacket())
      // read the packet that was received
      if( (size =SLIPSerial.available()) > 0)
      {
         while(size--)
            bundleIN.fill(SLIPSerial.read());
       }
  }

  if(!bundleIN.hasError())
   bundleIN.dispatch("/GPS", PrintBund);

}
