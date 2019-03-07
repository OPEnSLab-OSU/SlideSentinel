/********************** PROGRAM DEXCRIPTION *********************
 
 ****************************************************************/

/* To Do: 
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

  //ROCKBLOCK TESTING
  //RockBLOCK Plus  (OFF):< -1.5V     (ON):< 1.5
  //ON/OFF pin feather:  9, digital pin 9
  //Net Av pin: A5, digital pin 19
  //RI: A4, digital pin 18

  /*NOTES ON NET AV
    When a network is available, the Network Available pinout is high.
    When a network isn't available, the Network Available pinout is low.
  

//osc for arduino sending data description
//Talk to Storm about what data we want, get in touch with Kevin about configuring the spreadsheet
//network availability pin on ROCKBLOCK, unfortunatley if the network is not available and we try to send we still consume credits

IMPORTANT URL'S:
https://rockblock.rock7.com/Operations
https://postproxy.azurewebsites.net



1. still need logic for selecting the string to be sent
2. ring indicator, messages to the ROCKBLOCK to increase or decrease send frequency
3. basically done after that!

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
#define NODE_NUM 5
#define CONFIG_UPDATE
#define NET_AV 19
#define ON_OFF 9

//timer definitions
#define CPU_HZ 48000000
#define TIMER_PRESCALER_DIV 1024
#define IridiumSerial Serial1

//timer functions
void startTimer(int frequencyHz);
void setTimerFrequency(int frequencyHz);
void TC3_Handler();

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
void toggleRockblock(bool on);
uint8_t getSignalQuality();
uint8_t getNetwork();
void PrintBund(OSCMessage &msg);

//Initialize the satcom module
IridiumSBD modem(IridiumSerial);
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

char buffer[200];
bool success;
/**************************************
 * 
 * 
 *************************************/
void setup()
{
  //start with the ROCKBLOCK turned ON!
  toggleRockblock(true);
  Serial.begin(115200); //Opens the main serial port to communicate with the computer
  while (!Serial)
    ;
  Loom_begin();
  Serial2_setup(); //Serial port for communicating with Freewave radios
  Serial.println("serial configured... ");
  setup_sd();
  initialize_nodes();
  memset(inputBuf, '\0', sizeof(inputBuf));
  //initRockblock();

  satcom_freq = 30;     //number of seconds between ROCKBLOCK uploads
  retry_freq = 5;       //number of seconds to wait if no network is available
  update_freq = 259200; //check for updates once every three days?     YUCK I DONT LIKE THIS METHOD

  satcom_timer = 0;
  retry_timer = 0;
  update_timer = 0;

  startTimer(1);
  Serial.print("Initializing satcom to upload once every ");
  Serial.print((float)satcom_freq / 60);
  Serial.println(" minutes...");
  Serial.println("ALL SYSTEMS CONFIGURED... ");

  serialFlush();
  success = true;
  memset(buffer, '\0', sizeof(buffer));
}

//TRYING A GLOBAL BUNDLE 
OSCBundle bundleIN;

/*
GOOD
This is the string:/GPS,sssssss185149.0004434.001393512316.842463694.354D0.00.0
2F 47 50 53 2C 73 73 73 73 73 73 73 31 38 35 31 34 39 2E 30 30 30 34 34 33 34 2E 30 30 31 33 39 33 35 31 32 33 31 36 2E 38 34 32 34 36 33 36 39 34 2E 33 35 34 44 30 2E 30 30 2E 30 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 

2F 47 50 53 2C 73 73 73 73 73 73 73 31 38 35 31 34 39 2E 30 30 30 34 34 33 34 2E 30 30 31 33 39 33 35 31 32 33 31 36 2E 38 34 32 34 36 33 36 39 34 2E 33 35 34 44 30 2E 30 30 2E 30 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
BAD
This is the string:/GPS,sssssss185149.0004434.001393512316.842463694.354D0.00.0
*/

/**************************************
 *    Test what the dispatch function does with a bundle that has multiple messages, may there is some looping functionality?
 * 
 *************************************/
void loop()
{
  //OSCBundle bndl;
  TcCount16 *TC = (TcCount16 *)TC3;
  //AWESOME: since the SD takes a substantial amount of time and I will be receiving strings quickly I can buffer them in individual messages in a receive bundle then call dispatch once!!!
  //bndl.add("/GPS").add((const char *)node_num).add((const char *)UTC).add((const char *)lat).add((const char *)lon).add((const char *)alt).add((const char *)mode).add((const char *)age).add((const char *)ratio);
  //bndl.add("/GPS").add((const char *)node_num2).add((const char *)UTC2).add((const char *)lat2).add((const char *)lon2).add((const char *)alt2).add((const char *)mode2).add((const char *)age2).add((const char *)ratio2);
  // bndl.add("/State").add((const char *)node_num3).add((const char *)UTC3).add((const char *)x).add((const char *)y).add((const char *)z).add((const char *)voltage).add((const char *)temp);
  //String str = "test string...";
  unsigned long internal_time_cur;
  unsigned long internal_time_prev;

  if (Serial2.available())
  {
    TC->INTENCLR.bit.MC0 = 1;

    //OSCBundle bundleIN;
    //internal_time_prev = millis();
    internal_time_cur = millis();
    bool str_flag = false;

    while (millis() - internal_time_cur < 1000)
    {

      bundleIN.empty();
      int size;
      char input;

      if (Serial2.available())
      {
        // Serial.println("Reading data");
        if (input = Serial2.read() == (byte)2)
        {
          while (input != (byte)4)
          {
            if (Serial2.available())
            {
              input = Serial2.read();
              if (input == 4)
              {
                str_flag = true;
                break;
              }
              bundleIN.fill(input);
              append(buffer, input);
            }
          }
        }
        internal_time_cur = millis();
        Serial.print("This is the string:");
        Serial.println(buffer);
       /* printHex(buffer);
        
        
        /*int i; 
        for(i = 0; i < )
          bundleIN.fill(input);

        //try declaring a static bundle so we dont use the destructor
*/

        memset(buffer, '\0', sizeof(buffer));
      }

      //invlad bundle: error 2

      if (str_flag)
      {
        //Serial.print("String:");
        //bundleIN.send(Serial);
        if (!bundleIN.hasError())
        {
          //bundleIN.send(Serial);
          Serial.println("      GOOD");
          bundleIN.dispatch("/GPS", PrintBund);
          // count++;
        }
        else
        {
          // bad_count++;
          Serial.print("     ERROR: ");
          Serial.println(bundleIN.getError());
        }
      }

      str_flag = false;
    }
    Serial.println("outside of read");
    //internal_time_cur = millis();

    /*
    Serial.print("This number of successful packets: ");
    Serial.println(count);

    Serial.print("This number of failed packets: ");
    Serial.println(bad_count);

    Serial.print("success ratio: ");
    Serial.println((float)count / (count + bad_count));
*/
    /*
    char buffer[200];
    memset(buffer, '\0', sizeof(buffer));
    readBuffer(buffer);
    success = !success;
    Serial.print("changing value of success... Now: ");
    Serial.println(success);

    char large_buf[120];
    memset(large_buf, '\0', sizeof(large_buf));
    convert_OSC_bundle_to_string(&bndl, large_buf);
    Serial.println(large_buf);

    bndl.dispatch("/GPS", gpsProc);
    bndl.dispatch("/State", stateProc);
    */

    //Serial.println("Done processing...");
    //Re-enable rockblock
    TC->INTENSET.bit.MC0 = 1;
  }

  /*
  if ((satcom_timer - satcom_timer_prev) > satcom_freq)
  {                           //attempt to send via the ROCKBLOCK
    TC->INTENCLR.bit.MC0 = 1; //make sure to disable interrupts and clear the timer when done
    //attemptSendToday(str.c_str());
    Serial.println("Upload to SATCOM!!!");
    satcom_timer_prev = satcom_timer;
    TC->INTENSET.bit.MC0 = 1;
  }

  if (is_retry && ((retry_timer - retry_timer_prev) > retry_freq))
  {
    TC->INTENCLR.bit.MC0 = 1;
    attemptSendRetry();
    retry_timer_prev = retry_timer;
    TC->INTENSET.bit.MC0 = 1;
  }
*/
  /*************************
   * Due to the fact that the unit is passively off to save energy we cannot use ring indicators to determine if a request was sent
   * We will thus periodically perform a mailbox check, which consumes a signle credit at some configuration update interval
   **************************/
  /*if (update_timer == update_freq)
  {
    TC->INTENCLR.bit.MC0 = 1;
    update();
    TC->INTENSET.bit.MC0 = 1;
  }
  
  //update_time();*/
}
/*************************************
 * 
 * 
 *************************************/
void PrintBund(OSCMessage &msg)
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
void update_time()
{
  satcom_timer = millis();
  retry_timer = millis();
  update_timer = millis();
}
/*************************************
 * 
 * 
 *************************************/
void attemptSendToday(const char *message)
{
  toggleRockblock(true); //turn on the ROCKBLOCK
  if (getNetwork())
  { //if network is available we send, included because Rockblock will sometimes consume credits if network is not available but a request to send still occurs, no data will get uploaded.
    //CONSTRUCT PACKET                 //check if there is data that we can send from each folder
    if (successfulUpload(message))
    { //check if test string failed to send
      Serial.println("Successfully sent message!");
      is_retry = false;
    }
    else
    { //message failed to send, COPY OVER MESSAGE TO RETRY BUFFER, first check to see if the buffer is empty.
      is_retry = true;
      Serial.println("Failed configuring retry....");
    }
  }
  else //network was not initially available, attempt to retry, COPY OVER MESSAGE TO RETRY BUFFER, first check to see if the buffer is empty.
  {
    Serial.println("Network not available configuring retry....");
    is_retry = true;
  }
  toggleRockblock(false);
  satcom_timer = 0;
}

/*************************************
 * Test how many credits Iridium receive library consumes...
 * Use it here in a case, statement, some format in maps to some upload frequency value...
 * 
 *************************************/
void update()
{
}

/*************************************
 * TO DO:
 * - If we need to retry, we reset the retry timer. 
 * - Increment the retry timer in timer ISR 
 * - copy over the string we want to send to some retry buffer, which the retry send function will use
 * - Last thing to do will be composing the string to send, we need to verify the best string
 * - Need to figure out some update configuration timer on the hub as well
 *************************************/
void attemptSendRetry()
{
  Serial.println("Performing retry logic.....");
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
uint8_t getNetwork()
{
  Serial.println("Getting network availability");
  return digitalRead(NET_AV);
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
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
  Serial.println("Configuring ON/OFF, Ring Indicator and Network availability..."); //STILL NEED TO CONFIGURE RING INDICATOR!!
  pinMode(ON_OFF, OUTPUT);                                                          //ON/OFF control
  pinMode(NET_AV, INPUT);                                                           //Network availability
  toggleRockblock(false);
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

void printHex(char buffer[])
{
  int i = 0;
  for (i = 0; i < 200 /*sizeof(buffer)*/; i++)
  {
    Serial.print(buffer[i], HEX);
    Serial.write(' ');
  }
  Serial.println();
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
    if (Serial.available()) //if there is something in the buffer read it
    {
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
void setTimerFrequency(int frequencyHz)
{
  int compareValue = (CPU_HZ / (TIMER_PRESCALER_DIV * frequencyHz)) - 1;
  TcCount16 *TC = (TcCount16 *)TC3;
  // Make sure the count is in a proportional position to where it was
  // to prevent any jitter or disconnect when changing the compare value.
  TC->COUNT.reg = map(TC->COUNT.reg, 0, TC->CC[0].reg, 0, compareValue);
  TC->CC[0].reg = compareValue;
  Serial.println(TC->COUNT.reg);
  Serial.println(TC->CC[0].reg);
  while (TC->STATUS.bit.SYNCBUSY == 1)
    ;
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void startTimer(int frequencyHz)
{
  REG_GCLK_CLKCTRL = (uint16_t)(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TCC2_TC3);
  while (GCLK->STATUS.bit.SYNCBUSY == 1)
    ; // wait for sync

  TcCount16 *TC = (TcCount16 *)TC3;

  TC->CTRLA.reg &= ~TC_CTRLA_ENABLE;
  while (TC->STATUS.bit.SYNCBUSY == 1)
    ; // wait for sync

  // Use the 16-bit timer
  TC->CTRLA.reg |= TC_CTRLA_MODE_COUNT16;
  while (TC->STATUS.bit.SYNCBUSY == 1)
    ; // wait for sync

  // Use match mode so that the timer counter resets when the count matches the compare register
  TC->CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;
  while (TC->STATUS.bit.SYNCBUSY == 1)
    ; // wait for sync

  // Set prescaler to 1024
  TC->CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1024;
  while (TC->STATUS.bit.SYNCBUSY == 1)
    ; // wait for sync

  setTimerFrequency(frequencyHz);

  // Enable the compare interrupt
  TC->INTENSET.reg = 0;
  TC->INTENSET.bit.MC0 = 1;

  NVIC_EnableIRQ(TC3_IRQn);

  TC->CTRLA.reg |= TC_CTRLA_ENABLE;
  while (TC->STATUS.bit.SYNCBUSY == 1)
    ; // wait for sync
}

/*****************************************************
 * Function: 
 * Description:
*****************************************************/
void TC3_Handler()
{
  TcCount16 *TC = (TcCount16 *)TC3;
  if (TC->INTFLAG.bit.MC0 == 1)
  {
    TC->INTFLAG.bit.MC0 = 1;
    satcom_timer++;
    //config_timer++;
    //update_timer++;
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
