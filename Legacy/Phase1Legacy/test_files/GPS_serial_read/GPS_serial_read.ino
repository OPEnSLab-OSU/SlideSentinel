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
#include "wiring_private.h" // pinPeripheral() function

#define BAUD 115200    // 115200 for GPS rover configuration, 57600 for base configuration (no NMEA)

// RX pin 11, TX pin 10
Uart Serial2 (&sercom1, 11, 10, SERCOM_RX_PAD_0, UART_TX_PAD_2);


void SERCOM1_Handler()
{
  Serial2.IrqHandler();SERCOM1_Handler //?? only place used in the template file, don't know what it is
}



void setup() 
{
  //setup code here, to run once:
  Serial.begin(9600);         //Opens the main serial port to communicate with the computer
  Serial2.begin(BAUD);        //rate of serial communication via UART
  
  // Assign pins 10 & 11 SERCOM functionality, internal function
  
  pinPeripheral(10, PIO_SERCOM);  //Private functions for serial communication
  pinPeripheral(11, PIO_SERCOM);
  
  // K_30_Serial.begin(9600);    //Opens the virtual serial port with a baud of 9600
}

void loop() 
{
  
  char nmea_code [83] = {0};     //NMEA 0183 gurantees max length = 82, all code chars are initialized null
  int i = 0;                     //counts number of characters read
  sendRequest(nmea_code);

  
  if(nmea_code[0] != 0){
    Serial.print("NMEA 0183 = ");
  }

  while(nmea_code[i] != 0){
    Serial.print(nmea_code[i++]);  
  }  
  
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
