// ================================================================
// ===              INCLUDE CONFIGURATION FILE                  ===
// ===    INCLUDE DECLARATIONS, STRUCTS, AND FUNCTIONS FROM     ===
// ===            OTHER FILES AS SET IN CONFIG.H                ===
// ================================================================

// Config has to be first has it hold all user specified options
#include "config.h"

// Preamble includes any relevant subroutine files based 
// on options specified in the above config
#include "loom_preamble.h"

#include <Arduino.h>   // required before wiring_private.h
#include <RH_RF95.h> // Important Example code found at https://learn.adafruit.com/adafruit-rfm69hcw-and-rfm96-rfm95-rfm98-lora-packet-padio-breakouts/rfm9x-test
#include "wiring_private.h" // pinPeripheral() function


#define BAUD 57600    // reading and writing occurs at 
#define DEBUG 0       // turn on debug mode

//===== LoRa Initializations =====
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define CAPACITY 500
#define SERVER_ADDRESS 1
#define VBATPIN A7
#define RF95_FREQ 915.0  // Change to 434.0 or other frequency, must match RX's freq!

// RX pin 11, TX pin 10, configuring for UART
Uart Serial2 (&sercom1, 11, 10, SERCOM_RX_PAD_0, UART_TX_PAD_2);
void SERCOM1_Handler()
{
  Serial2.IrqHandler();
}

//define string for reading RTK data
uint8_t RTKString[RH_RF95_MAX_MESSAGE_LEN*5];
uint8_t dummyString[250];
char nmeaString[100];
uint8_t nmea_len;
int len;
int chars_to_send;
int first_index;
int last_payload;
uint8_t rec_from, rec_to, rec_id, rec_flags;
bool is_read = false;
unsigned long bytes_sent, timer_10;
// ================================================================ 
// ===                           SETUP                          ===
// ================================================================ 

void setup() 
{
	// LOOM_begin calls any relevant (based on config) LOOM device setup functions
	Loom_begin();	
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
  rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128); //Enum constant for setting bit rate options, constant configured for hight bitrate, short range
  len = 0;
	// Any custom setup code
}

// ================================================================ 
// ===                        MAIN LOOP                         ===
// ================================================================ 
void loop() 
{
  OSCMessage msg;
  msg.add(String())
  sendToPushingBox(&msg);
  
	additional_loop_checks();
  timer_10 = millis();
  while (millis()-timer_10 < 5){ //50ms timeout
    
    if(Serial2.available()){
        RTKString[len] = Serial2.read();
        len++;
        timer_10 = millis();
    }
    if (len >= 5*RH_RF95_MAX_MESSAGE_LEN) {
      break;
    }
  }
  /***********TEST CODE************/
  for(int i = 0; i < len; i++){
    if(i < len-1 && RTKString[i] == 0xA0 && RTKString[i+1] == 0xA1){
        Serial.println();
        Serial.print("Last String In: ");
        Serial.println(last_payload-7);
        last_payload = 0;
    }
      Serial.print(RTKString[i], HEX);
      Serial.print(",");
      last_payload++;
  }
  /***********************/
  
  
    first_index = 0;
    nmea_len = 100;
    if(manager.recvfromAck((uint8_t*)nmeaString, &nmea_len, &rec_from, &rec_to, &rec_id, &rec_flags)){
        Serial.println("Received nmea string");
        for(int i = 0; i<nmea_len; i++){
            Serial.print(nmeaString[i]);
        }
    }
  
  while(len-first_index > 0) {
    nmea_len = 100;
    if(manager.recvfromAck((uint8_t*)nmeaString, &nmea_len, &rec_from, &rec_to, &rec_id, &rec_flags)){
        Serial.println("Received nmea string");
        for(int i = 0; i<nmea_len; i++){
            Serial.print(nmeaString[i]);
        }
    }
    
    if(len-first_index > RH_RF95_MAX_MESSAGE_LEN){
      chars_to_send = RH_RF95_MAX_MESSAGE_LEN;
    }
    else{
      chars_to_send = len-first_index;
    } 
    
    if(rf95.send(&(RTKString[first_index]),chars_to_send)){
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
  
  len = len - first_index;
  for(int i = 0; i < len; i++){
      RTKString[i] = RTKString[i+first_index];
  }	
  
} // End loop section









