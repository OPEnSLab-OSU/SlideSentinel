/*
    Functions for reading GPS RTK corrections from base and transmitting them to rover
    **Will need to include rover to base NMEA receipt in future

  /*
  // For Arduino Uno and 32u4 use softSerial
  #include "SoftwareSerial.h"

  SoftwareSerial K_30_Serial(10,11);  //Sets up a virtual serial port
                                    //Using pin 12 for Rx and pin 13 for Tx
*/

// for MO, create new UART SERCOM on Serial2
// See https://learn.adafruit.com/using-atsamd21-sercom-to-add-more-spi-i2c-serial-ports/creating-a-new-serial

#include <Arduino.h>   // required before wiring_private.h
#include <RH_RF95.h> // Important Example code found at https://learn.adafruit.com/adafruit-rfm69hcw-and-rfm96-rfm95-rfm98-lora-packet-padio-breakouts/rfm9x-test
#include "wiring_private.h" // pinPeripheral() function
#include <RHReliableDatagram.h>

#define BAUD 57600    // reading and writing occurs at 
#define DEBUG 0       // turn on debug mode

//===== LoRa Initializations =====
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define CAPACITY 500
#define SERVER_ADDRESS 2
#define VBATPIN A7
#define RF95_FREQ 915.0  // Change to 434.0 or other frequency, must match RX's freq!

// RX pin 11, TX pin 10, configuring for UART
Uart Serial2 (&sercom1, 11, 10, SERCOM_RX_PAD_0, UART_TX_PAD_2);
void SERCOM1_Handler()
{
  Serial2.IrqHandler();
}

RH_RF95 rf95(RFM95_CS, RFM95_INT);  //instance of LoRA
RHReliableDatagram manager(rf95, SERVER_ADDRESS);  //LoRa message verification

//define string for reading RTK data
uint8_t RTKString[RH_RF95_MAX_MESSAGE_LEN*5];
int len;
int chars_to_send;
int first_index;
bool is_read = false;
unsigned long bytes_sent, timer_10;


void setup()
{
  //setup code here, to run once:
  Serial.begin(115200);         //Opens the main serial port to communicate with the computer
  Serial2.begin(BAUD);        //rate of serial communication via UART


  // Assign pins 10 & 11 SERCOM functionality, internal function
  pinPeripheral(10, PIO_SERCOM);  //Private functions for serial communication
  pinPeripheral(11, PIO_SERCOM);
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
  // checks if frequency is initialized
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
  bytes_sent = 0;
#endif
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips / symbol, CRC on
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
  len = 0;

  // K_30_Serial.begin(9600);    //Opens the virtual serial port with a baud of 9600
}

void loop()
{
  timer_10 = millis();
  while (millis()-timer_10 < 5){ //5ms timeout
    
    if(Serial2.available()){
        RTKString[len] = Serial2.read();
        len++;
        timer_10 = millis();
    }
    if (len == 5*RH_RF95_MAX_MESSAGE_LEN) {
      break;
    }
  }
  first_index = 0;
  while(len-first_index > 0) {
    if(len-first_index > RH_RF95_MAX_MESSAGE_LEN){
      chars_to_send = RH_RF95_MAX_MESSAGE_LEN;
    }
    else{
      chars_to_send = len-first_index;
    }
    rf95.send(&(RTKString[first_index]), chars_to_send);
    //delay(10);
    bytes_sent += chars_to_send;
    first_index=first_index+chars_to_send;
#if DEBUG == 1
    for (int i = 0; i < len; i++)
      Serial.print(RTKString[i], HEX);

    Serial.println();

    Serial.print("Bytes sent = ");
    Serial.println(bytes_sent);


    Serial.print("length = "); Serial.println(len);
    Serial.println("Sending to rf95_remote (rover station)");
#endif
  }
  len = 0;

  //        if (rf95.waitAvailableTimeout(500))
  //        {
  //          // Should be a reply message for us now
  //          if (rf95.recv(buf, &len))
  //          {
  //      #if DEBUG == 1
  //            Serial.println("Got reply: ");
  //            Serial.println("\nData received:");
  //            Serial.println((char*)buf);
  //            Serial.print("RSSI: ");
  //            Serial.println(rf95.lastRssi(), DEC); // prints RSSI as decimal value
  //            Serial.println();
  //      #endif
  //          }
  //          else //happens when there is a receiver but bad message
  //          {
  //            Serial.println("Receive failed");
  //          }
  //        }
  //        else //happens when there is no receiver on the same freq to listen to
  //        {
  //          Serial.println("No reply, is there a listener around?");
  //        }

//    if(is_read){
//      if( lastIn == 0xA0 && nextIn == 0xA1) //start code
//        Serial.println("Sequence Start");
//  
//      Serial.print(lastIn, HEX);
//  
//      if(lastIn == 0x0D && nextIn == 0x0A){
//        Serial.print(nextIn, HEX);
//        Serial.println("");
//      }
//    }
}


