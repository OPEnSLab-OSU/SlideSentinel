/*
    TinyGPS++ test code, use with gps module reading data from UART to verify the
    tinygps++ class is working as expected

    Serial monitor pirnts GPS data recieved from GPS UART, for debug purposes
*/

#include <Arduino.h>   // required before wiring_private.h
#include "TinyGPS++.h"
#include "wiring_private.h" // pinPeripheral() function


#define BAUD 115200    // 115200 for GPS rover configuration, 57600 for base configuration (no NMEA)
#define DEBUG 0

TinyGPSPlus gps; //instance of GPS parser

// RX pin 13, TX pin 10, configuring for base UART
Uart Serial2 (&sercom1, 13, 10, SERCOM_RX_PAD_1, UART_TX_PAD_2);

void SERCOM1_Handler()
{
  Serial2.IrqHandler(); //only place used in the template file, don't know what it is
}

void setup()
{
  //setup code here, to run once:
  Serial.begin(BAUD);         //Opens the main serial port to communicate with the computer
  Serial2Setup();
}



void loop()
{
    char nextIn;
    if(Serial2.available())
    {
        nextIn = Serial2.read();
        Serial.print(nextIn);
        gps.encode(nextIn);
    }
    
#if DEBUG
    if(gps.location.isValid()){
        Serial.print("LAT=");  Serial.println(gps.location.lat(), 6);
        Serial.print("LONG="); Serial.println(gps.location.lng(), 6);
    }
    if(gps.time.isValid()){
        Serial.println(gps.time.value()); // Raw time in HHMMSSCC format (u32)
    }
#endif
}

void Serial2Setup(){
  Serial2.begin(BAUD);
  
  // Assign pins 10 & 13 SERCOM functionality
  // See https://learn.adafruit.com/using-atsamd21-sercom-to-add-more-spi-i2c-serial-ports/muxing-it-up
  // This article helps describe pin muxing
  pinPeripheral(10, PIO_SERCOM);
  pinPeripheral(13, PIO_SERCOM);
}


