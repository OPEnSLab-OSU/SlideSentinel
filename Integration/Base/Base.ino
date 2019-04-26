/********************** PROGRAM DEXCRIPTION *********************
 
 ****************************************************************/

/*
IMPORTANT URL'S:
https://rockblock.rock7.com/Operations
https://postproxy.azurewebsites.net
future development: http://engineeringnotes.blogspot.com/2015/01/nmea-checksum-calculator-code.html

optional force upload 
set verbosity
Dont filter for best on every upload, log all data to cycle.txt then iterate through SD and grab the best string and latest state
Rethink the pertinent data which should be sent
Tag messages sent from accelerometer wake up, clean up accelerometer data, state data should be the first sent piece of data on wake up 

ROCKBLOCK PCB
use level shifter IC, verify that all connections are valid, re-wire the board with the new IC, find new voltage regulator
*/

#include "config.h"
#include "loom_preamble.h"
#include "wiring_private.h"
#include "IridiumSBD.h"
#include "MessageParser.h"
#include <EnableInterrupt.h>
#include <MemoryFree.h>

#define DEBUG 1
#define DEBUG_SD 1
#define TOGGLE_SATCOM true        //turns SATCOM uploads on or off
#define FORCE_SATCOM_FAILURE true //forces satcom to always fail for debug purposes
#define TOGGLE_UPDATES true       //turns on interrupt based Hub configuration
#define FORCE_UPDATE false        //forces the update routine to occur for testing
#define TOGGLE_RETRY true
#define NODE_NUM 1
#define NET_AV 19
#define RING_INDICATOR_PIN A3
#define IridiumSerial Serial1
#define NODE_TIMER 20  //for asking the hub when the next upload will occur (min)
#define MAX_LENGTH 150 //every buffer declared in this program is initialized with this length, Packets longer than 150 will always be dropped
#define MAX_FILE 30

//RF Communication
void Serial2_setup();

//Satcom
void initRockblock();
uint8_t getSignalQuality();
uint8_t getNetwork();
bool successfulUpload(const char *message);

//String manipulation
void append(char *s, char c);
void PrintMsg(OSCMessage &msg);

//SD Card Logging
void initialize_nodes();
void gpsProc(OSCMessage &msg);
void stateProc(OSCMessage &msg);

//Globals
bool is_retry;
volatile bool update_flag;
bool str_flag;
unsigned long satcom_count;
unsigned long retry_timer;
unsigned long retry_timer_prev;
unsigned long satcom_freq;
unsigned long retry_freq;
unsigned long last_wake;
uint16_t failed_count;
uint16_t retry_count;
unsigned long message_count;
const char *folder = "NODE_";

//Serial Port Init, RX pin 13, TX pin 10, configuring for rover UART
Uart Serial2(&sercom1, 13, 10, SERCOM_RX_PAD_1, UART_TX_PAD_2);
IridiumSBD modem(IridiumSerial, -1, RING_INDICATOR_PIN);

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void setup()
{
  Serial.begin(115200); //Opens the main serial port to communicate with the computer

  Loom_begin();
  Serial2_setup(); //Serial port for communicating with Freewave radios
  setup_sd();
  initRockblock();
  initialize_nodes();
  getNetwork();
  pinMode(RING_INDICATOR_PIN, INPUT);

#if TOGGLE_UPDATES
  attachInterrupt(digitalPinToInterrupt(RING_INDICATOR_PIN), toggleUpdate, FALLING);
#endif

  update_flag = false;
  str_flag = false;

  satcom_freq = 3;     //number of node messages between satcom uploads
  retry_freq = 180000; //check for updates once a day
  retry_count = 0;
  failed_count = 0;
  message_count = 0;

  retry_timer_prev = 0;
  retry_timer = millis();

  Serial.print("Initializing satcom to upload once every ");
  Serial.print(satcom_freq);
  Serial.println(" messages...");
  Serial.println("Setup Complete.. ");

  serialFlush();
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void loop()
{
  unsigned long internal_time_cur;
  unsigned long internal_time;
  int count = 0;
  char messageIn[MAX_LENGTH];
  memset(messageIn, '\0', sizeof(messageIn));
  diagnose();

  if (Serial2.available())
  {
    internal_time_cur = millis();
    while (millis() - internal_time_cur < 5000)
    {
      char input;
      if (Serial2.available())
      {
        if (input = Serial2.read() == (byte)2)
        {
          internal_time = millis();
          while (millis() - internal_time < 5000)
          {
            if (Serial2.available())
            {
              input = Serial2.read();
              count++; //for preventing buffer overflow
              if (input == 4 || count == MAX_LENGTH - 1)
              {
                str_flag = true;
                break;
              }
              append(messageIn, input);
            }
          }
        }
        count = 0;
        internal_time_cur = millis();
      }

      //if a message was successfully collected dispatch its contents HERE
      if (verify(messageIn))
        processData(messageIn);
      memset(messageIn, '\0', sizeof(messageIn));
    }

    //when done receiving all data for the wake cycle process the best captured position
    satcom_count++;
    last_wake = millis(); //for determining when the next satcom upload will occur
    serialFlush();
  }

  //check to make a satcom uplaod
  if ((satcom_count >= satcom_freq) && TOGGLE_SATCOM)
  {
    attemptSend(false);
    check_retry();
    satcom_count = 0;
    serialFlush();
  }

  //attempt retry
  if (TOGGLE_RETRY && (is_retry && ((retry_timer - retry_timer_prev) > retry_freq) && TOGGLE_SATCOM))
  {

#if DEBUG
    Serial.println("ATTEMPTING RETRY");
#endif
    attemptSend(true);
    retry_timer_prev = retry_timer;
    retry_count++;
    check_retry();
    serialFlush();
  }

  //Make a health update and read any remote configuration datas
  if (FORCE_UPDATE || (update_flag && TOGGLE_UPDATES))
  {
    update_flag = false;
#if DEBUG
    Serial.println("CHECKING FOR UPDATES");
#endif
    update();
    serialFlush();
  }

  update_time();
}

/*****************************************************
 * Function: 
 * Description: 5 for state
*****************************************************/
void diagnose()
{
  char cmd;
  if (Serial.available())
  {
    //make function
    char status[50];
    String str_status;
    File e;
    memset(status, '\0', sizeof(status));
    int nextUpload = (millis() - last_wake);
    int minutes = (nextUpload / 60000);
    int seconds = (nextUpload - (minutes * 60000)) / 1000;
    cmd = Serial.read();
    switch (cmd)
    {
    case '0':
      Serial.print("Free Memory: ");
      Serial.println(freeMemory(), DEC);
      break;
    case '1':
      Serial.print("Satcom count: ");
      Serial.println(satcom_count);
      break;
    case '2':
      Serial.print("Satcom frequency: ");
      Serial.println(satcom_freq);
      break;
    case '3':
      Serial.print("is_retry: ");
      Serial.println(is_retry);
      break;
    case '4':
      Serial.print("Retry count: ");
      Serial.println(retry_count);
      break;
    case '5':
      Serial.print("Last Message: ");
      Serial.print(minutes);
      Serial.print(":");
      Serial.println(seconds);
      break;
    case '6':
      Serial.print("Messages remaining to be retrieved: ");
      Serial.println(modem.getWaitingMessageCount());
      break;
    case '7':
      collectStatus(status);
      str_status = String(status);
      break;
    case '8':
      Serial.println("Forcing satcom...");
      satcom_count = satcom_freq;
      break;
    case '9':
      Serial.println("checking sd...");
      e = SD.open(folder, FILE_READ);
      Serial.print("Value of e: ");
      Serial.println(e);
      e.close();
      break;
    default:
      Serial.println("Invalid command.");
    }
  }
}

/*****************************************************
 * Function: 
 * Description: 5 for state
*****************************************************/
bool verify(char messageIn[])
{
  if (str_flag)
  {
    str_flag = false;
    if (strcmp(getValueAt(messageIn, 0), "/GPS") == 0)
    {
      if (verifyMsg(messageIn, 11))
      {
        message_count++;
        return true;
      }
      else
      {
        failed_count++;
        memset(messageIn, '\0', sizeof(messageIn));
      }
    }
    else if (strcmp(getValueAt(messageIn, 0), "/State") == 0)
    {
      if (verifyMsg(messageIn, 6))
      {
        message_count++;
        return true;
      }
      else
      {
        failed_count++;
        memset(messageIn, '\0', sizeof(messageIn));
      }
    }
    else
    {
      failed_count++;
      memset(messageIn, '\0', sizeof(messageIn));
    }
  }
  return false;
}

/*****************************************************
 * Function: 
 * Description:  Interrupt service routine for ring indicator interrupts
 *               ring indicator is a low pulse, followed by another low pulse 10 seconds later
*****************************************************/
void toggleUpdate()
{
  static uint8_t low_count = 0;
  low_count++;
#if DEBUG
  Serial.print("Low count: ");
  Serial.println(low_count);
#endif
  if (low_count >= 2)
  {
    detachInterrupt(digitalPinToInterrupt(RING_INDICATOR_PIN));
    update_flag = true;
    low_count = 0;
  }
}

/*************************************
 * 
 * 
 *************************************/
//Tool this code to continue creating packets until every node on the network has had its data logged
//For each node in the network
//  find the best GPS and latest state and append it to the packet up ot 340 bytes
void attemptSend(bool retry)
{
  bool packetized = false;
  char packet[340];
  char node_num[2];
  memset(packet, '\0', sizeof(packet));

  for (int i = 0; i < NODE_NUM; i++)
  {
    //nested while loop to ensure that we remain under 340 bytes for the satcom upload
    memset(node_num, '\0', sizeof(node_num));
    itoa(i, node_num, 10);
    if (!retry)
      packetized = addMsg(packet, node_num, "/CYCLE.TXT");
    else
      packetized = addMsg(packet, node_num, "/RETRY.TXT");

#if DEBUG
    Serial.print("PACKET CREATED: ");
    Serial.println(packet);
    Serial.println();
#endif

    if (packetized && getNetwork())
    {
      if (!FORCE_SATCOM_FAILURE && successfulUpload(packet)) //REMOTE CONFIG TEST
      {
#if DEBUG
        Serial.println("Successfully sent message!");
#endif
      }
      else
      {
#if DEBUG
        Serial.println("Message failed to send.");
#endif
        write_to_retry(packet);
      }
    }
    else
    {
#if DEBUG
      Serial.println("Network not available or failed to create a packet during this wake cycle, writing to retry folder...");
#endif
      write_to_retry(packet);
    }
  }
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void readLine(char buf[], File e)
{
  char input;
  input = e.read();
  while (input != '\n') // if char not  eol
  {
    append(buf, input);
    input = e.read();
  }
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void addElem(char msg[], char buf[])
{
  if (strlen(msg) != 0)
    strcat(msg, "#");
  strcat(msg, buf);
}

/*****************************************************
 * Function:    Takes a node number and file name, finds the latest and highest quality position for the given folder
 * Description: 
*****************************************************/
//Tool this code to create packets for every node on the network up to 340 bytes
bool addMsg(char *msg, char *node_num, const char *file)
{
  File e;
  char cur[MAX_LENGTH];
  char path[MAX_FILE];
  char best[MAX_LENGTH];
  char latestState[MAX_LENGTH];
  memset(latestState, '\0', sizeof(latestState));
  memset(best, '\0', sizeof(best));
  memset(cur, '\0', sizeof(cur));
  createFilePath(path, node_num, file);

  e = SD.open(path, FILE_READ);

  if (e && e.size() > 0)
  {
    while (e.available())
    {
      readLine(cur, e);
      if (strcmp(getValueAt(cur, 0), "/GPS") == 0)
        compareNMEA(cur, best);
      else if (strcmp(getValueAt(cur, 0), "/State") == 0)
      {
        memset(latestState, '\0', sizeof(latestState));
        strcpy(latestState, cur);
      }
      memset(cur, '\0', sizeof(cur));
    }

    addElem(msg, best);
    addElem(msg, latestState);

    e.close();
    SD.remove(path); //Fix ME
    e = SD.open(path, FILE_WRITE);
    if (e)
    {
      e.close();
    }
    return true;
  }
  else
    return false;
}

/*****************************************************
 * Function: 
 * Description: Eventually will need to decompose packets by # delimeter and place each packet back in respective retry folder, /G and /S
*****************************************************/
void write_to_retry(char packet[])
{
#if DEBUG
  Serial.print("FAILED TO SEND, WRITING TO NODE 0 RETRY FOLDER: ");
  Serial.println(packet);
#endif

  char buf[MAX_LENGTH];
  memset(buf, '\0', sizeof(buf));
  char *token = strtok(packet, "#");
  while (token != NULL)
  {
    Serial.print("Token: ");
    Serial.println(token);
    strncpy(buf, token, sizeof(buf));
    write_sd_str("/RETRY.TXT", buf);
    memset(buf, '\0', sizeof(buf));
    token = strtok(NULL, "#");
  }
}

/*****************************************************
 * Function: 
 * Description: 
*****************************************************/
void SDsize(File dir, int &size)
{
  while (true)
  {
    File entry = dir.openNextFile();
    if (!entry)
      return;
    if (entry.isDirectory())
      SDsize(entry, size);
    else
      size += entry.size();
    entry.close();
  }
}

/*****************************************************
 * Function: 
 * Description: 
*****************************************************/
void collectStatus(char status[])
{
  int size = 0, nextUpload;
  File root = SD.open("/");
  sprintf(status, "%d", satcom_count);
  strcat(status, ",");
  sprintf(status + strlen(status), "%d", satcom_freq);
  strcat(status, ",");
  if (root)
  {
    SDsize(root, size);
    root.close();
    sprintf(status + strlen(status), "%d", size);
  }
  nextUpload = (millis() - last_wake);
  int minutes = (nextUpload / 60000);
  int seconds = (nextUpload - (minutes * 60000)) / 1000;
  strcat(status, ",");
  sprintf(status + strlen(status), "%d", minutes);
  strcat(status, ":");
  sprintf(status + strlen(status), "%d", seconds);
  strcat(status, ",");
  int mem = freeMemory();
  sprintf(status + strlen(status), "%d", mem);
  strcat(status, ",");
  sprintf(status + strlen(status), "%d", message_count);
  strcat(status, ",");
  sprintf(status + strlen(status), "%d", failed_count);
#if DEBUG
  String str_status = String(status);
  Serial.print("HUB STATUS: ");
  Serial.println(str_status);
#endif
}

/*****************************************************
 * Function: 
 * Description: 
*****************************************************/
void update()
{
  char buffer[50];
  char status[50];
  unsigned int bufferSize = sizeof(buffer);
  memset(buffer, '\0', bufferSize);
  memset(buffer, '\0', sizeof(status));
  int signalQuality = -1, err;
  int ret = 0;

  err = modem.getSignalQuality(signalQuality);
  if (signalQuality > 0 && getNetwork())
  {
    collectStatus(status);
    err = modem.sendReceiveSBDText(status, (uint8_t *)buffer, bufferSize);
    if (err == ISBD_SUCCESS && checkBuf(buffer))
    {
      sscanf(buffer, "%d", &ret);
      if (ret > 0 && ret < 672) //minimum satcom uploads of one upload per week
      {
        satcom_freq = ret;
#if Debug
        Serial.print("System will now be sending satcom uploads every ");
        Serial.print(ret);
        Serial.println(" messages");
#endif
      }
#if DEBUG
      Serial.println();
      Serial.print("Messages remaining to be retrieved: ");
      Serial.println(modem.getWaitingMessageCount());
#endif
    }
  }
  //force a string upload
  satcom_count = satcom_freq;
  attachInterrupt(digitalPinToInterrupt(RING_INDICATOR_PIN), toggleUpdate, FALLING);
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
bool checkBuf(char buffer[])
{
  int i = 0;
  while (buffer[i] != '\0')
  {
    if ((buffer[i] < 0x30 || buffer[i] > 0x39) && (i > 0))
    {
      return false;
    }
    i++;
  }
  return true;
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
bool successfulUpload(const char *message)
{
  int err;
  err = modem.sendSBDText(message);
  if (err != ISBD_SUCCESS)
  {
#if DEBUG
    Serial.print("sendSBDText failed: error ");
    Serial.println(err);
    if (err == ISBD_SENDRECEIVE_TIMEOUT)
      Serial.println("Try again with a better view of the sky.");
    if (err == ISBD_MSG_TOO_LONG)
      Serial.println("Try again with a shorter message.");
    if (err == ISBD_SBDIX_FATAL_ERROR)
      Serial.println("Fatal Error. Something went wrong.");
#endif
    return false;
  }
  else
  {
#if DEBUG
    Serial.println("Message successfully sent.");
#endif
    return true;
  }
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void check_retry()
{
  File e;
  char path[MAX_FILE];

  //will need to be eventually tooled to scan through directories for each node on the network
  createFilePath(path, "0", "/RETRY.TXT"); //FIX ME

  e = SD.open(path, FILE_READ);
  if (e.size() > 0)
    is_retry = true;
  else
    is_retry = false;

  e.close();
#if DEBUG
  if (is_retry)
    Serial.println("RETRY TOGGLED ON");
#endif
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void initialize_nodes()
{
  //only use of strings and dynamic memory allocation occurs in this initialization routine
  String folder;
  Serial.println("Creating directories for each node in the network...");
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

      e = SD.open(folder + "/CYCLE.TXT", FILE_WRITE);
      if (e)
      {
        Serial.println(folder + "/CYCLE.TXT created...");
        e.close();
      }
      else
        Serial.println("Failed to create " + folder + "/CYCLE.TXT!");

      e = SD.open(folder + "/RETRY.TXT", FILE_WRITE);
      if (e)
      {
        Serial.println(folder + "/RETRY.TXT created...");
        e.close();
      }
      else
        Serial.println("Failed to create " + folder + "/RETRY.txt!");
    }
  }
  Serial.println("Directories complete!");
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void initRockblock()
{
  Serial.println("Initializing RockBlock...");
  int err;
  char version[12];

  // Start the serial port connected to the satellite modem
  IridiumSerial.begin(19200);

  // Begin satellite modem operation
  Serial.println("Starting modem...");
  err = modem.begin();
  if (err != ISBD_SUCCESS)
  {
    Serial.print("Begin failed: error ");
    Serial.println(err);
    if (err == ISBD_NO_MODEM_DETECTED)
      Serial.println("No modem detected: check wiring.");
  }

  //get the Rockblock firmware version
  err = modem.getFirmwareVersion(version, sizeof(version));
  if (err != ISBD_SUCCESS)
  {
    Serial.print("FirmwareVersion failed: error ");
    Serial.println(err);
    return;
  }
  Serial.print("Firmware Version is ");
  Serial.print(version);
  Serial.println(".");

  //determine current signal quality
  getSignalQuality();
  Serial.println("Configuring Network availability...");
  pinMode(NET_AV, INPUT); //Network availability
  is_retry = false;
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
uint8_t getNetwork()
{
#if DEBUG
  Serial.println("Getting network availability");
  if (digitalRead(NET_AV))
    Serial.println("NETWORK AVAILABLE");
  else
    Serial.println("NETWORK NOT AVAILABLE");
#endif
  return digitalRead(NET_AV);
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
uint8_t getSignalQuality()
{
  //determine current signal quality
  int signalQuality = -1;
  int err;
  err = modem.getSignalQuality(signalQuality);
  if (err != ISBD_SUCCESS)
  {
    Serial.print("SignalQuality failed: error ");
    Serial.println(err);
  }
  Serial.print("On a scale of 0 to 5, signal quality is currently: ");
  Serial.print(signalQuality);
  Serial.println(".");
  return signalQuality;
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void append(char s[], char c)
{
  int len = strlen(s);
  s[len] = c;
  s[len + 1] = '\0';
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void processData(char msg[])
{
#if DEBUG
  Serial.println();
  print_Msg(msg);
#endif
  //cycle.txt contains all data for the current satcom upload cycle.
  write_sd_str("/CYCLE.TXT", msg);
  if (strcmp(getValueAt(msg, 0), "/GPS") == 0)
    write_sd_str("/GPS_LOGS.TXT", msg);
  else if (strcmp(getValueAt(msg, 0), "/State") == 0)
    write_sd_str("/S_LOGS.TXT", msg);
}

/*****************************************************
 * Function: 
 * Description: 
*****************************************************/
void createFilePath(char path[], char *node_num, const char *file)
{
  strcpy(path, folder);
  strcat(path, node_num);
  strcat(path, file);
}

/*****************************************************
 * Function: 
 * Description: Needs to use the message node number field
*****************************************************/
void write_sd_str(const char *file, char message[])
{
  File e;
  int num;
  char path[MAX_FILE];
  memset(path, '\0', sizeof(path));
  createFilePath(path, "0", file);

  e = SD.open(path, FILE_WRITE);
  if (e)
  {
    num = e.write(message, strlen(message));
    e.write("\n", 1);
    e.close();

#if DEBUG
    if (num != 0)
    {
      Serial.print("Wrote ");
      Serial.print(num);
      Serial.println(" bytes to file...");
    }
    else
      Serial.println("Error writing to SD card");
#endif
  }
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

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void update_time()
{
  retry_timer = millis();
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void SERCOM1_Handler()
{
  Serial2.IrqHandler();
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void serialFlush()
{
  while (Serial2.available() > 0)
  {
    char t = Serial2.read();
  }
}
