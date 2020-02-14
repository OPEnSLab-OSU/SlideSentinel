#include "Battery.h"
#include "ComController.h"
#include "Controller.h"
#include "FreewaveRadio.h"
#include "GNSSController.h"
#include "IMUController.h"
#include "MAX3243.h"
#include "MAX4280.h"
#include "PMController.h"
#include "RTClibExtended.h"
#include "SN74LVC2G53.h"
#include "VoltageReg.h"
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

// Test Toggle
#define ADVANCED true

// COMMUNICATION CONTROLLER
#define RADIO_BAUD 115200
#define CLIENT_ADDR 1
#define SERVER_ADDR 2
#define RST 6
#define CD 10
#define IS_Z9C true
#define SPDT_SEL 14
#define FORCEOFF_N A5

Freewave radio(RST, CD, IS_Z9C);
SN74LVC2G53 mux(SPDT_SEL, -1);
MAX3243 max3243(FORCEOFF_N);
ComController *comController;

// Make sure you change the com level to TTL on the receiver!!!
// POWER MANAGEMENT CONTROLLER
#define SD_CS 18
#define VCC2_EN 13
#define MAX_CS 9
#define BAT 15

PoluluVoltageReg vcc2(VCC2_EN);
MAX4280 max4280(MAX_CS, &SPI);
Battery batReader(BAT);
PMController pmController(&max4280, &vcc2, &batReader, false, true);

// GLOBAL DOCUMENT
// StaticJsonDocument<RH_SERIAL_MAX_MESSAGE_LEN> doc;
// can only transmit 64 bytes packets at a time
StaticJsonDocument<1000> doc;

// Instatiate ACCELEROMETER Object
#define ACCEL_INT A3
IMUController *imuController;

// Instatiate RTC Object
#define RTC_INT 5
RTC_DS3231 RTC_DS;
volatile int HR = 8; //  These should not be volatile
volatile int MIN = 0;
volatile int awakeFor = 20;
#define RTC_WAKE_PERIOD                                                        \
  1 // Interval to wake and take sample in Min, reset alarm based on this period
    // (Bo - 5 min), 15 min

// GNSS Init
#define GNSS_TX 11
#define GNSS_RX 12
#define GNSS_BAUD 115200
GNSSController *gnssController;

Uart Serial2(&sercom1, GNSS_RX, GNSS_TX, SERCOM_RX_PAD_3, UART_TX_PAD_0);
void SERCOM1_Handler() { Serial2.IrqHandler(); }

// void mmaSetupSlideSentinel() {
//   if (!mma.begin()) {
//     Serial.println("Unable to find MMA8451");
//     while (1)
//       ;
//   }

//   mma.setRange(MMA8451_RANGE_2_G);
//   mma.setDataRate(MMA8451_DATARATE_6_25HZ);
// }

// void configInterrupts(Adafruit_MMA8451 device) {
//   uint8_t dataToWrite = 0;
//   // MMA8451_REG_CTRL_REG2
//   // sysatem control register 2

//   // dataToWrite |= 0x80;    // Auto sleep/wake interrupt
//   // dataToWrite |= 0x40;    // FIFO interrupt
//   // dataToWrite |= 0x20;    // Transient interrupt - enabled
//   // dataToWrite |= 0x10;    // orientation
//   // dataToWrite |= 0x08;    // Pulse interrupt
//   // dataToWrite |= 0x04;    // Freefall interrupt
//   // dataToWrite |= 0x01;    // data ready interrupt, MUST BE ENABLED FOR USE
//   // WITH ARDUINO

//   // MMA8451_REG_CTRL_REG3
//   // Interrupt control register
//   dataToWrite |=
//       0x80; // FIFO gate option for wake/sleep transition, default 0, Asserting
//             // this allows the accelerometer to collect data the moment an
//             // impluse happens and preserve that data because the FIFO buffer is
//             // blocked. Thus at the end of a wake cycle the data from the
//             // initial transient wake up is still in the buffer
//   dataToWrite |= 0x40; // Wake from transient interrupt enable
//   // dataToWrite |= 0x20;    // Wake from orientation interrupt enable
//   // dataToWrite |= 0x10;    // Wake from Pulse function enable
//   // dataToWrite |= 0x08;    // Wake from freefall/motion decect interrupt
//   // dataToWrite |= 0x02;    // Interrupt polarity, 1 = active high
//   dataToWrite |= 0x00; // (0) Push/pull or (1) open drain interrupt, determines
//                        // whether bus is driven by device, or left to hang

//   device.writeRegister8_public(MMA8451_REG_CTRL_REG3, dataToWrite);

//   dataToWrite = 0;

//   // MMA8451_REG_CTRL_REG4
//   // Interrupt enable register, enables interrupts that are not commented

//   // dataToWrite |= 0x80;    // Auto sleep/wake interrupt
//   // dataToWrite |= 0x40;    // FIFO interrupt
//   dataToWrite |= 0x20; // Transient interrupt - enabled
//   // dataToWrite |= 0x10;    // orientation
//   // dataToWrite |= 0x08;    // Pulse interrupt
//   // dataToWrite |= 0x04;    // Freefall interrupt
//   dataToWrite |=
//       0x01; // data ready interrupt, MUST BE ENABLED FOR USE WITH ARDUINO
//   device.writeRegister8_public(MMA8451_REG_CTRL_REG4, dataToWrite | 0x01);

//   dataToWrite = 0;

//   // MMA8451_REG_CTRL_REG5
//   // Interrupt pin 1/2 configuration register, bit == 1 => interrupt to pin 1
//   // see datasheet for interrupt's description, threshold int routed to pin 1
//   // comment = int2, uncoment = int1

//   // dataToWrite |= 0x80;    // Auto sleep/wake
//   // dataToWrite |= 0x40;    // FIFO
//   dataToWrite |= 0x20; // Transient, asserting this routes transients interrupts
//                        // to INT1 pin
//   // dataToWrite |= 0x10;    // orientation
//   // dataToWrite |= 0x08;    // Pulse
//   // dataToWrite |= 0x04;    // Freefall
//   // dataToWrite |= 0x01;    // data ready

//   device.writeRegister8_public(MMA8451_REG_CTRL_REG5, dataToWrite);

//   dataToWrite = 0;

//   // MMA8451_REG_TRANSIENT_CFG
//   // dataToWrite |= 0x10;  // Latch enable to capture accel values when
//   // interrupt occurs
//   dataToWrite |= 0x08; // Z transient interrupt enable
//   dataToWrite |= 0x04; // Y transient interrupt enable
//   dataToWrite |= 0x02; // X transient interrupt enable
//   // dataToWrite |= 0x01;    // High-pass filter bypass
//   device.writeRegister8_public(MMA8451_REG_TRANSIENT_CFG, dataToWrite);

//   dataToWrite = 0;

//   // MMA8451_REG_TRANSIENT_THS
//   // Transient interrupt threshold in units of .06g
//   // Acceptable range is 1-127
//   dataToWrite = 0x1F;
//   device.writeRegister8_public(MMA8451_REG_TRANSIENT_THS, dataToWrite);

//   dataToWrite = 0;

//   // MMA8451_REG_TRANSIENT_CT  0x20
//   dataToWrite =
//       0; // value is 0-255 for numer of counts to debounce for, depends on ODR
//   device.writeRegister8_public(MMA8451_REG_TRANSIENT_CT, dataToWrite);

//   dataToWrite = 0;
// }

// void accelInt() {
//   detachInterrupt(digitalPinToInterrupt(ACCEL_INT));
//   Serial.println("Accelerometer Wake");
//   attachInterrupt(digitalPinToInterrupt(ACCEL_INT), accelInt, CHANGE);
// }

void clearRTCAlarm() {
  // clear any pending alarms
  RTC_DS.armAlarm(1, false);
  RTC_DS.clearAlarm(1);
  RTC_DS.alarmInterrupt(1, false);
  RTC_DS.armAlarm(2, false);
  RTC_DS.clearAlarm(2);
  RTC_DS.alarmInterrupt(2, false);
}

void readGNSS() {
  if (Serial2.available()) {
    Serial.print((char)Serial2.read());
  }
}

bool writeData(char *file, char *data) {
  auto sdFile = SD.open(file, FILE_WRITE);
  if (!sdFile)
    return false;

  sdFile.print(data);
  sdFile.close();
  return true;
}

void initializeRTC() {
  if (!RTC_DS.begin()) {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }
  // clear any pending alarms
  clearRTCAlarm();

  // Set SQW pin to OFF (in my case it was set by default to 1Hz)
  // The output of the DS3231 INT pin is connected to this pin
  // It must be connected to arduino Interrupt pin for wake-up
  RTC_DS.writeSqwPinMode(DS3231_OFF);

  // Set alarm1
  clearRTCAlarm();
}

void rtcInt() {
  detachInterrupt(digitalPinToInterrupt(RTC_INT));
  Serial.println("RTC Wake");
  attachInterrupt(digitalPinToInterrupt(RTC_INT), rtcInt, FALLING);
}

void setRTCAlarm() {
  DateTime now = RTC_DS.now(); // Check the current time
  MIN = (now.minute() + RTC_WAKE_PERIOD) %
        60; // wrap-around using modulo every 60 sec
  HR = (now.hour() + ((now.minute() + RTC_WAKE_PERIOD) / 60)) %
       24; // quotient of now.min+periodMin added to now.hr, wraparound every
           // 24hrs

  Serial.print("Setting Alarm 1 for: ");
  Serial.print(HR);
  Serial.print(":");
  Serial.println(MIN);

  // Set alarm1
  RTC_DS.setAlarm(ALM1_MATCH_HOURS, MIN, HR, 0); // set your wake-up time here
  RTC_DS.alarmInterrupt(1, true); // code to pull microprocessor out of sleep is
                                  // tied to the time -> here
  attachInterrupt(digitalPinToInterrupt(RTC_INT), rtcInt, FALLING);
}

bool setup_sd() {
  Serial.println("Initializing SD card...");

  if (!SD.begin(SD_CS)) {
    Serial.println("SD Initialization failed!");
    return false;
  }
  Serial.println("SD initialization complete");
  return true;
}

void advancedTest() {
  char cmd;
  if (Serial.available()) {
    DateTime now = RTC_DS.now();
    cmd = Serial.read();
    switch (cmd) {
    case '1':
      pmController.enableGNSS();
      break;
    case '2':
      pmController.disableGNSS();
      break;
    case '3':
      pmController.enableRadio();
      break;
    case '4':
      pmController.disableRadio();
      break;
    case '5':
      Serial1.println(
          "TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST "
          "TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST "
          "TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST "
          "TEST TEST TEST TEST TEST TEST TEST TEST TEST");
      Serial.println("Writing data out...");
      break;
    case '6':
      Serial.println("RADIO -----> FEATHER M0 (A0 LOW)");
      mux.comY1();
      break;
    case '7':
      Serial.println("RADIO -----> GNSS RECEIVER (A0 HIGH)");
      mux.comY2();
      break;
    case '8':
      Serial.println("RS232 ---> OFF (Driving it LOW)");
      max3243.disable();
      break;
    case '9':
      Serial.println("RS232 ---> ON (Driving it HIGH)");
      max3243.enable();
      break;
    case 'r':
      Serial.println("Resetting the radio, DRIVING RST LOW");
      comController->resetRadio();
      break;
    case 't':
      Serial.print("STATE of CD pin: ");
      if (comController->channelBusy())
        Serial.println("BUSY");
      else
        Serial.println("NOT BUSY");
      break;
    case 'w':
      setRTCAlarm();
      break;
    case 'j':
      Serial.print("RTC Time is: ");
      Serial.print(now.hour(), DEC);
      Serial.print(':');
      Serial.print(now.minute(), DEC);
      Serial.print(':');
      Serial.println(now.second(), DEC);
      break;
    case 's':
      Serial.println("Sleeping");
      pmController.disableGNSS();
      pmController.disableRadio();
      pmController.sleep();

      Serial.begin(115200);
      pmController.enableGNSS();
      delay(2000);
      pmController.disableGNSS();
      Serial.println("Awake");
      break;
    case 'd':
      Serial.print("reading battery voltage: ");
      Serial.println(pmController.readBat());
      break;
    case 'f':
      Serial.print("reading battery voltage string: ");
      char volt[20];
      memset(volt, '\0', sizeof(char) * 20);
      pmController.readBatStr(volt);
      Serial.println(volt);
      break;
    case 'x':
      Serial.println("Turning off VCC2");
      vcc2.disable();
      break;
    case 'y':
      Serial.println("Turning on VCC2");
      vcc2.enable();
      break;
    case 'o':
      char buffer[RH_SERIAL_MAX_MESSAGE_LEN];
      memset(buffer, '\0', sizeof(char) * RH_SERIAL_MAX_MESSAGE_LEN);
      Serial.println("Turning BASE station ON");
      comController->request(doc);
      serializeJson(doc, buffer);
      Serial.print("Config Received:    ");
      Serial.println(buffer);
      pmController.enableGNSS();
      delay(500);
      break;

    case 'p':
      Serial.println("Turning BASE station OFF");
      char test[] = "{\"sensor\":\"gps\",\"time\":1351824120}";
      deserializeJson(doc, test);
      comController->upload(doc);
      pmController.disableGNSS();
      delay(500);
      break;
    }
  }

  if (Serial1.available()) {
    Serial.print(Serial1.read());
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  // Place instatiation here, Serial1 is not in the same compilation unit as
  static ComController _comController(&radio, &max3243, &mux, &Serial1,
                                      RADIO_BAUD, CLIENT_ADDR, SERVER_ADDR);
  comController = &_comController;

  static GNSSController _gnssController(&Serial2, GNSS_BAUD, GNSS_RX, GNSS_TX);
  gnssController = &_gnssController;



  // SPI INIT
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV8);

  //Init IMUController
  static IMUController _imuController(ACCEL_INT, 0x1F);
  imuController = &_imuController;

  // RTC INIT
  pinMode(RTC_INT, INPUT_PULLUP); // active low interrupts
  initializeRTC();

  // must be done after first call to attac1hInterrupt()
  pmController.init();

  // SD Card Initialization
  pinMode(SD_CS, OUTPUT);

  // CONSIDER a pointer to the file to be written to currently in the state
  setup_sd();
}

void loop() {
  while (1) {

    if (ADVANCED)
      advancedTest();

    if(gnssController->poll(doc)){
      // serializeJsonPretty(doc, Serial);
      doc.clear();
    }

    if(imuController->getFlag())
      Serial.println("accel int");
    
  }
}
