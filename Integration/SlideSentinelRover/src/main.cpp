#include "Battery.h"
#include "COMController.h"
#include "ConManager.h"
#include "Controller.h"
#include "FSController.h"
#include "FreewaveRadio.h"
#include "GNSSController.h"
#include "IMUController.h"
#include "MAX3243.h"
#include "MAX4280.h"
#include "PMController.h"
#include "RTCController.h"
#include "RTClibExtended.h"
#include "SN74LVC2G53.h"
#include "SSModel.h"
#include "VoltageReg.h"
#include "config_2.0.0.h"
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

// TODO use a supported RTC library for DS3231
/****** Test Routine ******/
#define ADVANCED2 true

/****** Model ******/
SSModel model(CLIENT_ADDR);
ConManager manager;

/****** FSController Init ******/
FSController fsController(SD_CS, SD_RST);

/****** ComController Init ******/
Freewave radio(RST, CD, IS_Z9C);
SN74LVC2G53 mux(SPDT_SEL, -1);
MAX3243 max3243(FORCEOFF_N);
COMController *comController;

/****** PMController Init ******/
PoluluVoltageReg vcc2(VCC2_EN);
MAX4280 max4280(MAX_CS, &SPI);
Battery batReader(BAT);
PMController pmController(max4280, vcc2, batReader, false, true);

/****** IMUController Init ******/
IMUController imuController(ACCEL_INT, INIT_SENSITIVITY);

/****** GNSSController Init ******/
Uart Serial2(&sercom1, GNSS_RX, GNSS_TX, SERCOM_RX_PAD_3, UART_TX_PAD_0);
void SERCOM1_Handler() { Serial2.IrqHandler(); }
GNSSController *gnssController;

/****** RTCController Init ******/
RTC_DS3231 RTC_DS;
RTCController rtcController(RTC_DS, RTC_INT, INIT_WAKETIME, INIT_SLEEPTIME);

// bool pollFlag = false;
// void advancedTest() {
//   char cmd;
//   char test[] = "{\"sensor\":\"gps\",\"time\":1351824120}";

//   char fileName[30];
//   memset(fileName, '\0', sizeof(char) * 30);

//   if (Serial.available()) {
//     cmd = Serial.read();
//     switch (cmd) {
//     case '1':
//       pmController.enableGNSS();
//       break;
//     case '2':
//       pmController.disableGNSS();
//       break;
//     case '3':
//       pmController.enableRadio();
//       break;
//     case '4':
//       pmController.disableRadio();
//       break;
//     case '5':
//       Serial1.println(
//           "TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST "
//           "TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST "
//           "TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST "
//           "TEST TEST TEST TEST TEST TEST TEST TEST TEST");
//       Serial.println("Writing data out...");
//       break;
//     case '6':
//       Serial.println("RADIO -----> FEATHER M0 (A0 LOW)");
//       mux.comY1();
//       break;
//     case '7':
//       Serial.println("RADIO -----> GNSS RECEIVER (A0 HIGH)");
//       mux.comY2();
//       break;
//     case '8':
//       Serial.println("RS232 ---> OFF (Driving it LOW)");
//       max3243.disable();
//       break;
//     case '9':
//       Serial.println("RS232 ---> ON (Driving it HIGH)");
//       max3243.enable();
//       break;
//     case 'q':
//       Serial.println("Resetting the radio, DRIVING RST LOW");
//       comController->resetRadio();
//       break;
//     case 'w':
//       Serial.print("STATE of CD pin: ");
//       if (comController->channelBusy())
//         Serial.println("BUSY");
//       else
//         Serial.println("NOT BUSY");
//       break;
//     case 'e':
//       Serial.print("Printing timestamp: ");
//       Serial.println(rtcController.getTimestamp());
//       break;
//     case 't':
//       Serial.println("Sleeping");
//       pmController.disableGNSS();
//       pmController.disableRadio();
//       pmController.sleep();

//       Serial.begin(115200);
//       pmController.enableGNSS();
//       delay(2000);
//       pmController.disableGNSS();
//       Serial.println("Awake");
//       break;
//     case 'y':
//       Serial.print("reading battery voltage: ");
//       Serial.println(pmController.readBat());
//       break;
//     case 'u':
//       Serial.print("reading battery voltage string: ");
//       Serial.println(pmController.readBatStr());
//       break;
//     case 'i':
//       Serial.println("Turning off VCC2");
//       vcc2.disable();
//       break;
//     case 'o':
//       Serial.println("Turning on VCC2");
//       vcc2.enable();
//       break;
//     case 'p':
//       char buffer[RH_SERIAL_MAX_MESSAGE_LEN];
//       memset(buffer, '\0', sizeof(char) * RH_SERIAL_MAX_MESSAGE_LEN);
//       Serial.println("Turning BASE station ON");
//       comController->request(model);
//       pmController.enableGNSS();
//       delay(500);
//       break;

//     case 's':
//       Serial.println("Creating new directory");
//       Serial.println(rtcController.getTimestamp());
//       fsController.setupWakeCycle(rtcController.getTimestamp(),
//                                   gnssController->getFormat());
//       break;

//     case 'd':
//       Serial.println("Attempting to send with no connection");
//       comController->request(model);
//       fsController.logDiag(model.toState());
//       break;

//     case 'a':
//       Serial.println("Turning BASE station OFF");
//       // deserializeJson(doc, test);
//       // ccomController->upload(doc);
//       // pmController.disableGNSS();
//       delay(500);
//       break;
//     case 'f':
//       if (!pollFlag) {
//         Serial.println("Polling...");
//         pollFlag = true;
//       } else {
//         Serial.println("Done Polling...");
//         pollFlag = false;
//       }
//       break;
//     case 'g':
//       Serial.println("Setting poll alarm for: ");
//       rtcController.setPollAlarm();
//       break;
//     case 'h':
//       Serial.println("Setting wake alarm for: ");
//       rtcController.setWakeAlarm();
//       break;
//     case 'j':
//       Serial.println("reinit: ");
//       if (!fsController.init())
//         Serial.println("failed");
//       break;
//     }
//   }

//   if (Serial1.available()) {
//     Serial.print(Serial1.read());
//   }
// }

void advancedTest2() {
  char cmd = '\0';
  int count = 0;
  char test[] = "{\"TYPE\":\"ACK\",\"STATE\":[3000,-1,3,4,10,200000]}";

  if (Serial.available())
    cmd = Serial.read();

  switch (cmd) {
  case '1': // PMController
    Serial.println("\n\nTesting PMCOntroller");
    delay(2000);
    Serial.println("Toggling GNSS");
    pmController.enableGNSS();
    delay(2000);
    pmController.disableGNSS();
    Serial.println("Toggling RADIO");
    pmController.enableRadio();
    delay(2000);
    pmController.disableRadio();
    Serial.print("Battery Voltage: ");
    Serial.println(pmController.readBatStr());
    delay(2000);
    Serial.print("Sleeping Tap Device to wake.....:\n");
    pmController.sleep();
    // ...
    delay(2000);
    Serial.println("Awake from sleep!");
    delay(2000);
    Serial.println("Collecting Status");
    pmController.status(model);
    delay(2000);
    model.print();
    model.clear();
    break;

  case '2':
    Serial.println("\n\nTesting IMUCOntroller");
    delay(2000);
    Serial.print("Collecting Status");
    imuController.status(model);
    model.print();
    model.clear();
    break;

  case '3':
    Serial.println("\n\nTesting RTCCOntroller");
    delay(2000);
    Serial.print("Timestamp: ");
    Serial.println(rtcController.getTimestamp());
    Serial.println("Setting 1 min wake alarm");
    // internal testing method
    count = 0;
    rtcController.setWakeAlarm();
    while (1) {
      delay(1000);
      Serial.print(count);
      Serial.println(" seconds");
      count++;
      if (rtcController.alarmDone())
        break;
    }
    Serial.println("Wake alarm triggered!");
    delay(2000);
    Serial.println("Setting 1 min poll alarm");
    count = 0;
    rtcController.setPollAlarm();
    while (1) {
      delay(1000);
      Serial.print(count);
      Serial.println(" seconds");
      count++;
      if (rtcController.alarmDone())
        break;
    }
    Serial.println("Poll alarm triggered!");
    delay(2000);
    Serial.print("Collecting Status");
    rtcController.status(model);
    model.print();
    model.clear();
    break;

    // TODO update routine so we can dynamically change props and test
    // log frequency not effecting logging rate?
    // model not getting updated
  case '4':
    Serial.println("\n\nTesting GNSSController");
    delay(2000);
    Serial.println("Polling for data");
    pmController.enableGNSS();
    rtcController.setPollAlarm();
    while (1) {
      if (gnssController->poll(model))
        model.print();
      if (rtcController.alarmDone())
        break;
    }
    pmController.disableGNSS();
    gnssController->status(model);
    model.print();
    model.clear();
    break;

  case '5':
    Serial.println("\n\nTesting FSController");
    delay(2000);
    Serial.print("Creating new timestampped directory: ");
    delay(2000);
    Serial.println(rtcController.getTimestamp());
    if (fsController.setupWakeCycle(rtcController.getTimestamp(),
                                    gnssController->getFormat()))
      Serial.println("Timestampped directory created!");
    else {
      Serial.println("Throwing write error");
      model.statusERR(WRITE_ERR);
      Serial.println(model.toError());
      fsController.logDiag(model.toError());
    }
    delay(2000);
    Serial.println("Simulated Wake...");
    pmController.enableGNSS();
    rtcController.setPollAlarm();
    while (1) {
      if (gnssController->poll(model))
        fsController.logData(model.toData());
      if (rtcController.alarmDone())
        break;
    }
    pmController.disableGNSS();
    Serial.println("Wake Cycle complete!");
    delay(2000);
    if (fsController.setupWakeCycle(rtcController.getTimestamp(),
                                    gnssController->getFormat()))
      Serial.println("Timestampped directory created!");
    else {
      Serial.println("Throwing write error");
      model.statusERR(WRITE_ERR);
      Serial.println(model.toError());
      fsController.logDiag(model.toError());
    }
    delay(2000);
    Serial.println("Throwing random error");
    delay(2000);
    model.statusERR(ACK_ERR);
    Serial.println(model.toError());
    fsController.logDiag(model.toError());
    Serial.println("Get state of device");
    delay(2000);
    manager.status(model);
    Serial.println("Status of machine: ");
    model.print();
    delay(2000);
    Serial.println("Created diagnostic and state packet:");
    Serial.println(model.toDiag());
    Serial.println(model.toState());
    Serial.println("Writing data to SD...");
    delay(2000);
    fsController.logDiag(model.toDiag());
    fsController.logDiag(model.toState());
    Serial.println("Complete");
    break;

  case '6':
    Serial.println("\n\nTesting COMController");
    delay(2000);
    Serial.println("Creating Request packet");
    manager.status(model);
    Serial.println(model.toReq());
    comController->request(model);
    delay(2000);
    Serial.println("Creating Upload Packet");
    Serial.println(model.toUpl());
    comController->upload(model);
    model.clear();
    Serial.println("Handling response");
    delay(2000);
    model.handleRes(test);
    Serial.println("Collecting status");
    delay(2000);
    comController->status(model);
    model.print();
    model.clear();
    break;
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  // SPI INIT
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV8);

  // Place instatiation here, Serial1 is not in the same compilation unit as
  static COMController _comController(radio, max3243, mux, Serial1, RADIO_BAUD,
                                      CLIENT_ADDR, SERVER_ADDR, INIT_TIMEOUT,
                                      INIT_RETRIES);
  comController = &_comController;
  static GNSSController _gnssController(Serial2, GNSS_BAUD, GNSS_RX, GNSS_TX,
                                        INIT_LOG_FREQ);
  gnssController = &_gnssController;

  manager.add(&_comController);
  manager.add(&_gnssController);
  manager.add(&fsController);
  manager.add(&imuController);
  manager.add(&pmController);
  manager.add(&rtcController);
  manager.init();
}

void loop() {

  while (1) {

    // if (ADVANCED)
    //   advancedTest();

    if (ADVANCED2)
      advancedTest2();

    // if (pollFlag && gnssController->poll(model)) {
    //   // TODO only log if we have valid data, in case data stays in the
    //   buffer
    //   // at sleep?
    //   fsController.logData(model.toData());
    //   model.print();
    // }

    if (imuController.getWakeStatus())
      Serial.println("IMU WAKE");

    // if (rtcController.alarmDone())
    //   Serial.println("RTC ALARM");
  }
}