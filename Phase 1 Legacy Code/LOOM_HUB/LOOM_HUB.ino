// ================================================================
// ===              INCLUDE CONFIGURATION FILE                  ===
// ===    INCLUDE DECLARATIONS, STRUCTS, AND FUNCTIONS FROM     ===
// ===            OTHER FILES AS SET IN CONFIG.H                ===
// ================================================================

// ********************** PROGRAM DEXCRIPTION *********************
// The LOOM_HUB acts as the data manager for the Slide Sentinel project
// It receives and handles uploads of all nmea and accelerometer data,
//  it does not handle any data going to the rovers, but will take
//  data from all nodes, determine the "best" nmea data and upload it
//  in a dump to cellular periodically. Transmission is handled by 
//  a second microprocessor on board the hub which is dedicated to sending 
//  complete RTK transmissions.
// ****************************************************************


// Config has to be first has it hold all user specified options
#include "config.h"

// Preamble includes any relevant subroutine files based
// on options specified in the above config
#include "loom_preamble.h"

#include <Arduino.h>   // required before wiring_private.h
#include <RH_RF95.h>   // Important Example code found at https://learn.adafruit.com/adafruit-rfm69hcw-and-rfm96-rfm95-rfm98-lora-packet-padio-breakouts/rfm9x-test
#include "wiring_private.h" // pinPeripheral() function


#define BAUD 57600    // reading and writing occurs at 
#define DEBUG 1       // turn on debug mode
#define DEBUG_SD 1
#define CELLULAR 0

//===== LoRa Initializations =====
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define CAPACITY 500
#define VBATPIN A7
#define RF95_FREQ 915.0   // Change to 434.0 or other frequency, must match RX's freq!
#define MAX_LEN 200
#define GSM_TIMEOUT 10    // Timeout to upload all selected strings in minutes
#define N_NODES 20
#define RTK_SERVER 21
#define FILENAME_LENGTH 20
#define UPLOAD_INTERVAL 5  // A`mmount of time between dumps of best NMEA strings
//define string for reading RTK data
uint8_t RTKString[RH_RF95_MAX_MESSAGE_LEN * 5];
uint8_t nmea_len;
uint8_t rec_from;
uint8_t rec_to;
uint8_t rec_id;
uint8_t rec_flags;

char nmeaString[RH_RF95_MAX_MESSAGE_LEN];

const int SERVER_ADDRESS = N_NODES;  // nodes must have defined server address between 0 and N_NODES (not including N_NODES)
short int nodes[N_NODES];

int len;
int chars_to_send;
int first_index;
int last_payload;

bool is_read = false;

unsigned long bytes_sent;
unsigned long generic_timer;
unsigned long bndl_time;

// ================================================================
// ===                           SETUP                          ===
// ================================================================
void setup()
{
  // LOOM_begin calls any relevant (based on config) LOOM device setup functions
  Loom_begin();
  //setup code here, to run once:
  Serial.begin(115200);         //Opens the main serial port to communicate with the computer
  
  startLora();
  len = 0;
  bndl_time = millis();
  setupSDDirs();
  memset(nodes, -1, N_NODES);
  //Any custom setup code
}

// ================================================================
// ===                        MAIN LOOP                         ===
// ================================================================
void loop()
{



  generic_timer = millis();

  first_index = 0;

  nmea_len = MAX_LEN;  
  if(manager.available()){
    memset(nmeaString, 0, MAX_LEN);
  }
  if (manager.recvfromAck((uint8_t*)nmeaString, &nmea_len, &rec_from, &rec_to, &rec_id, &rec_flags)) {
    if(rec_from != RTK_SERVER){
      saveString(nmeaString, rec_from);
  #if CELLULAR
      if(millis() - bndl_time > GSM_TIMEOUT * 1000 * 60){
        pushNodesReset(nodes);
        bndl_time = millis();
      }
  #endif //CELLULAR
    }
  }
  additional_loop_checks();
} // End loop section


void pushString(char* nmea, uint8_t string_len) {
  if (generic_timer - bndl_time > GSM_TIMEOUT * 1000) {
    Serial.println("Received string");
    Serial.print("String length: ");
    Serial.print(string_len);
    for (int i = 0; i < string_len; i++) {
      Serial.print(nmea[i]);
      if (i >= MAX_LEN - 1) break;
    }
    if (string_len < MAX_LEN) {
      nmea[string_len - 4] = '\0';
    }
    else {
      nmea[MAX_LEN] = '\0';
    }
#if CELLULAR
    if (generic_timer - bndl_time > GSM_TIMEOUT * 1000) {
      OSCBundle bndl;
      OSCMessage msg;
      if (nmea[0] == '$') {
        sprintf(tab_id, "SS_GPS");
        msg.setAddress("/nmea");
        msg.add("NMEA_Data").add(nmea);
      }
      else {
        sprintf(tab_id, "SS_ACCEL");
        msg.setAddress("/accel");
        msg.add("ACCEL_Data").add(nmea);
      }
      bndl.add(msg);
      Serial.println("Added message to bundle");
      log_bundle(&bndl, PUSHINGBOX);
      bndl_time = millis();
      Serial.println("OSCMessage: ");
      print_bundle(&bndl);
    }
#endif //CELLULAR
    bndl_time = millis();
    memset(nmea, '\0', MAX_LEN);
  }
}

//parse a nmea string and return int corresponding to quality of measurement
short int nmeaType(char* nmea, uint8_t string_len){
  int pos = 0;
  int index;
  for(index = 0; index < string_len; index++){
    if(pos == 13) break;
    if(nmea[index] == ',') pos++;
  }
  if(pos < 13){
    return -1;
  }
  switch(nmea[index]){
    case('N'):
      return -1;
    case('A'):
      return 0;
    case('D'):
      return 1;
    case('F'):
      return 2;
    case('R'):
      return 3;
    default:
      return -1;
  }
}

//format must have a place from the node number in it
void fillFilename(char* dest, const char* format, int node_number){
  memset(dest, '\0', FILENAME_LENGTH);
  sprintf(dest, format, node_number);
}

void saveString(char* nmeaString, int rec_from){
  char filename[FILENAME_LENGTH];
  bool stringIsValid = false;
  if(nmeaString[0] == '$'){
    fillFilename(filename, "GDMP/%d", rec_from);
    for(int i = 0; i < MAX_LEN-1; i++){
      if(nmeaString[i] == '*'){
        memset(nmeaString + i + 3, 0, MAX_LEN - i - 3);
        stringIsValid = true;
        break;
      }
    }
  }
  else{
    fillFilename(filename, "ADMP/%d", rec_from);
    stringIsValid = true;
  }
  if(stringIsValid){
    sd_save_elem <char*> (filename, nmeaString, '\n');
    short int file_type = nmeaType(nmeaString, nmea_len);
    if(file_type != -1 && nodes[rec_from % N_NODES] <= file_type){
      fillFilename(filename, "GSEL/%d", rec_from);
      sd_delete_file(filename);
      sd_save_elem <char*> (filename, nmeaString, '\n');
      nodes[rec_from % N_NODES] = file_type;
    }
  }
}

void sendSelectedStrings(char* nmea, uint8_t string_len) {
  if (generic_timer - bndl_time > UPLOAD_INTERVAL * 1000 * 60) {
    Serial.println("Received string");
    Serial.print("String length: ");
    Serial.print(string_len);
    for (int i = 0; i < string_len; i++) {
      Serial.print(nmea[i]);
      if (i >= MAX_LEN - 1) break;
    }
    if (string_len < MAX_LEN) {
      nmea[string_len - 4] = '\0';
    }
    else {
      nmea[MAX_LEN] = '\0';
    }
#if CELLULAR
    if (generic_timer - bndl_time > GSM_TIMEOUT * 1000) {
      OSCBundle bndl;
      OSCMessage msg;
      if (nmea[0] == '$') {
        sprintf(tab_id, "SS_GPS");
        msg.setAddress("/nmea");
        msg.add("NMEA_Data").add(nmea);
      }
      else {
        sprintf(tab_id, "SS_ACCEL");
        msg.setAddress("/accel");
        msg.add("ACCEL_Data").add(nmea);
      }
      bndl.add(msg);
      Serial.println("Added message to bundle");
      log_bundle(&bndl, PUSHINGBOX);
      bndl_time = millis();
      Serial.println("OSCMessage: ");
      print_bundle(&bndl);
    }
#endif //CELLULAR
    bndl_time = millis();
    memset(nmea, '\0', MAX_LEN);
  }

}

void setupSDDirs(){
  if(SD.mkdir("ADMP")){
    Serial.println("ADMP directory successfully created");
  }
  else{
    Serial.println("ADMP directory failed to initialize");
  }
  if(SD.mkdir("GDMP")){
    Serial.println("GDMP directory successfully created");
  }
  else{
    Serial.println("GDMP directory failed to initialize");
  }
  if(SD.mkdir("GSEL")){
    Serial.println("GSEL directory successfully created");
  }
  else{
    Serial.println("dmp directory failed to initialize");
  }
}

void pushNodesReset(short int* node_list){
  char filename[FILENAME_LENGTH];
  char file_contents[RH_RF95_MAX_MESSAGE_LEN];
  File dataFile;
  for(int i = 0; i < N_NODES; i++){
    if(node_list[i] != -1){
      memset(file_contents, '\0', RH_RF95_MAX_MESSAGE_LEN);
      // open the file. note that only one file can be open at a time,
      // so you have to close this one before opening another.
      fillFilename(filename, "GSEL/%d", i);
      node_list[i] = -1;
      dataFile = SD.open(filename);

      // if the file is available, write to it:
      if (dataFile) {
        int j = 0;
        while (dataFile.available()) {
          file_contents[j++] = dataFile.read();
          if(j >= RH_RF95_MAX_MESSAGE_LEN) break;
        }
        dataFile.close();
        pushString(file_contents, j);
      }
      // if the file isn't open, pop up an error:
      else {
        Serial.println("error opening datalog.txt");
      }
      SD.remove(filename);
    }
  }
}

void startLora(){
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
  rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128); //Enum constant for setting bit rate options, constant configured for high bitrate, short range
}


