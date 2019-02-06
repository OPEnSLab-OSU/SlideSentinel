/********************** PROGRAM DEXCRIPTION *********************
 ****************************************************************/

#include "config.h"
#include "loom_preamble.h"

#include <EnableInterrupt.h>
//#include <Arduino.h>
#include "wiring_private.h" 

#define DEBUG 1 // turn on debug mode
#define DEBUG_SD 1
#define CELLULAR 1
#define MAX_LEN 82 //NMEA0183 specification standard
#define FILENAME_LENGTH 20
#define N_NODES 1
#define GSM_TIMEOUT 10 // Timeout to upload all selected strings in minutes

void append(char *s, char c);
void readNMEA();
short int nmeaType(char *nmea, uint8_t string_len);
void fillFilename(char *dest, const char *format, int node_number);
void setupSDDirs();
void saveString(char *nmeaString, int rec_from);
void pushString(char *nmea, uint8_t string_len);
void Serial2_setup();
void readBuffer();
double GpsEncodingToDegrees(char *gpsencoding);

// ======== Serial Port Init ==========
// RX pin 13, TX pin 10, configuring for rover UART
Uart Serial2(&sercom1, 13, 10, SERCOM_RX_PAD_1, UART_TX_PAD_2);
void SERCOM1_Handler()
{
  Serial2.IrqHandler();
}

// ======== Define NMEA structure ==========
typedef struct
{
  String type;
  double lat;
  char ew;
  double lon;
  char ns;
}NMEAstruct;

// ======== Serial Port Init ==========
char nmeaString[MAX_LEN];
NMEAstruct nmeastruct;

unsigned long bytes_sent;
unsigned long generic_timer;
unsigned long bndl_time;

// ================================================================
// ===                           SETUP                          ===
// ================================================================
void setup()
{
  Serial.begin(115200); //Opens the main serial port to communicate with the computer
  Serial2_setup();     //Serial port for communicating with Freewave radios
  while (!Serial);
  Loom_begin();
  Serial.println("serial configured... ");
  bndl_time = millis();
  //setupSDDirs();
}

// ================================================================
// ===                        MAIN LOOP                         ===
// ================================================================
void loop()
{
  memset(nmeaString, '\0', sizeof(nmeaString));
  //read from FREEWAVE, note any incoming NMEA string at this point are corrected and need only be sent over the cellular link
  if (Serial2.available())
  {
    readBuffer();
    if (nmeaString[0] == '$')
    {
      Serial.print("STRING: ");
      Serial.println(nmeaString);

      //get measurement quality
      Serial.print("RTK quality: ");
      Serial.println(nmeaType(nmeaString, sizeof(nmeaString)));
      Serial.println("Saving NMEA string to SD card...");
      //saveString(nmeaString, 1);

#if CELLULAR
      Serial.println("Writing to Google Sheets...");
      pushString(nmeaString, sizeof(nmeaString) / sizeof(nmeaString[0]));
      bndl_time = millis();
      Serial.println("Done...");
#endif
    }
  }
}

double GpsEncodingToDegrees(const char *gpsencoding)
{
  double a = strtod(gpsencoding, 0);
  double d = (int)a / 100;
  a -= d * 100;
  return d + (a / 60);
}

/*
void signGPS(struct NMEAstruct nmeaStruct){
  if(nmeaStruct->ns == 'S')
    nmeaStruct->lat = nmeaStruct->lat * (-1);
  if(nmeaStruct->ew == 'W')
    nmeaStruct->lon = nmeaStruct->lon * (-1);
}*/

void pushString(char *nmea, uint8_t string_len)
{

#if CELLULAR
  OSCBundle bndl;
  Serial.println("sending message...");

  if (nmea[0] == '$')
  {
    Serial.print("NMEA string caught: ");
    Serial.println(nmea);

    uint8_t pos = 0;
    char delim[] = ",";
    char *ptr = strtok(nmea, delim);

    //populate the comma delimited nmea string
    while (ptr != NULL)
    {
      switch (pos)
      {
      case 0: //type
        nmeastruct.type = *ptr;
        break;
      case 4: //latitude
        nmeastruct.lat = GpsEncodingToDegrees((const char *)ptr);
        break;
      case 5: //N/S
        nmeastruct.ns = *ptr;
        break;
      case 6: //longitude
        nmeastruct.lon = GpsEncodingToDegrees((const char *)ptr);
        break;
      case 7: //E/W
        nmeastruct.ew = *ptr;
        break;
      }
      ptr = strtok(NULL, delim);
      pos++;
    }

    if (nmeastruct.ns == 'S')
      nmeastruct.lat = nmeastruct.lat * (-1);
    if (nmeastruct.ew == 'W')
      nmeastruct.lon = nmeastruct.lon * (-1);

    Serial.println("-----------------------------");
    Serial.print("Type: ");
    Serial.println(nmeastruct.type);
    Serial.print("Lat: ");
    Serial.println(nmeastruct.lat);
    Serial.print("EW: ");
    Serial.println(nmeastruct.ew);
    Serial.print("Long: ");
    Serial.println(nmeastruct.lon);
    Serial.print("NS: ");
    Serial.println(nmeastruct.ns);
    Serial.println("-----------------------------");

    //test string located at the OPeNs lab
    //input: $PSTI,030,205759.000,A,4434.0007514,N,12316.8327123,W,104.056,-0.02,-0.00,-0.01,040219,D,0.0,0.0*20

    //$GPGGA,092750.000,4433.99756,N,12316.85238,W,1,8,1.03,61.7,M,55.2,M,,*76
    /* for map config:
    center: 44.566626	-123.280873
    Zoom control: small
    */

    char lat[16];
    char lon[16];
    memset(lat, '\0', 16);
    memset(lon, '\0', 16);

    sprintf(lat, "%lf", nmeastruct.lat);
    sprintf(lon, "%lf", nmeastruct.lon);

    bndl.add("/Loom1/Hub");
    append_to_bundle_key_value(&bndl, "latitude", lat);
    append_to_bundle_key_value(&bndl, "longitude", lon);
  }
  else
  {
    //set the config_tab_id to be the accelerometer testing sheet
    //msg.add("ACCEL_Data").add(nmea);
  }
  log_bundle(&bndl, PUSHINGBOX);

#endif

  memset(nmea, '\0', sizeof(nmea));
}


void Serial2_setup()
{
  Serial2.begin(115200); //rx on rover to pin 10
  // Assign pins 10 & 13 SERCOM functionality, internal function
  pinPeripheral(10, PIO_SERCOM); //Private functions for serial communication
  pinPeripheral(13, PIO_SERCOM);
}


void append(char *s, char c)
{
  int len = strlen(s);
  s[len] = c;
  s[len + 1] = '\0';
}

/*****************************************************

*****************************************************/
void readBuffer()
{
  char nmeaChar;
  uint8_t input_flag = 0; 
  while (Serial2.available() && nmeaChar != 0x20)
  { //read from serial 0x0A for line feed character in actual NMEA string
    nmeaChar = Serial2.read();
    append(nmeaString, nmeaChar);
  }
  Serial.println("String...");
  Serial.println(nmeaString);
}

/*
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
}*/

//parse a nmea string and return int corresponding to quality of measurement
short int nmeaType(char *nmea, uint8_t string_len)
{
  int pos = 0;
  int index;
  for (index = 0; index < string_len; index++)
  {
    if (pos == 13)
      break;
    if (nmea[index] == ',')
      pos++;
  }
  if (pos < 13)
  {
    return -1;
  }
  switch (nmea[index])
  {
  case ('N'):
    return -1;
  case ('A'):
    return 0;
  case ('D'):
    return 1;
  case ('F'):
    return 2;
  case ('R'):
    return 3;
  default:
    return -1;
  }
}

//format must have a place from the node number in it
void fillFilename(char *dest, const char *format, int node_number)
{
  memset(dest, '\0', FILENAME_LENGTH);
  sprintf(dest, format, node_number);
}

void saveString(char *nmeaString, int rec_from)
{
  char filename[FILENAME_LENGTH];
  bool stringIsValid = false;
  if (nmeaString[0] == '$')
  {
    fillFilename(filename, "GDMP/%d", rec_from);
    for (int i = 0; i < MAX_LEN - 1; i++)
    {
      if (nmeaString[i] == '*')
      {
        memset(nmeaString + i + 3, 0, MAX_LEN - i - 3);
        stringIsValid = true;
        break;
      }
    }
  }
  else
  {
    fillFilename(filename, "ADMP/%d", rec_from);
    stringIsValid = true;
  }
  /*if(stringIsValid){
    sd_save_elem <char*> (filename, nmeaString, '\n');
    short int file_type = nmeaType(nmeaString, nmea_len);
    if(file_type != -1 && nodes[rec_from % N_NODES] <= file_type){
      fillFilename(filename, "GSEL/%d", rec_from);
      sd_delete_file(filename);
      sd_save_elem <char*> (filename, nmeaString, '\n');
      nodes[rec_from % N_NODES] = file_type;
    }
  }*/
}
