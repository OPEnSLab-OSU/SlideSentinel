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


void setup() {
  SLIPSerial.begin(115200);
  pinPeripheral(SERIAL3_TX, PIO_SERCOM);  //Private functions for serial communication
  pinPeripheral(SERIAL3_RX, PIO_SERCOM_ALT);
  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  String alpha = "";
  char letr = 'A';
  char buffer[30];
  for(int i = 0; i<26; i++){
    alpha += letr++;
  }
  OSCMessage data("/alpha");
  alpha.toCharArray(buffer, 30);
  data.add((const char *)buffer);
  SLIPSerial.beginPacket();
  data.send(SLIPSerial);
  SLIPSerial.endPacket();
  print_message(&data);
  delay(1000);
}
