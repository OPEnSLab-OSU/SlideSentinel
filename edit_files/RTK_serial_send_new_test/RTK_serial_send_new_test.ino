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
#define SERVER_ADDRESS 21   // equal to RTK_SERVER in Hub code and node code
#define HUB_ADDRESS 20
#define VBATPIN A7
#define RF95_FREQ 915.0     // Change to 434.0 or other frequency, must match RX's freq!

// RX pin 11, TX pin 10, configuring for UART
Uart Serial2 (&sercom1, 11, 10, SERCOM_RX_PAD_0, UART_TX_PAD_2);
void SERCOM1_Handler()
{
  Serial2.IrqHandler();
}

RH_RF95 rf95(RFM95_CS, RFM95_INT);                  // instance of LoRA
RHReliableDatagram manager(rf95, SERVER_ADDRESS);   // LoRa message verification

//define string for reading RTK data
uint8_t RTKString[RH_RF95_MAX_MESSAGE_LEN*10];
int len;
int chars_to_send;
int first_index;
int last_payload;
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
  
  // Enum constant for setting bit rate options, constant configured for hight bitrate, short range
  // Bw = 500 KHz, Cr = 4/5, Sf = 128 chips/symbol
  rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128); 
  len = 0;

}

void loop()
{
  //don't set any timers before these, they wait for a significant period of time
  serialWaitIdle();               // flush the buffer and wait until current RTK string has been read out
  while(!Serial2.available()){}   // wait for nex RTK string to arrive at the serial port

  
  timer_10 = millis();
  while (millis()-timer_10 < 1000){ // 50ms timeout
    
    if(Serial2.available()){
        RTKString[len] = Serial2.read();
        len++;
        //timer_10 = millis();
    }
    if (len >= 10*RH_RF95_MAX_MESSAGE_LEN) {
      break;
    }
  }

  #if DEBUG 
  // show the result of the last read from serial along with the payload length
  // payload length should be equal to the value represented by bytes 2,3 in the RTK string preceeding it
  // string should look like A0,A1,01,C0,...,D,A
  // A0,A1 is the start of sequence delimeter
  // 01,C0 is the VARIABLE payload length, in this example "Last Payload In = 448" 448d==1C0h
  // D,A is the end of sequence delimeter

  // If all of these parts are not present or matching, the RTK string is not being properly read from serial
  // Likely because the code is blocking while sending the string vis LoRa, need higher bitrate or to accept all RTK data wont be transmitted
  
  for(int i = 0; i < len; i++){
    if(i < len-1 && RTKString[i] == 0xA0 && RTKString[i+1] == 0xA1){
        Serial.println();
        Serial.print("Last Payload In = ");
        Serial.println(last_payload-7);
        last_payload = 0;
    }
      Serial.print(RTKString[i], HEX);
      Serial.print(",");
      last_payload++;
  }
  #endif

  
  
  first_index = 0;
  
  while(len-first_index > 0) {

    if(len-first_index > RH_RF95_MAX_MESSAGE_LEN){
      chars_to_send = RH_RF95_MAX_MESSAGE_LEN;
    }
    else{
      chars_to_send = len-first_index;
    } 
    
    if(manager.sendtoWait(&(RTKString[first_index]),chars_to_send, RH_BROADCAST_ADDRESS)){
        Serial.println();
        Serial.println("MESSAGE SENT");
    }
  
    bytes_sent += chars_to_send;
    first_index=first_index+chars_to_send;
#if DEBUG == 1
    for (int i = first_index; i < first_index+chars_to_send; i++){
      Serial.print(RTKString[i], HEX);
      Serial.print(",");
    }

    Serial.println();

    Serial.print("Bytes sent = ");
    Serial.println(bytes_sent);


    Serial.print("length = "); Serial.println(chars_to_send);
    Serial.println("Sending to rf95_remote (rover station)");
#endif
  }
  len = 0;
}

// Flush serial and wait until there is no serial for a short period. This ensures reading from the beginning of a RTK string
void serialWaitIdle(){
  byte flushchar;
  unsigned long idle_time = millis();
  Serial2.flush();
  while(millis()-idle_time <= 2){
    if(Serial2.available()){
      idle_time = millis();
      flushchar = Serial2.read();
    }
  }
  Serial2.flush();
}



