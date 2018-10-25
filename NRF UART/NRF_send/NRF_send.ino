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
#include "wiring_private.h" // pinPeripheral() function


#include <Arduino.h>   // required before wiring_private.h


#define BAUD 57600    // reading and writing occurs at 
#define DEBUG 1       // turn on debug mode
#define DEBUG_RTK 0   // debug rtk corrections, lot of character output
#define DEBUG_LORA 0
#define CELLULAR 1

//===== LoRa initial_messageizations =====
//#define RFM95_CS 8
//#define RFM95_RST 4
//#define RFM95_INT 3
#define CAPACITY 500
#define SERVER_ADDRESS 1
#define VBATPIN A7
#define RF95_FREQ 915.0  // Change to 434.0 or other frequency, must match RX's freq!
#define MAX_LEN 200
#define NRF_NODE_ADDRESS 15
#define MAX_RTK_SIZE 1921

#define TIMEOUT 500

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


uint8_t RTKString[MAX_RTK_SIZE];
uint8_t len;
int loop_counter = 0;
struct nmeaString input_string;
struct nmeaString last_string;
unsigned long bytes_recvd, message_timer, buff_len, psti30_timer, psti32_timer;
char nmeaChar;
bool lastUpdated;
// ================================================================ 
// ===                           SETUP                          ===
// ================================================================ 

void setup() 
{
	//setup code here, to run once:

  
  // Open the main serial port to communicate with the computer
  Serial.begin(115200);
  
  // pins 13(rx), 10(tx)
  Serial2_setup();

  // pins 6(rx), A5(tx)
  Serial3_setup();

  set_globals();

  while(!Serial){}
  Serial.println("begin");
  message_timer = millis();
}

// ================================================================ 
// ===                        MAIN LOOP                         ===
// ================================================================ 
void loop()
{
  if(millis()-message_timer > 2000){
    sendNRF2("hello world", strlen("hello world"));
    Serial.print("loop ");
    Serial.println(loop_counter++);
    message_timer = millis();
  }
}


/*****************************************************
* read nmea data from serial port 3 when there is no lora transmission to receive
* will send NMEA to base if particular string has been read
*****************************************************/
//void readNMEA(){
//    while (!rf95.available()) {
//        while (Serial3.available() && !rf95.available()) { //read from serial
//            nmeaChar = Serial3.read();
//            loadString(&input_string, &last_string, nmeaChar, &lastUpdated);
//            if (lastUpdated) {
//                if(millis() - psti30_timer > 5000){
//                    if(trySend30(last_string)){
//                        psti30_timer = millis();
//                    }
//                }
//                if(millis() - psti32_timer > 5000){
//                    if(trySend32(last_string)){
//                        psti32_timer = millis();
//                    }
//                }
//                lastUpdated = false;
//            }
//        }
//        if (millis() - message_timer > TIMEOUT) {
//            break;
//        }
//    }
//}

void recvNRF(char * string_to_fill, int *len){
  network.update();                      // Check the network regularly
  short int message_num;
  short int total_messages;
  *len = 0;
  int total_chars = 0;
  char message[NRF_MESSAGE_SIZE];
  memset(string_to_fill, '\0', 15*NRF_MESSAGE_SIZE);
  RF24NetworkHeader header;          // If so, grab it and print it out
  if(network.available()){
    char initial_message[3];
    network.read(header,&initial_message,3);
    message_num = int(initial_message[0]);
    total_messages = int(initial_message[1]);
  }
  
  unsigned long temporary_timer = millis();
  while (message_num != total_messages) {        // Is there anything ready for us?
    memset(message, '\0', NRF_MESSAGE_SIZE);
    network.read(header,&message,NRF_MESSAGE_SIZE-1); //read next section of the message
    int chars_recvd = strlen(message) - 2;
    strncpy(string_to_fill+total_chars, message + 2, chars_recvd); //copy message into the longer string
    total_chars += chars_recvd;
    len += chars_recvd;
    message_num = int(message[0]);
    if(millis() - temporary_timer > TIMEOUT){
      memset(string_to_fill, '\0', 15*NRF_MESSAGE_SIZE);
      return;
    }
  }
  Serial.print("received string:");
  Serial.println(string_to_fill);
}

void sendNRF(char * string_to_send, int send_len){
  int n_messages = send_len / (NRF_MESSAGE_SIZE - 3) + 1;
  int message_num = 0;
  int chars_to_send;
  int chars_sent = 0;

  char message[NRF_MESSAGE_SIZE];
  RF24NetworkHeader header(NRF_NODE_ADDRESS, 'A');              // This should be better generalized, as to be able to send to nodes


  char initial_message[3];
  memset(initial_message, '\0', 3);
  initial_message[0] = message_num;
  initial_message[1] = n_messages;
  Serial.println("DEBUG 1");
  network.write(header, initial_message, 3);
  Serial.println("DEBUG 2");
  
  do{
    if(send_len > NRF_MESSAGE_SIZE - 3){
      chars_to_send = NRF_MESSAGE_SIZE - 3;
    }
    else chars_to_send = send_len;
    
    memset(message, '\0', NRF_MESSAGE_SIZE);

    message[0] = ++message_num;
    message[1] = n_messages;
    strncpy(message, string_to_send+chars_sent, chars_to_send);

    bool is_sent = network.write(header, message, chars_to_send+3);

    #if LOOM_DEBUG == 1
      if (is_sent) Serial.println("NRF Packet Send Suceeded!");
      else       Serial.println("NRF Packet Send Failed!");
    #endif

    chars_sent += chars_to_send;
    send_len -= chars_to_send;
  }while(send_len > 0);
}

void sendNRF2(char * string_to_send, int send_len){
  // Max buffer size is 1920
  // IMPORTANT: Must edit the line "#define MAIN_BUFFER_SIZE 1920 + 10" in RF24Network_config.h
  if(send_len > 1920){
    send_len = 1920;
  }
  RF24NetworkHeader header(NRF_NODE_ADDRESS, 'A');
  bool is_sent = network.write(header, string_to_send, send_len);

    #if LOOM_DEBUG == 1
      if (is_sent) Serial.println("NRF Packet Send Suceeded!");
      else       Serial.println("NRF Packet Send Failed!");
    #endif
}

void recvNRF2(char * recv_buffer, int chars_available){
  memset(recv_buffer, '\0', chars_available);
  RF24NetworkHeader header;
  network.read(header, &recv_buffer, chars_available-1);  
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

//void lora_setup(){
//  pinMode(RFM95_RST, OUTPUT);
//  digitalWrite(RFM95_RST, HIGH);
//
//  /*manually reset LoRa*/
//  digitalWrite(RFM95_RST, LOW);
//  delay(10);
//  digitalWrite(RFM95_RST, HIGH);
//  delay(10);
//
//  /*check LoRa device and set frequency*/
//  while (!manager.init()) {
//#if DEBUG == 1
//    Serial.println("LoRa manager init failed"); //if print wiring may be wrong
//#endif
//    while (1);
//  }
//#if DEBUG == 1
//  Serial.println("LoRa radio init OK!");
//#endif
//  // checks if frequency is initialized
//  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
//  if (!rf95.setFrequency(RF95_FREQ)) {
//#if DEBUG == 1
//    Serial.println("setFrequency failed");
//#endif
//    while (1);
//  }
//#if DEBUG == 1
//  Serial.print("Set Freq to: ");
//  Serial.println(RF95_FREQ);
//#endif
//
//  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips / symbol, CRC on
//  // The default transmitter power is 13dBm, using PA_BOOST.
//  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
//  // you can set transmitter powers from 5 to 20 dBm:
//  rf95.setTxPower(20, false);
//  
//  //Enum constant for setting bit rate options, constant configured for high bitrate, short range
//  rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128); 
//}

void set_globals(){
    bytes_recvd = 0;
    psti30_timer = millis(); 
    psti32_timer = millis();
    input_string.len = 0;
    last_string.len = 0;
    lastUpdated = false;
}



