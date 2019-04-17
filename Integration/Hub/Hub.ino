/********************** PROGRAM DEXCRIPTION *********************
 
 ****************************************************************/

/*
IMPORTANT URL'S:
https://rockblock.rock7.com/Operations
https://postproxy.azurewebsites.net
future development: http://engineeringnotes.blogspot.com/2015/01/nmea-checksum-calculator-code.html
*/

#include "config.h"
#include "loom_preamble.h"
#include "wiring_private.h"
#include "IridiumSBD.h"
#include "SlideS_parser.h"
#include <EnableInterrupt.h>
#include <CRC32.h>

#define DEBUG 1
#define DEBUG_SD 1
#define TOGGLE_SATCOM true         //turns SATCOM uploads on or off
#define FORCE_SATCOM_FAILURE false //forces satcom to always fail for debug purposes
#define TOGGLE_UPDATES true        //turns on interrupt based Hub configuration
#define FORCE_UPDATE false         //forces the update routine to occur for testing
#define NODE_NUM 1
#define NET_AV 19
#define RING_INDICATOR_PIN A3
#define IridiumSerial Serial1
#define NODE_TIMER 20 //for asking the hub when the next upload will occur (min)

//RF Communication
void Serial2_setup();

//Satcom
void initRockblock();
uint8_t getSignalQuality();
uint8_t getNetwork();
bool successfulUpload(const char *message);

//String manipulation
void append(char *s, char c);
void serialToOSCmsg(char *osc_string, OSCMessage *msg);
void oscMsg_to_string(char *osc_string, OSCMessage *msg);
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
unsigned long last_message;

//Serial Port Init, RX pin 13, TX pin 10, configuring for rover UART
Uart Serial2(&sercom1, 13, 10, SERCOM_RX_PAD_1, UART_TX_PAD_2);
IridiumSBD modem(IridiumSerial, -1, RING_INDICATOR_PIN);
CRC32 crc;

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
  initialize_nodes();
  initRockblock();
  getNetwork();
  pinMode(RING_INDICATOR_PIN, INPUT);

#if TOGGLE_UPDATES
  attachInterrupt(digitalPinToInterrupt(RING_INDICATOR_PIN), toggleUpdate, FALLING);
#endif

  update_flag = false;
  str_flag = false;

  satcom_freq = 3;     //number of node messages between satcom uploads
  retry_freq = 180000; //check for updates once a day

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

  if (Serial2.available())
  {
    internal_time_cur = millis();

    //initialize the best string for the wake cycle
    OSCMessage best("/GPS");
    best.add("X");
    best.add("X");
    best.add("X");
    best.add("X");
    best.add("X");
    best.add("X");
    best.add("X");
    best.add("X");
    best.add("X");
    best.add("0"); //initial checksum

    while (millis() - internal_time_cur < 5000)
    {
      OSCMessage messageIN;
      messageIN.empty();
      char input;

      if (Serial2.available())
      {
        if (input = Serial2.read() == (byte)2)
        {
          internal_time = millis();
          while (millis() - internal_time < 5000) //(input != (byte)4) ||
          {
            if (Serial2.available())
            {
              input = Serial2.read();
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
      }

      if (str_flag)
      {
        //Compare the incoming data with the current best
        if (get_address_string(&messageIN).equals("/GPS"))
        {
          if (verifyOSC(&messageIN, 11))
          {
            compareNMEA(&messageIN, &best);
          }
        }
        else if (get_address_string(&messageIN).equals("/State"))
          verifyOSC(&messageIN, 6);
        else //  if (get_address_string(current).equals("/GPS"))
        {
          messageIN.empty();
          str_flag = false;
        }
      }

      //if a message was successfully collected dispatch its contents
      if (str_flag)
      {
        if (!messageIN.hasError())
        {
          messageIN.dispatch("/GPS", gpsProc);
          messageIN.dispatch("/State", stateProc);
        }
      }
      str_flag = false;
      last_message = millis(); //for determining when the next satcom upload will occur
    }

    //After a completed send cycle dispatch the highest quality positional string
    if (!best.hasError() && !(get_data_value(&best, 1).equals("X"))) //send the best string to the SD make sure it is initialized
    {
      best.dispatch("/GPS", gpsBest);
    }
  }

  //check to make a satcom uplaod
  if ((satcom_count >= satcom_freq) && TOGGLE_SATCOM)
  {
    attemptSend(false);
    check_retry();
    satcom_count = 0;
  }

  //attempt retry
  if (is_retry && ((retry_timer - retry_timer_prev) > retry_freq) && TOGGLE_SATCOM)
  {
    attemptSend(true);

#if DEBUG
    Serial.println("ATTEMPTING RETRY");
#endif

    retry_timer_prev = retry_timer;
    check_retry();
  }

  //Make a health update and read any remote configuration datas
  if (FORCE_UPDATE || (update_flag && TOGGLE_UPDATES))
  {
    update_flag = false;
#if DEBUG
    Serial.println("CHECKING FOR UPDATES");
#endif
    update();
  }

  update_time();
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
uint32_t functionHandler(String cmd)
{
  uint32_t value = strtoul(cmd.c_str(), NULL, 10);
  return value;
}

/*****************************************************
 * Function: 
 * Description: 5 for state
*****************************************************/
bool verifyOSC(OSCMessage *current, uint8_t numFields)
{
  uint8_t count = 0;
  char buffer[120];
  char newMsg[100];
  memset(buffer, '\0', sizeof(buffer));
  memset(newMsg, '\0', sizeof(newMsg));
  //Generalize the checksum tto handle both state and GPS data

  String raw = get_data_value(current, numFields - 1);
  uint32_t checksum = functionHandler(raw);

#if DEBUG
  Serial.println();
  Serial.println("Message to verify: ");
  PrintMsg(*current);
  Serial.print("CHECKSUM: ");
  Serial.println(checksum);
#endif

  oscMsg_to_string(buffer, current);
  for (int i = 0; i < strlen(buffer); i++)
  {
    if (buffer[i] == ',')
    {
      count++;
      if (count == numFields)
        break;
    }
    newMsg[i] = buffer[i];
    crc.update(buffer[i]);
  }
  uint32_t test_checksum = crc.finalize();

#if DEBUG
  Serial.print("Calculated checksum: ");
  Serial.println(test_checksum);
#endif
  crc.reset();

  //packet intact
  if (test_checksum == checksum)
  {
#if DEBUG
    Serial.println("Checksums match, dropping checksum");
#endif
    current->empty();
    stringToOSCmsg(newMsg, current);
    return true;
  }

#if DEBUG
  Serial.println("Checksums don't match, tossing string");
#endif
  return false;
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void compareNMEA(OSCMessage *current, OSCMessage *best)
{
  char buf[5];

#if DEBUG
  Serial.println();
  Serial.println("COMPARING: ");
  PrintMsg(*current);
  Serial.println("---- AND ----");
  PrintMsg(*best);
  Serial.println();
#endif

  //necessary to go through character conversion becuase the value is stored in the OSC message as a string
  memset(buf, '\0', sizeof(buf));
  get_data_value(current, 7).toCharArray(buf, sizeof(buf));
  char modeCur = buf[0];
  memset(buf, '\0', sizeof(buf));

  get_data_value(best, 7).toCharArray(buf, sizeof(buf));
  char modeBest = buf[0];
  memset(buf, '\0', sizeof(buf));

#if DEBUG
  Serial.print("Value of current mode: ");
  Serial.println(modeCur);
  Serial.print("Value of best mode: ");
  Serial.println(modeBest);
#endif

  if (stringRank(modeCur) > stringRank(modeBest))
  {
    best->empty();
    deep_copy_message(current, best);

#if DEBUG
    Serial.println("New best position: ");
    PrintMsg(*best);
    Serial.println('\n');
#endif
  }
  else if (stringRank(modeCur) == stringRank(modeBest))
  {
    if (get_data_value(current, 9).toFloat() > get_data_value(best, 9).toFloat())
    {
      best->empty();
      deep_copy_message(current, best);
#if DEBUG
      Serial.println("Current string has a higher RTK ratio: ");
      PrintMsg(*best);
      Serial.println('\n');
#endif
    }
  }
}

/* Interrupt service routine for ring indicator interrupts*/
//ring indicator is a low pulse, followed by another low pulse 10 seconds later
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
 * The one bug I kow of in this code, occurs if the SD fails to open on upload. 
 * This will cause another node wake cycle to be written to the best data folder. 
 * This actually is not a critical bug at all but something to keep in mind for refractoring
 *************************************/
void attemptSend(bool bulk)
{
  String node_num;
  char packet[340];
  char buf[100];
  memset(packet, '\0', sizeof(packet));
  memset(buf, '\0', sizeof(buf));

  bool packetized = false;
  if (bulk)
    packetized = bulkUpload(packet, "/RETRY.TXT");
  else
    packetized = createPacket(packet, "/CYCLE.TXT");

  if (packetized && getNetwork()) //bulkUpload(bulk, "/RETRY.TXT")
  {
#if DEBUG
    Serial.print("PACKET CREATED: ");
    Serial.println(packet);
    Serial.println();
#endif

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
      node_num = "0";
      write_to_retry(packet, node_num);
    }
  }
  else
  {
    write_to_retry(packet, node_num);
#if DEBUG
    Serial.println("Network not available or failed to create a packet during this wake cycle, writing to retry folder...");
#endif
  }
  serialFlush();
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void write_to_retry(char packet[], String node_num)
{
#if DEBUG
  Serial.print("FAILED TO SEND, WRITING TO NODE ");
  Serial.print(node_num);
  Serial.println(" RETRY FOLDER.");
#endif
  char buf[100];
  char *token = strtok(packet, "#");
  while (token != NULL)
  {
    snprintf(buf, sizeof(buf), "%s", token);
    write_sd_str(node_num, "/RETRY.TXT", buf);
    memset(buf, '\0', sizeof(buf));
    token = strtok(NULL, "#");
  }
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
bool createPacket(char *buf, const char *file)
{
  String folder;
  String gpsStr;
  File e;

  char stateBuf[100];
  memset(stateBuf, '\0', sizeof(stateBuf));

  OSCMessage latestState;
  latestState.empty();

  OSCMessage cur;
  cur.empty();

  OSCMessage best("/GPS");
  best.add("X");
  best.add("X");
  best.add("X");
  best.add("X");
  best.add("X");
  best.add("X");
  best.add("X");
  best.add("X");
  best.add("0");

  String node = "0";
  folder = "NODE_" + node;
  e = SD.open(folder + file, FILE_READ);
  if (e.size() > 0)
  {
    while (e.available())
    {
      gpsStr = e.readStringUntil('\n');
      gpsStr.toCharArray(buf, gpsStr.length());
      stringToOSCmsg(buf, &cur);
      if (get_address_string(&cur).equals("/GPS")) //check ifthe string is a state message
      {
        compareNMEA(&cur, &best);
      }
      else if (get_address_string(&cur).equals("/State"))
      {
        latestState.empty();
        deep_copy_message(&cur, &latestState); //the lower the state strings are in the file the more recent, after encountering a new one simply update the state
      }
      cur.empty();
      memset(buf, '\0', sizeof(buf));
    }

    oscMsg_to_string(buf, &best);

#if DEBUG
    Serial.println("BEST STRING FOR THE CURRENT UPLOAD CYCLE");
    Serial.println(buf);
#endif

    if (latestState.bytes() > 8) //check if we have a state string, if so concatenate the state string with the best GPS string for this satcom cycle, an empty OSC string apparantly has 8 bytes
    {
      oscMsg_to_string(stateBuf, &latestState);

#if DEBUG
      Serial.println("BEST LATEST FOR THE CURRENT UPLOAD CYCLE");
      Serial.println(stateBuf);
#endif

      strcat(buf, "#");
      strcat(buf, stateBuf);
      memset(stateBuf, '\0', sizeof(stateBuf));
    }

    SD.remove(folder + file);
    e = SD.open(folder + "/CYCLE.TXT", FILE_WRITE);
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
 * Description:
*****************************************************/
bool bulkUpload(char *buf, const char *file)
{
  String folder;
  String gpsStr;
  File e, t;

  char buffer[100];
  memset(buffer, '\0', sizeof(buffer));
  memset(buf, '\0', sizeof(buf));

  String node = "0";
  folder = "NODE_" + node;

  t = SD.open(folder + "/TEMP.TXT", FILE_WRITE);
  if (!t)
  {
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
          t.println(gpsStr);
        }
        else
        {
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
      t.close();
      return false;
    }
  }

  SD.remove(folder + file);                       //remove the old retry folder
  e = SD.open(folder + "/RETRY.TXT", FILE_WRITE); //create a new retry folder
  if (e)
  {
#if DEBUG
    Serial.println("New copy of " + folder + "/RETRY.TXT created...");
#endif
  }
  else
  { //it is very important that I verify that the program can recover from this, it will create this folder whenever it uploads, so I think im good as long as I do not delete the contents of temp
    t.close();
    return false;
  }

  //Create a temp file so I can copy the retry file over, delete it, then create a new retry file copy the contents of temp over. Terribly inefficient work around becuase file renaming is not supported.
  t = SD.open(folder + "/TEMP.TXT", FILE_READ);
  if (!t)
  {
    return false;
  }

  if (t.size() > 0)
  {
#if DEBUG
    Serial.println("Copying data from TEMP over to the new RETRY file");
#endif
    memset(buffer, '\0', sizeof(buffer));
    while (t.available())
    {
      gpsStr = t.readStringUntil('\n');
      gpsStr.trim();
      gpsStr.toCharArray(buffer, sizeof(buffer)); //find where a new byte is being added!!!
      write_sd_str(node, "/RETRY.TXT", buffer);
      memset(buffer, '\0', sizeof(buffer));
    }
  }
  SD.remove(folder + "/TEMP.TXT");
  e.close();
  t.close();
  return true;
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
  sprintf(status, "%d", satcom_freq);
  strcat(status, ",");
  SDsize(root, size);
  sprintf(status + strlen(status), "%d", size);
  nextUpload = (satcom_freq * NODE_TIMER) - (satcom_count * NODE_TIMER) + (NODE_TIMER - ((millis() - last_message) / 60000));
  int seconds = nextUpload % 60;
  int minutes = (nextUpload / 60) % 60;
  strcat(status, ",");
  sprintf(status + strlen(status), "%d", minutes);
  strcat(status, ":");
  sprintf(status + strlen(status), "%d", seconds);
#if DEBUG
  String str_status = String(status);
  Serial.print("HUB STATUS: ");
  Serial.println(str_status);
#endif
}

/*****************************************************
 * Function: 
 * Description: HERE
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
    //err = modem.sendReceiveSBDBinary(buffer, bufferSize, buffer, bufferSize);
    collectStatus(status);
    err = modem.sendReceiveSBDText(status, (uint8_t *)buffer, bufferSize);
    if (err == ISBD_SUCCESS && checkBuf(buffer))
    {
      sscanf(buffer, "%d", &ret);
      if (ret > 0 && ret < 672) //minimum satcom uploads of one upload per week
      {
        satcom_freq = ret;
        Serial.print("System will now be sending satcom uploads every ");
        Serial.print(ret);
        Serial.println(" messages");
      }

      Serial.println();
      Serial.print("Messages remaining to be retrieved: ");
      Serial.println(modem.getWaitingMessageCount());
    }
  }
  //force a string upload
  satcom_count = satcom_freq;
  attachInterrupt(digitalPinToInterrupt(RING_INDICATOR_PIN), toggleUpdate, FALLING);
  serialFlush();
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
  String folder;
  File e;
  String node = "0"; //REFRACTOR
  folder = "NODE_" + node;
  e = SD.open(folder + "/RETRY.TXT", FILE_READ);
  if (e.size() > 0)
    is_retry = true;
  else
    is_retry = false;

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
void gpsProc(OSCMessage &msg)
{
#if DEBUG
  PrintMsg(msg);
#endif
  String node_num = "0";
  char write_buffer[100];
  memset(write_buffer, '\0', sizeof(write_buffer));
  oscMsg_to_string(write_buffer, &msg);
  write_sd_str(node_num, "/GPS_LOGS.TXT", write_buffer);
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void gpsBest(OSCMessage &msg)
{
#if DEBUG
  Serial.println();
  Serial.println("BEST STRING FOR UPLOAD CYCLE: ");
  PrintMsg(msg);
#endif
  String node_num = "0";
  char write_buffer[100];
  memset(write_buffer, '\0', sizeof(write_buffer));
  oscMsg_to_string(write_buffer, &msg);
  write_sd_str(node_num, "/CYCLE.TXT", write_buffer);
  satcom_count++;
}

/*****************************************************
 * Function:         
 * Description:   
*****************************************************/
void stateProc(OSCMessage &msg)
{
#if DEBUG
  Serial.println();
  PrintMsg(msg);
#endif
  String node_num = "0";
  char write_buffer[100];
  memset(write_buffer, '\0', sizeof(write_buffer));
  oscMsg_to_string(write_buffer, &msg);
  write_sd_str(node_num, "/S_LOGS.TXT", write_buffer);
  memset(write_buffer, '\0', sizeof(write_buffer)); //prevent newline from making spaces in write_sd
  oscMsg_to_string(write_buffer, &msg);
  write_sd_str(node_num, "/CYCLE.TXT", write_buffer);
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

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void PrintMsg(OSCMessage &msg)
{
  print_message(&msg);
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