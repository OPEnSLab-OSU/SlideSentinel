/********************** PROGRAM DEXCRIPTION *********************
 
 ****************************************************************/

/* To Do: 
  // 1.) now if temp has contents, we delete old file and create a new one, then copy temp to the new file
  // 2.) then we need to implement bulk upload failure, in which we decompose the upload and place it back in the retry folder
  // 3.) The last thing we do is write the hub configuration logic, for sending requests to the hub,
  //  GET MORE CREDITS, REFINE ROCKBLOCK PCB, ORDER MORE ROCKBLOCK PCB's, BUILD ROCKBLOCK INTO LOOM

  Note: edits were made to the Iridium SBD library power() function for use with the ROCKBLOCK+, lines 743 and 746
  Super frustrating, according to the documentation I should be able to just read and pass NULL, but it does not work unless I send first

  //ROCKBLOCK TESTING
  //RockBLOCK Plus  (OFF):< -1.5V     (ON):< 1.5
  //ON/OFF pin feather:  9, digital pin 9
  //Net Av pin: A5, digital pin 19
  //RI: A4, digital pin 18

  /*NOTES ON NET AV
    When a network is available, the Network Available pinout is high.
    When a network isn't available, the Network Available pinout is low.
  
IMPORTANT URL'S:
https://rockblock.rock7.com/Operations
https://postproxy.azurewebsites.net
*/

#include "config.h"
#include "loom_preamble.h"
#include <EnableInterrupt.h>
#include "wiring_private.h"
#include "IridiumSBD.h" 
#include <SLIPEncodedSerial.h>
#include "SlideS_parser.h"

#define DEBUG 1
#define DEBUG_SD 1
#define MAX_LEN 82 //NMEA0183 specification standard
#define FILENAME_LENGTH 20
#define NODE_NUM 1
#define CONFIG_UPDATE
#define NET_AV 19
#define ON_OFF 9

//timer definitions
#define CPU_HZ 48000000
#define TIMER_PRESCALER_DIV 1024
#define IridiumSerial Serial1

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
void initRockblock();
//void toggleRockblock(bool on);
uint8_t getSignalQuality();
uint8_t getNetwork();
void PrintMsg(OSCMessage &msg);
bool successfulUpload(const char *message);

//Initialize the satcom module
IridiumSBD modem(IridiumSerial); //TRY BY PASSING THE SLEEP PIN
bool is_on;
bool is_retry;

//Globals
char inputBuf[MAX_LEN];

unsigned long satcom_timer;
unsigned long retry_timer;
unsigned long update_timer;
unsigned long satcom_timer_prev;
unsigned long retry_timer_prev;
unsigned long update_timer_prev;

unsigned long update_freq;
unsigned long satcom_freq;
unsigned long retry_freq;

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
  setup_sd();
  initialize_nodes();
  memset(inputBuf, '\0', sizeof(inputBuf));
  //toggleRockblock(true);
  initRockblock();

  satcom_freq = 720000; //number of seconds between ROCKBLOCK uploads, 15 minutes
  retry_freq = 300000;   //number of seconds to wait if no network is available, 5 minutes
  update_freq = 86400000; //check for updates once a day

  satcom_timer_prev = 0;
  retry_timer_prev = 0;
  update_timer_prev = 0;
  satcom_timer = millis();
  retry_timer = millis();
  update_timer = millis();

  Serial.print("Initializing satcom to upload once every ");
  Serial.print((float)satcom_freq / 60000);
  Serial.println(" minutes...");
  Serial.println("Setup Complete.. ");

  serialFlush();
}

/**************************************
 *    
 * 
 *************************************/
void loop()
{
  unsigned long internal_time_cur;
  unsigned long internal_time_prev;

  if (Serial2.available())
  {
    bool str_flag = false;
    internal_time_cur = millis();

    //initialize the best string for this run, as the worst possible reading, REFRACTOR
    OSCMessage best("/GPS");
    best.add("X");
    best.add("X");
    best.add("X");
    best.add("X");
    best.add("X");
    best.add("X");
    best.add("X");
    best.add("X");

    while (millis() - internal_time_cur < 1000)
    {
      OSCMessage messageIN;
      messageIN.empty();
      int size;
      char input;

      if (Serial2.available())
      {
        if (input = Serial2.read() == (byte)2)
        {
          while (input != (byte)4)
          {
            if (Serial2.available())
            {
              input = Serial2.read();
              Serial.print(input);
              if (input == 4)
              {
                str_flag = true;
                break;
              }
              messageIN.fill(input);
            }
          }
        }
        internal_time_cur = millis();

        //Compare the incoming data with the current best
        if (get_address_string(&messageIN).equals("/GPS")) //Refractor, test that this properly filters input
        {
          compareNMEA(&messageIN, &best);
        }

      }



      if (str_flag)
      {
        if (!messageIN.hasError())    //check that the msg is not null 
        {
          messageIN.dispatch("/GPS", gpsProc);     //gpsProc
          messageIN.dispatch("/State", stateProc); //this function will write state data to both state logs, and also the cycle folder for the node
        }
        else
        {
          Serial.println(messageIN.getError());
        }
      }
      str_flag = false;
    }

    Serial.println("Checking to write the best string...");
    if (!best.hasError() && !(get_data_value(&best, 1).equals("X"))) //send the best string to the SD make sure it is initialized
    {
      best.dispatch("/GPS", gpsBest);
    }
    Serial.println("Done handling the best string...");
  }

  if ((satcom_timer - satcom_timer_prev) > satcom_freq)
  {
    attemptSendToday();
    satcom_timer_prev = satcom_timer;
    retry_timer_prev = satcom_timer;
    check_retry();
  }

  if (is_retry && ((retry_timer - retry_timer_prev) > retry_freq))
  {
    attemptSendRetry();
    Serial.println("Attempting to send retry...");
    retry_timer_prev = retry_timer;
    check_retry();
  }

  if ((update_timer - update_timer_prev) > update_freq)
  {
    update();
    update_timer_prev = update_timer;
  }

  update_time();
}

/*************************************
 * 
 * 
 *************************************/
void PrintMsg(OSCMessage &msg)
{
  print_message(&msg);
}

/*************************************
 * 
 * 
 *************************************/
void serialFlush()
{
  while (Serial.available() > 0)
  {
    char t = Serial.read();
  }
}

/*************************************
 * 
 * 
 *************************************/
void compareNMEA(OSCMessage *current, OSCMessage *best)
{
  Serial.println('\n');
  Serial.println("Inside compare NMEA...");
  Serial.print("Comparing: ");
  PrintMsg(*current);
  Serial.println("--------------- AND ----------------");
  PrintMsg(*best);
  Serial.println('\n');
  Serial.println('\n');

  char buf[5];
  memset(buf, '\0', sizeof(buf));
  get_data_value(current, 5).toCharArray(buf, sizeof(buf));
  char modeCur = buf[0];
  memset(buf, '\0', sizeof(buf));

  get_data_value(best, 5).toCharArray(buf, sizeof(buf));
  char modeBest = buf[0];
  memset(buf, '\0', sizeof(buf));

  Serial.print("Value of current mode: ");
  Serial.println(modeCur);
  Serial.print("Value of best mode: ");
  Serial.println(modeBest);

  //if (stringRank(get_data_value(current, 4)) > stringRank((char)get_data_value(best, 4)))
  if (stringRank(modeCur) > stringRank(modeBest))
  { //4 is the position of the mode
    best->empty();
    Serial.println("Better mode found!");
    deep_copy_message(current, best);
    PrintMsg(*best);
    Serial.println('\n');
    //Serial.println(get_data_value(best, 4));
  }
  //else if (stringRank(get_data_value(current, 6)) == stringRank(get_data_value(best, 6)))
  else if (stringRank(modeCur) == stringRank(modeBest))
  { //6 is the position of the RTK ratio
    Serial.println("Modes of same quality...");
    if (get_data_value(current, 6).toFloat() > get_data_value(best, 6).toFloat())
    {   
      Serial.println("Current value has higher RTK ratio...");
      best->empty();
      deep_copy_message(current, best);
    }
  }
  Serial.println("Leaving compare NMEA...");
}

/*************************************
 * 
 * 
 *************************************/
void update_time()
{
  satcom_timer = millis();
  retry_timer = millis();
  update_timer = millis();
}

/*************************************
 * Iterate through the the data stored for this upload period, all strings in  write_sd(node_num, "/CYCLE.TXT", &msg);
 * concatenate the last state message, the latest info about the state of the node 
 *************************************/
bool createPacket(char *buf, const char *file)
{
  String folder;
  String gpsStr;
  File e;
  char stateBuf[100];
  memset(stateBuf, '\0', sizeof(stateBuf));
  memset(buf, '\0', sizeof(buf));
  OSCMessage latestState;
  OSCMessage cur;
  OSCMessage best("/GPS");

  latestState.empty();
  Serial.println("Creating packet!!!");
  String node = "0"; //REFRACTOR ALSO CONSIDER LARGER NODE NETWORKS!!!
  folder = "NODE_" + node;
  e = SD.open(folder + file, FILE_READ);
  if (e.size() > 0)
  {
    if (e)
    {
      //initialize the best of the current cycle data be the first line, this needs be GPS DATA!!!!
      best.add("X");
      best.add("X");
      best.add("X");
      best.add("X");
      best.add("X");
      best.add("X");
      best.add("X");
      best.add("X");

      while (e.available()) //REFRACTOR, create function which takes an open file pointer and returns an OSC message TEST
      {
        gpsStr = e.readStringUntil('\n');
        gpsStr.toCharArray(buf, gpsStr.length());
        stringToOSCmsg(buf, &cur);                   //HERE check if the string is the state, if so update state string to send!
        if (get_address_string(&cur).equals("/GPS")) //Refractor, test that this properly filters input
        {
          compareNMEA(&cur, &best); //if gps in cycle folder check if it is better than the current best
        }
        else if (get_address_string(&cur).equals("/State"))
        {
          latestState.empty();
          Serial.println("New latest state found...");
          deep_copy_message(&cur, &latestState); //the lower the state strings are in the file the more recent, after encountering a new one simply update the state
        }
        cur.empty();
        memset(buf, '\0', sizeof(buf));
      }
    }
    else
    {
      Serial.println("Could not open file!");
      return false;
    }

    oscMsg_to_string(buf, &best);
    Serial.println("BEST STRING FOR THE CURRENT UPLOAD CYCLE");
    Serial.println(buf);

    //TEST ME! If state data was found add it to the packet!
    if (latestState.bytes() > 8) //check if we have a state string, if so concatenate the state string with the best GPS string for this satcom cycle, an empty OSC string apparantly has 8 bytes
    {
      oscMsg_to_string(stateBuf, &latestState);
      Serial.println("BEST LATEST FOR THE CURRENT UPLOAD CYCLE");
      Serial.println(stateBuf);

      strcat(buf, "#"); //concatenate the latest state reading with the best GPS to send for the ROCKBLOCK cycle
      strcat(buf, stateBuf);
      memset(stateBuf, '\0', sizeof(stateBuf));
    }

    Serial.println("Clearing file for current upload cycle.");
    SD.remove(folder + file);
    e = SD.open(folder + "/CYCLE.TXT", FILE_WRITE);
    if (e)
    {
      Serial.println(folder + "/CYCLE.TXT created...");
      e.close();
    }

    return true;
  }
  else
    return false;
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void stringToOSCmsg(char *osc_string, OSCMessage *bndl)
{
  bndl->empty();
  data_value value_union;
  char buf[strlen(osc_string) + 1];
  char *p = buf, *p2 = NULL;
  char *token = NULL, *msg_token = NULL;
  strcpy(buf, osc_string);
  OSCMessage *msg;
  msg_token = strtok_r(p, " ", &p);
  while (msg_token != NULL & strlen(msg_token) > 0)
  {
    p2 = msg_token;
    token = strtok_r(p2, ",", &p2);
    msg = &(bndl->setAddress(token));
    token = strtok_r(NULL, ",", &p2);
    uint8_t count = 0;

    while (token != NULL & strlen(token) > 0)
    {
      if (token[0] == 'f')
      {
        value_union.u = strtoul(&token[1], NULL, 0);
        msg->add(value_union.f);
      }
      else if (token[0] == 'i')
      {
        value_union.u = strtoul(&token[1], NULL, 0);
        msg->add(value_union.i);
      }
      else if (token[0] == 's')
      {
        msg->add(&token[1]);
      }
      token = strtok_r(p2, ",", &p2);
      count++;
    }
    msg_token = strtok_r(p, " ", &p);
  }
}

/*************************************
 * The one bug I kow of in this code, occurs if the SD fails to open on upload. 
 * This will cause another node wake cycle to be written to the best data folder. 
 * This actually is not a critical bug at all but something to keep in mind for refractoring
 *************************************/
void attemptSendToday()
{
  char packet[340];
  char buf[100]; //for decomposing bulk uploads
  String node_num;
  char *token;
  OSCMessage copy;
  copy.empty();
  if (createPacket(packet, "/CYCLE.TXT")) //create the packet, check if a there are strings which need to be sent
  {
    Serial.println("Packet created... Contents: ");
    Serial.println(packet);
    //toggleRockblock(true); //turn on the ROCKBLOCK
    if (getNetwork())
    {
      if (successfulUpload(packet))          //TESTING 
      {
        Serial.println("Successfully sent message!");
      }
      else
      { //message failed to send, write messaage to retry folder on SD card
        node_num = "0";
        //node_num = get_data_value(&msg,0); //0th position is the node number         //REFRACTOR
        token = strtok(packet, "#");
        while (token != NULL)
        {
          snprintf(buf, sizeof(buf), "%s", token);
          Serial.print("Writing back to SD for retrty: ");
          Serial.println(buf);
          write_sd_str(node_num, "/RETRY.TXT", buf);
          memset(buf, '\0', sizeof(buf));
          token = strtok(NULL, "#");
        }

        Serial.println("Failed to send, writing to retry folder...");
      }
    }
    else //network was not initially available, attempt to retry, COPY OVER MESSAGE TO RETRY BUFFER, first check to see if the buffer is empty.
    {
      Serial.println("Network not available, writing to retry folder...");
      node_num = "0";
      //node_num = get_data_value(&msg,0); //0th position is the node number         //REFRACTOR

      token = strtok(packet, "#");
      while (token != NULL)
      {
        snprintf(buf, sizeof(buf), "%s", token);
        write_sd_str(node_num, "/RETRY.TXT", buf);
        memset(buf, '\0', sizeof(buf));
        token = strtok(NULL, "#");
      }
    }
    copy.empty();
    memset(packet, '\0', sizeof(packet));
    //toggleRockblock(false);
  }
  else
    Serial.println("No strings needed to be sent or could not open data file.");
}

/*************************************
 * 
 * 
 *************************************/
void check_retry()
{
  String folder;
  File e;
  String node = "0"; //REFRACTOR
  folder = "NODE_" + node;
  e = SD.open(folder + "/RETRY.TXT", FILE_READ);
  if (e.size() > 0)
  {
    is_retry = true;
    Serial.println("Retry toggled on!");
  }
  else
  {
    is_retry = false;
    Serial.println("Retry toggled off!");
  }
}

/*************************************
 *
 * 
 *************************************/
void attemptSendRetry()
{
  Serial.println("Attempting to resend");
  char *token;
  String node_num;
  char bulk[340];
  char buf[100];
  memset(buf, '\0', sizeof(buf));
  memset(bulk, '\0', sizeof(bulk));

  //toggleRockblock(true); //turn on the ROCKBLOCK
  if (getNetwork() && bulkUpload(bulk, "/RETRY.TXT"))   
  {
    Serial.print("BULK UPLOAD: ");
    Serial.println(bulk);

    if (successfulUpload(bulk))           //TESTING 
    {
      Serial.println("Successfully sent bulk message!");
    }
    else
    { //message failed to send, write messaage to retry folder on SD card
      node_num = "0";
      //node_num = get_data_value(&msg,0); //0th position is the node number         //REFRACTOR
      token = strtok(bulk, "#");
      while (token != NULL)
      {
        snprintf(buf, sizeof(buf), "%s", token);
        write_sd_str(node_num, "/RETRY.TXT", buf);
        memset(buf, '\0', sizeof(buf));
        token = strtok(NULL, "#");
      }

      Serial.println("Failed to send, writing to retry folder...");
    }
  }
  else //network was not initially available, attempt to retry, COPY OVER MESSAGE TO RETRY BUFFER, first check to see if the buffer is empty.
  {
    Serial.println("Network not available for bulk upload or failed to create a bulk packet, writing to retry folder...");
  }
  memset(buf, '\0', sizeof(buf));
  memset(bulk, '\0', sizeof(bulk));
  //toggleRockblock(false);
}

/*****************************************************
 * Function:      This function will iterate through the retry folder, and concatenate as many string which need to be resent into one bulk upload
 *                
*****************************************************/
bool bulkUpload(char *buf, const char *file)
{
  String folder;
  String gpsStr;
  File e, t;

  char buffer[100];
  memset(buffer, '\0', sizeof(buffer));
  memset(buf, '\0', sizeof(buf));

  Serial.println("Creating packets for retry...");
  String node = "0"; //REFRACTOR
  folder = "NODE_" + node;

  t = SD.open(folder + "/TEMP.TXT", FILE_WRITE);
  if (!t)
  {
    Serial.println("Failed to open temp aborting retry");
    return false;
  }

  e = SD.open(folder + file, FILE_READ);
  if (e.size() > 0) //redundant logic Refractor
  {
    if (e)
    {
      while (e.available())
      {
        gpsStr = e.readStringUntil('\n');
        if ((strlen(buf) + gpsStr.length() + 1) >= 340)
        {
          // Serial.println("Full packet sized reach, writing to temp");
          t.println(gpsStr);
        }
        else
        {
          //here in this code, the characters are getting trimmed!
          strcat(buf, "#");                           //place the delimiter
          gpsStr.toCharArray(buffer, sizeof(buffer)); //test that this works
          strcat(buf, buffer);
          memset(buffer, '\0', sizeof(buffer));
        }
      }
      e.close();
      t.close();
    }
    else
    {
      Serial.println("failed to open data folder, aborting bulk upload");
      t.close(); //dont forget to close the temp file
      return false;
    }
  }

  Serial.println("Deleting old retry folder...");
  SD.remove(folder + file); //remove the old retry folder

  e = SD.open(folder + "/RETRY.TXT", FILE_WRITE); //create a new retry folder
  if (e)
  {
    Serial.println("New copy of " + folder + "/RETRY.TXT created...");
  }
  else
  { //it is very important that I verify that the program can recover from this, it will create this folder whenever it uploads, so I think im good as long as I do not delete the contents of temp
    Serial.println("Failed to create a new file!");
    t.close();
    return false;
  }

  t = SD.open(folder + "/TEMP.TXT", FILE_READ); //Super annoyed that I have to reopen temp, documentation declares that opening a file for writing opens it for both reading and writing...
  if (!t)
  {
    Serial.println("Failed to open temp for reading aborting retry");
    return false;
  }
  if (t.size() > 0)
  { //copy t to e, really annoying that file renaming is not supported, this is a terribly inefficient work around
    memset(buffer, '\0', sizeof(buffer));
    Serial.println("Copying data from temp to new retry folder..."); //ISSUE HERE, EVERY REWRITE A NEW CHARACTER IS ADDED !!!
    while (t.available())
    {
      gpsStr = t.readStringUntil('\n');
      gpsStr.trim();
      gpsStr.toCharArray(buffer, sizeof(buffer)); //find where a new byte is being added!!!
      write_sd_str(node, "/RETRY.TXT", buffer);
      memset(buffer, '\0', sizeof(buffer));
    }
  }

  Serial.println("deleting temp file....");
  SD.remove(folder + "/TEMP.TXT");

  Serial.println("Done!");
  e.close();
  t.close();
  return true;
}

/*************************************
 * Test how many credits Iridium receive library consumes...
 * Use it here in a case, statement, some format in maps to some upload frequency value...
 * 
 *************************************/
void update()
{
  uint8_t buffer[10];
  memset(buffer, '\0', sizeof(buffer));
  size_t bufferSize = sizeof(buffer);
  int err;
  long ret = 0;
  int signalQuality = -1;
  Serial.println("updating system....");

  err = modem.getSignalQuality(signalQuality);
  if (err != 0 || signalQuality == 0)
  {
    Serial.print("SignalQuality failed: error ");
    Serial.println(err);
  }
  else
  {
    if (/*(!first_sent || modem.getWaitingMessageCount() > 0) &&*/ getNetwork()) //we need a first not sent flag because rockblock is dumb and only accuratley reads incoming messages if at leasto e has been sent...
    {
      Serial.print("Signal quality is ");
      Serial.println(signalQuality);
      //toggleRockblock(true);
      err = modem.sendReceiveSBDBinary(buffer, 10, buffer, bufferSize);
      if (err != ISBD_SUCCESS)
      {
        Serial.print("Failed to receive... ");
      }
      else // success!
      {
        Serial.print("Inbound buffer size is ");
        Serial.println(bufferSize);

        if ((buffer[0] == 'C' || buffer[0] == 'S') && checkBuf(buffer)) //This is really unfortunate, the ROCKBLOCK can only receive data after sending data (I tested mutliple things because I wanted to avoid wasting the credit and nothing worked)
        {                                                               //Additionally I decided to not couple the receive with uploads because this could very likely be configured for month long upload cycles, in which case configuration commands
          int i, j = 0;
          int temp = 0; //to the system would not be registered for an entire month worst case, thus I think the update frequency will be once every three days about,
          for (i = 7; i >= 0; i--)
          {
            if (buffer[i] >= 0x30 && buffer[i] <= 0x39)
            {
              temp = pow(10, j) * (buffer[i] - '0');
              Serial.print("Value of temp: ");
              Serial.println(temp);
              Serial.print("This is buffer at i: ");
              Serial.println(buffer[i] - '0');
              j++;
              ret = ret + temp;
            }
          }

          Serial.print("Value of Ret: ");
          Serial.println(ret);

          //input will be the number of minutes per upload, number of minutes in a year
          if (ret > 0 && ret < 525600)
          {
            if (buffer[0] == 'C')
            {
              update_freq = (ret * 60000);
              Serial.println("System will now be checking for configuration commands every ");
              Serial.print(ret);
              Serial.print(" minutes (");
              Serial.print(update_freq);
              Serial.println(" milliseconds).");
            }
            else if (buffer[0] == 'S')
            {
              satcom_freq = (ret * 60000);
              Serial.println("System will now be sending satcom uploads every ");
              Serial.print(ret);
              Serial.print(" minutes (");
              Serial.print(satcom_freq);
              Serial.println(" milliseconds).");
            }
          }
        }
        else
        {
          Serial.println("Sadly invalid input data...");
        }
        Serial.println();
        Serial.print("Messages remaining to be retrieved: ");
        Serial.println(modem.getWaitingMessageCount());
      }
    }
    else
    {
      Serial.println("No messages...");
    }
  }
  memset(buffer, '\0', sizeof(buffer));
  //toggleRockblock(false);
}

/**************************************
 * Verifies that configuration input to the ROCKBLOCK IS VALID
 * 
 *************************************/
bool checkBuf(uint8_t buffer[])
{
  int i;
  Serial.println();
  Serial.print("size of buffer: ");
  Serial.println(sizeof(buffer));
  for (i = 0; i < sizeof(buffer) - 1; i++)
  {
    Serial.print("Value in checkbuf: ");
    Serial.println(buffer[i]); //REFRACTOR
    if ((buffer[i] < 0x30 || buffer[i] > 0x39) && (i > 0))
    {
      Serial.println("non integer input...");
      return false;
    }
  }
  Serial.println("Valid configuration data.");
  return true;
}

/**************************************
 * 
 * 
 *************************************/
bool successfulUpload(const char *message)
{
  int err;
  err = modem.sendSBDText(message);
  if (err != ISBD_SUCCESS)
  {
    Serial.print("sendSBDText failed: error ");
    Serial.println(err);
    if (err == ISBD_SENDRECEIVE_TIMEOUT)
      Serial.println("Try again with a better view of the sky.");
    if (err == ISBD_MSG_TOO_LONG)
      Serial.println("Try again with a shorter message.");
    if (err == ISBD_SBDIX_FATAL_ERROR)
      Serial.println("Fatal Error. Something went wrong.");
    return false;
  }
  else
  {
    Serial.println("Message successfully sent.");
    return true;
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

      e = SD.open(folder + "/CYCLE.TXT", FILE_WRITE);
      if (e)
      {
        Serial.println(folder + "/CYCLE.TXT created...");
        e.close();
      }
      else
        Serial.println("Failed to create " + folder + "/CYCLE.TXT!");
      //Retry strings!
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
uint8_t getNetwork()
{
  Serial.println("Getting network availability");
  return digitalRead(NET_AV);
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************
void toggleRockblock(bool on)
{
  if (on)
  {
    Serial.println("Turning device on...");
    delay(1000);
    digitalWrite(ON_OFF, LOW);
    is_on = true;
  }
  else
  {
    Serial.println("Turning device off...");
    digitalWrite(ON_OFF, HIGH);
    is_on = false;
  }
}*/

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
  Serial.println("Configuring ON/OFF, and Network availability...");
  pinMode(ON_OFF, OUTPUT); //ON/OFF control
  pinMode(NET_AV, INPUT);  //Network availability
  //toggleRockblock(false);
  is_on = false;
  is_retry = false;
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
void printHex(char buffer[])
{
  int i = 0;
  for (i = 0; i < 200 /*sizeof(buffer)*/; i++)
  {
    Serial.print(buffer[i], HEX);
    Serial.write(',');
  }
  Serial.println();
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void gpsProc(OSCMessage &msg)
{
  PrintMsg(msg);
  String node_num;

  node_num = "0";
  //node_num = get_data_value(&msg, 0); //0th position is the node number         //REFRACTOR
  write_sd(node_num, "/GPS_LOGS.TXT", &msg);
  //write_sd(node_num, "/RETRY.TXT", &msg); //TESTING
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void gpsBest(OSCMessage &msg)
{
  Serial.println("Writing best message to SD card for this wake cycle...");
  PrintMsg(msg);
  String node_num;

  node_num = "0";
  //node_num = get_data_value(&msg,0); //0th position is the node number         //REFRACTOR
  write_sd(node_num, "/CYCLE.TXT", &msg);
}

/*****************************************************
 * Function:          Note on this function, a tate string is sent at the end of every node wake cylce, every upload cycle we want to send the latest 
 *                    state of the node, thus everytime a state string is received we delete the current state folder and write the new state data to the folder!
 *                    Another option would be to just all data in cycle folder and a retry folder... 
 *                    Then when create packet is called, while comparing GPS strings we check the header and only compare if its GPS, if its State we continuously 
 *                    update the most current state until we reach the end of the file! I think I actually like this option more
 * Description:   
*****************************************************/
void stateProc(OSCMessage &msg)
{
  String node_num;
  Serial.println("State router...");
  //node_num = get_data_value(&msg, 0); //0th position is the node number
  node_num = "0";
  write_sd(node_num, "/S_LOGS.TXT", &msg);
  write_sd(node_num, "/CYCLE.TXT", &msg);
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
    oscMsg_to_string(write_buffer, msg);
    append(write_buffer, '\n');
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
void write_sd_str(String node, const char *file, char message[])
{
  String folder;
  File e;
  int num = 0;

  folder = "NODE_" + node;
  e = SD.open(folder + file, FILE_WRITE);
  if (e)
  {
    append(message, '\n');
    num = e.write(message, strlen(message));
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
