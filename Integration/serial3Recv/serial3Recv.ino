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


void PrintBund(OSCMessage &msg)
{
    print_message(&msg);
}

void setup()
{
  Serial.begin(115200);
  Serial3.begin(115200);
  pinPeripheral(SERIAL3_TX, PIO_SERCOM);  //Private functions for serial communication
  pinPeripheral(SERIAL3_RX, PIO_SERCOM_ALT);
  while(!Serial){}
  Serial.println("Start");
}

void loop() {

  OSCBundle bundleIN;
  int size;
  char input;
  
  if(Serial3.available()){
    Serial.println("Reading data");
    if(input = Serial3.read() == (byte) 2){
      while(input != (byte) 4){
        if(Serial3.available()){
          input = Serial3.read();
          if(input == 4) break;
          bundleIN.fill(input);
        }
      }
    }
  }
  
  if(!bundleIN.hasError())
  bundleIN.dispatch("/GPS", PrintBund);
  else{
    Serial.print("ERROR: ");
    Serial.println(bundleIN.getError());
  }
}
