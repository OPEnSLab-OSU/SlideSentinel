/*******************************************************
 * Program: RTK_serial_read
 * Description: Rover RTK receive and NMEA reading/parsing
 *
 * Application notes:
 * RTK data rate must be high for RTK to function, at least 1 kBps
 * Serial NMEA reading is performed with tinyGPS++ library
 * RTK binary data transmission and receipt performed manually
 * RTK float position data measured
 * Position data is relative to other measurements of the same type
*********************************************************/


// for MO, create new UART SERCOM on Serial2
// See https://learn.adafruit.com/using-atsamd21-sercom-to-add-more-spi-i2c-serial-ports/creating-a-new-serial

#include <Arduino.h>   // required before wiring_private.h
#include <RH_RF95.h> // Important Example code found at https://learn.adafruit.com/adafruit-rfm69hcw-and-rfm96-rfm95-rfm98-lora-packet-padio-breakouts/rfm9x-test
#include "wiring_private.h" // pinPeripheral() function
#include <RHReliableDatagram.h>

//print statements for debugging
#define DEBUG 0
#define TIMEOUT 500 //LoRa timeout in ms

//===== LoRa Initializations =====
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define CAPACITY 500
#define SERVER_ADDRESS 12
#define LORA_HUB_ADDRESS 1
#define VBATPIN A7
#define RF95_FREQ 915.0  // Change to 434.0 or other frequency, must match RX's freq!

struct nmeaString {
  char data[100];
  short int len;
};


// RX pin 13, TX pin 10, configuring for rover UART
Uart Serial2 (&sercom1, 13, 10, SERCOM_RX_PAD_1, UART_TX_PAD_2);
void SERCOM1_Handler()
{
  Serial2.IrqHandler();
}

/***********************IMPORTANT********************************/
// to use SERCOM5, comment out SERCOM5 definitions in arduinoCore-samd/variant/arduino-zero/variant.cpp
// see https://learn.adafruit.com/using-atsamd21-sercom-to-add-more-spi-i2c-serial-ports/muxing-it-up
// section: SERCOM 5
/****************************************************************/

// RX pin 6, TX pin A5, configuring for rover to GPS UART
Uart Serial3 (&sercom5, 6, A5, SERCOM_RX_PAD_2, UART_TX_PAD_0);
void SERCOM5_Handler()
{
  Serial3.IrqHandler();
}

RH_RF95 rf95(RFM95_CS, RFM95_INT);  //instance of LoRA
RHReliableDatagram manager(rf95, SERVER_ADDRESS);  //LoRa message verification

//define string for reading RTK data
uint8_t RTKString[RH_RF95_MAX_MESSAGE_LEN];
uint8_t len;
struct nmeaString input_string;
struct nmeaString last_string;
unsigned long bytes_recvd, message_timer, buff_len, psti30_timer, psti32_timer;
char nmeaChar;
bool lastUpdated;

void setup()
{
  //setup code here, to run once:

  
  // Open the main serial port to communicate with the computer
  Serial.begin(115200);
  
  // pins 13(rx), 10(tx)
  Serial2_setup();

  // pins 6(rx), A5(tx)
  Serial3_setup();

  // lora pinmodes and manager initialization
  // pins 8, 4, 3
  lora_setup();

  set_globals();
}


void loop()
{
  len = RH_RF95_MAX_MESSAGE_LEN; //when passing recv, len is max message size, then updated to size of meassage received
  message_timer = millis();
  //  if (rf95.waitAvailableTimeout(100))

  readNMEA(); // blocks until lora is received or 500 ms timeout occurs
  recvLORA(); // non-blocking, reads a single lora string

  
}


/*****************************************************
* read nmea data from serial port 3 when there is no lora transmission to receive
* will send NMEA to base if particular string has been read
*****************************************************/
void readNMEA(){
    while (!rf95.available()) {
        while (Serial3.available() && !rf95.available()) { //read from serial
            nmeaChar = Serial3.read();
            loadString(&input_string, &last_string, nmeaChar, &lastUpdated);
            if (lastUpdated) {
                if(millis() - psti30_timer > 5000){
                    if(trySend30(last_string)){
                        psti30_timer = millis();
                    }
                }
                if(millis() - psti32_timer > 5000){
                    if(trySend32(last_string)){
                        psti32_timer = millis();
                    }
                }
                lastUpdated = false;
            }
        }
        if (millis() - message_timer > TIMEOUT) {
            break;
        }
    }
}

/*****************************************************
* receive transmission from lora and write it to the serial port
* need to rewrite with RHRelibleDatagram so only messages from base written to serial port
*****************************************************/
void recvLORA(){
  if (rf95.available()) {
    if (rf95.recv(RTKString, &len)) {
      bytes_recvd += len;
      Serial2.write(RTKString, len);
      #if DEBUG
      Serial.println();
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC); // prints RSSI as decimal value
      Serial.println();
      for (int j = 0; j < len; j++) {
              if(RTKString[j] == 0xA0 && RTKString[j+1] == 0xA1){
                  Serial.println();
                  Serial.print("Buff Len: ");
                  Serial.println(buff_len - 7);
                  buff_len = 0;
              }
              buff_len++;
              Serial.print(RTKString[j], HEX);
              Serial.print(",");
      }
      #endif
    }
    else //happens when there is a receiver but bad message
    {
      Serial.println("Receive failed");
    }
  }
  else //happens when there is no receiver on the same freq to listen to
  {
#if DEBUG
    Serial.println("No reply, is there a listener around?");
#endif
  }
}

/*****************************************************
* load next character into nmeaString struct
* write new string to last_string struct if input_string is complete
*****************************************************/

void loadString(struct nmeaString* input_string, struct nmeaString* last_string, char nmeaChar, bool* lastUpdated) {
  if (nmeaChar == '$' || input_string->len >= 100) {
    for (int i = 0; i < input_string->len; i++) {
      last_string->data[i] = input_string->data[i];
    }
    last_string->len = input_string->len;
    input_string->len = 0;
    *lastUpdated = true;
  }
  input_string->data[input_string->len] = nmeaChar;
  input_string->len += 1;
}

/*****************************************************
* send the newest nmea string for type PSTI 30
* string is minimal 3D GPS fix info
*****************************************************/

bool trySend30(struct nmeaString last_string) {
  if (!strncmp(last_string.data, "$PSTI,030", 9)) {
    if (manager.sendtoWait((uint8_t*)last_string.data, last_string.len, LORA_HUB_ADDRESS)) {
      Serial.print("Sent bundle through LoRa!\nData: ");
      Serial.write(last_string.data, last_string.len);
      return 1;
    }
    else {
      Serial.print("Failed to send bundle!\nData: ");
      Serial.write(last_string.data, last_string.len);
      return 0;
    }
  }
  return 0;
}


/*****************************************************
* send the newest nmea string for type PSTI 32
* string is RTK information
*****************************************************/

bool trySend32(struct nmeaString last_string) {
  if (!strncmp(last_string.data, "$PSTI,032", 9)) {
    if (manager.sendtoWait((uint8_t*)last_string.data, last_string.len, LORA_HUB_ADDRESS)) {
      Serial.print("Sent bundle through LoRa!\nData: ");
      Serial.write(last_string.data, last_string.len);
      return 1;
    }
    else {
      Serial.print("Failed to send bundle!\nData: ");
      Serial.write(last_string.data, last_string.len);
      return 0;
    }
  }
  return 0;
}

void Serial2_setup(){
    Serial2.begin(56700);           //rx on rover to pin 10
    // Assign pins 10 & 13 SERCOM functionality, internal function
    pinPeripheral(10, PIO_SERCOM);  //Private functions for serial communication
    pinPeripheral(13, PIO_SERCOM);
}

void Serial3_setup(){
    Serial3.begin(115200);          //tx from rover to pin 6
    
    // Assign pins 6 & A5 SERCOM functionality, internal function
    pinPeripheral(6, PIO_SERCOM);  //Private functions for serial communication
    pinPeripheral(A5, PIO_SERCOM_ALT);
}

void lora_setup(){
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  /*manually reset LoRa*/
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

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
  // you can set transmitter powers from 5 to 20 dBm:
  rf95.setTxPower(20, false);
  
  //Enum constant for setting bit rate options, constant configured for high bitrate, short range
  rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128); 
}

void set_globals(){
    bytes_recvd = 0;
    psti30_timer = millis(); 
    psti32_timer = millis();
    input_string.len = 0;
    last_string.len = 0;
    lastUpdated = false;
}



