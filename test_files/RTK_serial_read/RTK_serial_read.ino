/*
  Basic serial nmea data read function, currently prints any valid input nmea code to serial monitor
  In future should
    -Use a checksum
    -Use a deque to store most recent nmea string
    -Read from deque when desired type of nmea string is read
    -Verify GPS signal output using  1pps output 
*/

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

#define BAUD 57600    // 115200 for GPS rover configuration, 57600 for base configuration (no NMEA)

//===== LoRa Initializations =====
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

#define SERVER_ADDRESS 2

#define VBATPIN A7

#define RF95_FREQ 915.0  // Change to 434.0 or other frequency, must match RX's freq!

// RX pin 11, TX pin 10, configuring for base UART
Uart Serial2 (&sercom1, 11, 10, SERCOM_RX_PAD_0, UART_TX_PAD_2);
void SERCOM1_Handler()
{
  Serial2.IrqHandler(); //?? only place used in the template file, don't know what it is
}

RH_RF95 rf95(RFM95_CS, RFM95_INT);  //instance of LoRA
RHReliableDatagram manager(rf95, SERVER_ADDRESS);  //LoRa message verification

//define string for reading RTK data
//char RTKString[RH_RF95_MAX_MESSAGE_LEN];
String RTKString = "";
bool isRead = false;
byte lastIn;
byte nextIn;


void setup() 
{
  //setup code here, to run once:
  Serial.begin(115200);         //Opens the main serial port to communicate with the computer
  Serial2.begin(BAUD);        //rate of serial communication via UART


  // Assign pins 10 & 11 SERCOM functionality, internal function  
  pinPeripheral(10, PIO_SERCOM);  //Private functions for serial communication
  pinPeripheral(11, PIO_SERCOM);



  
  // K_30_Serial.begin(9600);    //Opens the virtual serial port with a baud of 9600
}

void loop() 
{
  if(Serial2.available() > 1){
      nextIn = Serial2.read();
      Serial2.write(nextIn);
      Serial.write(nextIn);
    }

//  if(isRead){
//    if( lastIn == 0xA0 && nextIn == 0xA1) //start code
//      Serial.println("Sequence Start");
//      
//    Serial.print(lastIn, HEX);
//    
//    if(lastIn == 0x0D && nextIn == 0x0A){
//      Serial.print(nextIn, HEX);
//      Serial.println("");
//    }
//  }
}

void sendRequest(char * nmea_code)
{  
  int idx = 0;  //index of the code being read

  char next_in = 0;       //next character to be read
  
  while(next_in != '$'){  //read all chearcters of unfinished nmea string out of buffer
    next_in = Serial2.read();  
  } 
  
  
  while(next_in != 10){   //read until stop delimeter
    if(idx >= 81) break;
    nmea_code[idx++] = next_in;
    next_in = Serial2.read();
  }

  nmea_code[idx] = 10;     //set last character = stop delimeter
  
}

void clearNMEA(char * nmea_code){ //fill all values in a character string with null
  int idx = 0;
  while(nmea_code[idx]){
    nmea_code[idx++] = 0;
  } 
}

