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

Test the send and receive functionality of the ROCKBLOCK

1. RTC wake ups
2. Sending upon wake up with some flag
3. If the network is not present reseting the alarm
4. Remember that you have a file in which you detail this information

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
void write_gps(String node, const char *file);

//Globals
char inputBuf[MAX_LEN];
uint32_t satcom_frequency;
volatile bool alarm_flag;

//testing globals
char UTC[20] = "030718.1408";
char node_num[2] = "0";
char lat[20] = "2447.0924110";
char lon[20] = "12100.5227860";
char alt[15] = "103.323";
char mode[2] = "R";
char age[10] = "1.2";
char ratio[10] = "4.2";

char UTC2[20] = "038.1408";
char node_num2[2] = "2";
char lat2[20] = "500.1110";
char lon2[20] = "1122.12340";
char alt2[15] = "103.3";
char mode2[2] = "N";
char age2[10] = "1.1";
char ratio2[10] = "2.2";

char UTC3[20] = "038.1408";
char node_num3[2] = "3";
char x[20] = "500.1110";
char y[20] = "1122.12340";
char z[15] = "103.3";
char voltage[3] = "14";
char temp[10] = "21.1";

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
  //initialize_nodes();
  memset(inputBuf, '\0', sizeof(inputBuf));
  Serial.println("ALL SYSTEMS CONFIGURED... ");

  //some initialization routine for setting the update frequency
  satcom_frequency = 1;
  alarm_flag = false;
  setRTCAlarm_Relative(0, satcom_frequency, 0);
}

/**************************************
 *    Test what the dispatch function does with a bundle that has multiple messages, may there is some looping functionality?
 * 
 *************************************/
void loop()
{
  OSCBundle bndl;
  //AWESOME: since the SD takes a substantial amount of time and I will be receiving strings quickly I can buffer them in individual messages in a receive bundle then call dispatch once!!!
  bndl.add("/GPS").add((const char *)node_num).add((const char *)UTC).add((const char *)lat).add((const char *)lon).add((const char *)alt).add((const char *)mode).add((const char *)age).add((const char *)ratio);
  bndl.add("/GPS").add((const char *)node_num2).add((const char *)UTC2).add((const char *)lat2).add((const char *)lon2).add((const char *)alt2).add((const char *)mode2).add((const char *)age2).add((const char *)ratio2);
 // bndl.add("/State").add((const char *)node_num3).add((const char *)UTC3).add((const char *)x).add((const char *)y).add((const char *)z).add((const char *)voltage).add((const char *)temp);

  /*read in the message
  while (!SLIPSerial.endofPacket())
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
  Serial.print("Alarm flag: ");
  Serial.println(alarm_flag);
  LOOM_DEBUG_Println("Current Time:");
	print_DateTime( rtc_inst.now() );
  


  if (Serial.available())
  {
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
      if (e)
      {
        Serial.println(folder + "/GPS_LOGS.txt created...");
        e.close();
      }
      else
        Serial.println("Failed to create " + folder + "/GPS_LOGS.txt!");

      e = SD.open(folder + "/S_LOGS.txt", FILE_WRITE);
      if (e)
      {
        Serial.println(folder + "/S_LOGS.txt created...");
        e.close();
      }
      else
        Serial.println("Failed to create " + folder + "/S_LOGS.txt!");

      e = SD.open(folder + "/GPS_C.txt", FILE_WRITE);
      if (e)
      {
        Serial.println(folder + "/GPS_C.txt created...");
        e.close();
      }
      else
        Serial.println("Failed to create " + folder + "/GPS_C.txt!");

      e = SD.open(folder + "/S_C.txt", FILE_WRITE);
      if (e)
      {
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
  buffer[strlen(buffer) - 1] = '\0';
  Serial.println(buffer);
  memset(buffer, '\0', sizeof(buffer));
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void gpsProc(OSCMessage &msg)
{
  String node_num;

  Serial.println("GPS router...");

  node_num = get_data_value(&msg, 0); //0th position is the node number
  write_sd(node_num, "/GPS_LOGS.TXT", &msg);
  write_sd(node_num, "/GPS_C.TXT", &msg);
}

/*****************************************************
 * Function: 
 * Description:   Converts data in bndl to a string then 
 *                writes the string to the proper location on the SD
*****************************************************/
void stateProc(OSCMessage &msg)
{
  String node_num;
  Serial.println("State router...");
  node_num = get_data_value(&msg, 0); //0th position is the node number
  write_sd(node_num, "/S_LOGS.TXT", &msg);
  write_sd(node_num, "/S_C.TXT", &msg);
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void write_sd(String node, const char *file, OSCMessage *msg)
{
  String folder;
  File e;
  char write_buffer[100];
  int num = 0;

  folder = "NODE_" + node;
  e = SD.open(folder + file, FILE_WRITE);
  if (e)
  {
    Serial.println("Successfully opened directory " + folder + file + "\n");
    oscMsg_to_string(write_buffer, msg);
    append(write_buffer, '\n');

    Serial.print("The OSC msg as a string:   ");
    Serial.println(write_buffer);

    num = e.write(write_buffer, strlen(write_buffer));
    e.close();
    if (num != 0)
    {
      Serial.print("Wrote ");
      Serial.print(num);
      Serial.println(" bytes to file...");
    }
    else
      Serial.println("Error writing to SD card");
  }
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void oscMsg_to_string(char *osc_string, OSCMessage *msg)
{
  char data_type;
  data_value value;
  int addr_len = 40;
  char addr_buf[addr_len];

  memset(osc_string, '\0', sizeof(osc_string));
  memset(addr_buf, '\0', addr_len);
  msg->getAddress(addr_buf, 0);
  strcat(osc_string, addr_buf);

  for (int j = 0; j < msg->size(); j++)
  {
    data_type = msg->getType(j);
    switch (data_type)
    {
    case 'f':
      value.f = msg->getFloat(j);
      snprintf(addr_buf, addr_len, ",f%lu", value.u);
      strcat(osc_string, addr_buf);
      break;

    case 'i':
      value.i = msg->getInt(j);
      snprintf(addr_buf, addr_len, ",i%lu", value.u);
      strcat(osc_string, addr_buf);
      break;

    case 's':
      char data_buf[40];
      msg->getString(j, data_buf, sizeof(data_buf));
      snprintf(addr_buf, addr_len, ",s%s", data_buf);
      strcat(osc_string, addr_buf);
      break;

    default:
      if (data_type != '\0')
      {
        LOOM_DEBUG_Println("Invalid message arg type");
      }
      break;
    }
  }
  if (msg != NULL)
    strcat(osc_string, " ");
}

/*
  Serial.println("getting the current time...");
  char* current_time;
  current_time = get_timestring();
  Serial.print("Current time: ");
  Serial.println(current_time);
  delay(1000);
*/

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
