/********************** PROGRAM DEXCRIPTION *********************
 
 ****************************************************************/

/* To Do: 
1. folder init
2. on/off sheild
3. code for turning the ROCKBLOCK device off
4. FUTURE: Node monitoring, if we dont receive packets from a node over the course of two days we know something happened!
           Nodes get deployed they automatically get cheked into the network

HARDWARE: 
  Test RS232 shield, play around with ON/OFF controls and network available on the ROCKBLOCK+ (tommorrow)
  What happens when upload interrupt occurs but the network is not available???, break out of ISR and set alarm for every 10 minute interval until it works and sends!
  Keep in touch with Kevin about App

CODE:
 Parsing code: 
    SETUP...
    -initialize folders
    -read the time
    -turn off the ROCKBLOCK (initialize)

    LOOP...
    - read string
    - save fields to folder
    - parse an osc bundle
    
    - implement a counterwith the RTC. 
    - a simple register value in the counter upon hitting a value will produce a new upload
    - after uploading clear the data in the current nodes folder so we dont send redundant data if asked for a higher frequency upload rat 
    - IS THE NODE MAPPING THE TIME AND LOCATIONS TOGETHER? ASK GRAY
    --RTC set alarm functions, just reduce the times. Upon alarm interrupt send all of the SD shit!


//osc for arduino sending data description
//Talk to Storm about what data we want, get in touch with Kevin about configuring the spreadsheet
//network availability pin on ROCKBLOCK, unfortunatley if the network is not available and we try to send we still consume credits

ROCKBLOCK:
consumes credit 
username: 
pw: SlideSentinel



IMPORTANT URL'S:
https://rockblock.rock7.com/Operations
https://postproxy.azurewebsites.net

Old sheet: https://docs.google.com/spreadsheets/d/1Laa9uiGudBIt20_-pP0hakaxJziztCw4hWVcvK4OTTo/edit?usp=sharing
New Sheet: https://docs.google.com/spreadsheets/d/1whYegShf4DCOdr8wcOLqrE0EAbwxUkiP6pIbef7favo/edit?usp=sharing






*/

#include "config.h"
#include "loom_preamble.h"
#include <EnableInterrupt.h>
#include "wiring_private.h"

#define DEBUG 1
#define DEBUG_SD 1
#define CELLULAR 1
#define MAX_LEN 82 //NMEA0183 specification standard
#define FILENAME_LENGTH 20
#define NODE_NUM 5

//Freewave communication
void append(char *s, char c);
void pushString(char *nmea, uint8_t string_len);
void Serial2_setup();

//SD card
void initialize_nodes();
void fillFilename(char *dest, const char *format, int node_number);
void saveString(char *inputBuf, int rec_from);
void oscMsg_to_string(char *osc_string, OSCMessage *msg);
void readBuffer(char buffer[]);
void append(char *s, char c);
void serialToOSCmsg(char *osc_string, OSCMessage *msg);
void gpsProc(OSCMessage &msg);
void stateProc(OSCMessage &msg);

//Error handling
bool verify();

//Globals
char inputBuf[MAX_LEN];
unsigned int bndl_time;

//testing globals
char address[10] = "/GPS";
char UTC[20] = "030718.1408";
char node_num[2] = "1";
char lat[20] = "2447.0924110";
char lon[20] = "12100.5227860";
char alt[15] = "103.323";
char mode[2] = "R";
char age[10] = "1.2";
char ratio[10] = "4.2";


//Serial Port Init
//RX pin 13, TX pin 10, configuring for rover UART
Uart Serial2(&sercom1, 13, 10, SERCOM_RX_PAD_1, UART_TX_PAD_2);
void SERCOM1_Handler()
{
  Serial2.IrqHandler();
}


/**************************************
 * 
 * 
 *************************************/
void setup()
{
  Serial.begin(115200); //Opens the main serial port to communicate with the computer
  while (!Serial)
    ;
  Loom_begin();
  Serial2_setup(); //Serial port for communicating with Freewave radios
  Serial.println("serial configured... ");
  setup_rtc();
  setup_sd();
  initialize_nodes();
  memset(inputBuf, '\0', sizeof(inputBuf));
  Serial.println("ALL SYSTEMS CONFIGURED... ");
}

/**************************************
 * 
 * 
 *************************************/
void loop()
{
  OSCBundle bndl;
  bndl.add("/GPS").add((const char *)node_num).add((const char *)UTC).add((const char *)lat).add((const char *)lon).add((const char *)alt).add((const char*)mode).add((const char *)age).add((const char *)ratio);

  //Testing
  char buffer[200];
  readBuffer(buffer);

  //read in the message
  /* while (!SLIPSerial.endofPacket())
  {
    int size = SLIPSerial.available();
    if (size > 0)
    {
      //fill the msg with all of the available bytes
      while (size--)
      {
        msg.fill(SLIPSerial.read());
      }
    }
  }*/
  
  Serial.println("\n");
  print_bundle(&bndl);
  Serial.println("\n");

  char large_buf[120];
  memset(large_buf, '\0', sizeof(large_buf));
  convert_OSC_bundle_to_string(&bndl, large_buf);
  Serial.println(large_buf);

  bndl.dispatch("/GPS", gpsProc);
  bndl.dispatch("/State", stateProc);

}

/**************************************
 * 
 * 
 *************************************/
void initialize_nodes()
{
  Serial.println("Creating directories for each node in the network...");
  String folder;
  File e;
  for (int i = 0; i < NODE_NUM; i++)
  {
    folder = "NODE_" + String(i);
    if (!SD.exists(folder))
    {   
      SD.mkdir(folder);
      e = SD.open(folder + "/GPS_LOGS.txt", FILE_WRITE);
      if(e){
        Serial.println(folder + "/GPS_LOGS.txt created...");
        e.close();
      }
      else
        Serial.println("Failed to create " + folder + "/GPS_LOGS.txt!");

      e = SD.open(folder + "/S_LOGS.txt", FILE_WRITE);
      if(e){
        Serial.println(folder + "/S_LOGS.txt created...");
        e.close();
      }
      else
        Serial.println("Failed to create " + folder + "/S_LOGS.txt!");

      e = SD.open(folder + "/GPS_C.txt", FILE_WRITE);
      if(e){
        Serial.println(folder + "/GPS_C.txt created...");
        e.close();
      }
      else
        Serial.println("Failed to create " + folder + "/GPS_C.txt!");

      e = SD.open(folder + "/S_C.txt", FILE_WRITE);
      if(e){
        Serial.println(folder + "/S_C.txt created...");
        e.close();
      }
      else
        Serial.println("Failed to create " + folder + "/S_C.txt!");   
    }
  }
  Serial.println("Directories complete!");
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
double strToDouble(const char *gpsencoding)
{
  double a = strtod(gpsencoding, 0);
  return a;
}


/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void append(char *s, char c)
{
  int len = strlen(s);
  s[len] = c;
  s[len + 1] = '\0';
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void readBuffer(char buffer[])
{
  Serial.println("Reading Buffer...");
  char c = '\0';
  while (c != '\n') //read until we hit the line feed return 0x0d
  {
    if (Serial.available())
    { //if there is something in the buffer read it
      c = Serial.read();
      append(buffer, c);
    }
  }

  //overwrite the newline
  buffer[strlen(buffer)-1] ='\0';
  Serial.println(buffer);
  memset(buffer, '\0', sizeof(buffer));
}


/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void gpsProc(OSCMessage &msg)
{
  //sprintf(dest, format, node_number);
  Serial.println("GPS router...");
}

/*****************************************************
 * Function: 
 * Description:   Converts data in bndl to a string then 
 *                writes the string to the proper location on the SD
*****************************************************/
void stateProc(OSCMessage &msg)
{
  //sprintf(dest, format, node_number);
  Serial.println("State router...");
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void fillFilename(char *dest, const char *format, int node_number)
{
  memset(dest, '\0', FILENAME_LENGTH);
  sprintf(dest, format, node_number);
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void saveString(char *inputBuf, int rec_from)
{
  char filename[FILENAME_LENGTH];
  bool stringIsValid = false;
  if (inputBuf[0] == '$')
  {
    fillFilename(filename, "GDMP/%d", rec_from);
    for (int i = 0; i < MAX_LEN - 1; i++)
    {
      if (inputBuf[i] == '*')
      {
        memset(inputBuf + i + 3, 0, MAX_LEN - i - 3);
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
}

/*
  Serial.println("getting the current time...");
  char* current_time;
  current_time = get_timestring();
  Serial.print("Current time: ");
  Serial.println(current_time);
  delay(1000);
*/

/*

  if(Serial2.available())                    //Input from Freewave
  {
    readBuffer();                             //read from the input buffer and populate 'inputBuf[]' 
    if(verify()){                             //verifies that the string is of the proper format
      saveString(inputBuf[0], inputBuf[1]);   //save the string, pass the node number and the type of data (GPS or accelerometer)
    
    }
  }


    if (inputBuf[0] == '$')
    {
      pushString(inputBuf, sizeof(inputBuf) / sizeof(inputBuf[0]));
      bndl_time = millis();
    }
*/

/*****************************************************
 * Function: 
 * Description:
*****************************************************
void pushString(char *nmea, uint8_t string_len)
{
#if CELLULAR
  OSCBundle bndl;
  if (nmea[0] == '$')
  {
    Serial.print("NMEA string: ");
    Serial.println(inputBuf);
 
    bndl.add("/Loom1/Hub");
    //append_to_bundle_key_value(&bndl, "latitude", lat);
    append_to_bundle_key_value(&bndl, "NMEA_Data", nmea);
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

/*****************************************************
 * Function: 
 * Description:
*****************************************************
void append(char *s, char c)
{
  int len = strlen(s);
  s[len] = c;
  s[len + 1] = '\0';
}

/*****************************************************
 * Function:      verifyNMEA()
 * Description:   #DataType,NodeNum$Data<CR><LF>
 *                By this time because of therror handling on readBuffer,
 *                we know the string starts with # character
*****************************************************
bool verify()
{
  int i = 0; 
  for(i = 1; i < strlen(inputBuf); i++){    
    if(inputBuf[i] == '#'){                         //if we encounter another '$' in the middle of the string the string is invalid, from testing this is the only failure condition encountered
      Serial.print("Tossing string: ");
      Serial.println(inputBuf);
      return false; 
    }
  }
  return true;
}

/*****************************************************
 * Function:      verify()
 * Description:   This function will apply a number of filters on the input 
 *                verifying that the received string is either valid NMEA or 
 *                or a valid accelerometer reading. 
*****************************************************/
/*bool verify()
{
  //verify that the second character is a node in the network
  for(int i = 0; i < N_NODES; i++){
    if(inputBuf[1] == (i + '0')){
      if(inputBuf[0] == '$')
        return verifyNMEA(); 
      else if(inputBuf[0] = 'a')
        return verifyAccel();
    }
  }
  return false; 
}*/

/*****************************************************
 * Function: 
 * Description:
*****************************************************
void readBuffer()
{
  char nmeaChar;
  nmeaChar = Serial2.read();

  if (nmeaChar == '#')
  {
    append(inputBuf, nmeaChar);
    while (nmeaChar != '\n')   //read until we hit the line feed return 0x0d
    { 
      if(Serial2.available()){  //if there is something in the buffer read it 
        nmeaChar = Serial2.read();
        append(inputBuf, nmeaChar);
      }
    }
  }
  //inputBuf[strlen(inputBuf)-1] ='\0';
  inputBuf[strlen(inputBuf)-1] ='\0';
  Serial.println(inputBuf);
}


/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void Serial2_setup()
{
  Serial2.begin(115200);         //rx on rover to pin 10
                                 //Assign pins 10 & 13 SERCOM functionality, internal function
  pinPeripheral(10, PIO_SERCOM); //Private functions for serial communication
  pinPeripheral(13, PIO_SERCOM);
}
