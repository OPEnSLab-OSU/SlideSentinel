/*
    Serial reading is performed with tinyGPS++ library
    RTK binary data transmission performed manually
    RTK still having issues with position fix

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
#include "TinyGPS++.h"


#define BAUD 57600    // 115200 for GPS rover configuration, 57600 for base configuration (no NMEA)
#define DEBUG 1

//===== LoRa Initializations =====
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define CAPACITY 500
#define SERVER_ADDRESS 2
#define VBATPIN A7
#define RF95_FREQ 915.0  // Change to 434.0 or other frequency, must match RX's freq!

// RX pin 13, TX pin 10, configuring for rover UART
Uart Serial2 (&sercom1, 13, 10, SERCOM_RX_PAD_1, UART_TX_PAD_2);

void SERCOM1_Handler()
{
  Serial2.IrqHandler(); 
}

// RX pin 6, TX pin A5, configuring for rover to GPS UART
/***********************IMPORTANT********************************/
// to use SERCOM5, comment out SERCOM5 definitions in feather_M0/variants.cpp
Uart Serial3 (&sercom5, 6, A5, SERCOM_RX_PAD_2, UART_TX_PAD_0);

void SERCOM5_Handler()
{
  Serial3.IrqHandler(); 
}

RH_RF95 rf95(RFM95_CS, RFM95_INT);  //instance of LoRA
RHReliableDatagram manager(rf95, SERVER_ADDRESS);  //LoRa message verification

TinyGPSPlus gps; //instance of GPS parser

//define string for reading RTK data
uint8_t RTKString[RH_RF95_MAX_MESSAGE_LEN];
uint8_t complete_string[RH_RF95_MAX_MESSAGE_LEN];
uint8_t len;
bool isRead = false, wait_write = false;
byte lastIn;
byte nextIn;
unsigned long bytes_recvd, timer;
char nmeaChar;




void setup()
{
  //setup code here, to run once:
  Serial.begin(115200);         //Opens the main serial port to communicate with the computer
  Serial2.begin(56700);        //rx from rover
  Serial3.begin(115200);         //tx to rover

  // Assign pins 10 & 13 SERCOM functionality, internal function
  pinPeripheral(10, PIO_SERCOM);  //Private functions for serial communication
  pinPeripheral(13, PIO_SERCOM);
  
  // Assign pins 6 & A5 SERCOM functionality, internal function
  pinPeripheral(6, PIO_SERCOM);  //Private functions for serial communication
  pinPeripheral(A5, PIO_SERCOM_ALT);
  
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
#endif
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips / symbol, CRC on
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);

  bytes_recvd = 0;

  // K_30_Serial.begin(9600);    //Opens the virtual serial port with a baud of 9600
}


void loop()
{
  len = RH_RF95_MAX_MESSAGE_LEN; //when passing recv, len is max message size, then updated to size of meassage received
  timer = millis();
  //  if (rf95.waitAvailableTimeout(100))
  /* code is similar to waitAvailableTimeout, but does not block 
   * this allows device to read NMEA code while waiting for RTK String from LoRa
   * while loop is equivaled to rf95.waitAvailableTimeout(500)
   */

  while (!rf95.available()) {
    while (Serial3.available()) { //read from serial
      nmeaChar=Serial3.read();
      Serial.print(nmeaChar);
      gps.encode(nmeaChar);
    }
    if(millis() - timer > 500){break;}
  }
  if (rf95.available()) {
//    if (wait_write) {
//      wait_write = false;
//      Serial2.write(RTKString, len);
//      for(int i = 0; i < len; i++)
//        Serial.print(RTKString[i], HEX);
//    }
//    if (rf95.recv(RTKString, &len))
//    { bytes_recvd += len;
//      if (RTKString[len - 2] == 0x0D && RTKString[len - 1] == 0x0A) {
//        Serial2.write(RTKString, len);
//        for(int i = 0; i < len; i++)
//          Serial.print(RTKString[i], HEX);
//        Serial.println();
//      }
//      else {
//        wait_write = true;
//      }
    if (rf95.recv(RTKString, &len))
    {   bytes_recvd += len;
        Serial2.write(RTKString, len);
        for(int i = 0; i < len; i++)
          Serial.print(RTKString[i], HEX);
        Serial.println();
      
    
#if DEBUG == 1
      Serial.print("Bytes received = ");
      Serial.println(bytes_recvd);
      Serial.print("Got reply, length = ");
      Serial.println(len, DEC);
      Serial.println("\nData received:");
      for (int i = 0; i < len; i++) {
        Serial.print(RTKString[i], HEX);
      }
      Serial.println();
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC); // prints RSSI as decimal value
      Serial.println();
#endif
    }
    else //happens when there is a receiver but bad message
    {
      Serial.println("Receive failed");
    }
  }
  else //happens when there is no receiver on the same freq to listen to
  {
    Serial.println("No reply, is there a listener around?");
  }
  #if DEBUG
  if(gps.location.isValid()){
    Serial.print("LAT=");  Serial.println(gps.location.lat(), 6);
    Serial.print("LONG="); Serial.println(gps.location.lng(), 6);
  }
  if(gps.time.isValid()){
    Serial.print("Timestring: ");
    Serial.println(gps.time.value()); // Raw time in HHMMSSCC format (u32)
  }
  #endif

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



