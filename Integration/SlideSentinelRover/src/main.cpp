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
#include "network_config_2.0.0.h"
#include "pcb_2.0.0.h"
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

// NOTE 3.5mm pins on swift breakout are open if RS232 jumper not applied!
// TODO libraries from Eagle in the github updated
// TODO clearing the wake flag if it triggers
// TODO rename the ComController folder in remote repo to COMController
// TODO Rename SSModel to RoverModel, the base station will have BaseModel

/****** Model ******/
SSModel model;
ConManager manager;

/****** FSController Init ******/
FSController fsController(SD_CS, SD_RST);

/****** COMController Init ******/
Freewave radio(RST, CD, IS_Z9C);
SN74LVC2G53 mux(SPDT_SEL, -1);
MAX3243 max3243(FORCEOFF_N);
COMController *comController;

/****** PMController Init ******/
PoluluVoltageReg vcc2(VCC2_EN);
MAX4280 max4280(MAX_CS, &SPI);
Battery batReader(BAT_PIN);
PMController pmController(max4280, vcc2, batReader, GNSS_RAIL2, RADIO_RAIL2);

/****** IMUController Init ******/
IMUController imuController(ACCEL_INT, INIT_SENSITIVITY);

/****** GNSSController Init ******/
Uart Serial2(&sercom1, GNSS_RX, GNSS_TX, SERCOM_RX_PAD_3, UART_TX_PAD_0);
void SERCOM1_Handler() { Serial2.IrqHandler(); }
GNSSController *gnssController;

/****** RTCController Init ******/
RTC_DS3231 RTC_DS;
RTCController rtcController(RTC_DS, RTC_INT, INIT_WAKETIME, INIT_SLEEPTIME);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    yield();

  // SPI INIT
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV8);

  // Place instatiation here, Serial1 is not in the same compilation unit
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

  // // NOTE Serial passthrough
  // mux.comY1();
  // max3243.disable();
  // Serial1.begin(115200);
  // pmController.enableGNSS();
}

enum State { WAKE, HANDSHAKE, UPDATE, POLL, UPLOAD, SLEEP };

// NOTE serial passthrough
// void loop() {
//   if (Serial.available()) { // If anything comes in Serial (USB),
//     Serial1.write(Serial.read()); // read it and send it out Serial1 (pins 0
//     & 1)
//   }

//   if (Serial1.available()) {      // If anything comes in Serial1 (pins 0 &
//   1)
//     Serial.write(Serial1.read()); // read it and send it out Serial (USB)
//   }
// }

void execute();
void test();
void wait();

void loop() { execute(); }

void execute() {
  State state = WAKE;
  while (1) {
    switch (state) {

    case WAKE:
      // create new directory for the wake cycle
      fsController.setupWakeCycle(rtcController.getTimestamp(),
                                  gnssController->getFormat());

      // enable the radio
      pmController.enableRadio();

      Serial.println("Transitioning to HANDSHAKE...");
      state = HANDSHAKE; // EDIT!
      break;

    case HANDSHAKE:
      // collect system status
      manager.status(model);

      model.print();

      // log system status
      fsController.logDiag(model.toDiag());
      fsController.logDiag(model.toProp());

      // make a request, send diagnostics/receive props
      if (!comController->request(model)) {
        fsController.logDiag(model.toError());
        Serial.println("Transitioning to SLEEP...");
        state = SLEEP;
        break;
      }

      Serial.println("Transitioning to UPDATE...");
      state = UPDATE;
      break;

    case UPDATE:
      // update the systems properties
      manager.update(model);

      // enable the GNSS receiver
      pmController.enableGNSS();

      // collect system status
      manager.status(model);
      Serial.println("System status post update..");
      model.print();

      // log system status
      fsController.logDiag(model.toDiag());
      fsController.logDiag(model.toProp());

      // set the poll alarm
      rtcController.setPollAlarm();

      Serial.println("Transitioning to POLL...");
      state = POLL;
      break;

    case POLL:
      // check for data from the GNSS receiver
      if (gnssController->poll(model))
        fsController.logData(model.toData(model.getProp(THRESHOLD)));

      // check ifs the alarm is triggered
      if (rtcController.alarmDone()) {
        Serial.println("Transitioning to UPLOAD...");
        state = UPLOAD;
      }
      break;

    case UPLOAD:
      // disable the GNSS receiver
      pmController.disableGNSS();

      // collect the systems status
      manager.status(model);
      model.print();

      // log system status
      fsController.logDiag(model.toDiag());
      fsController.logDiag(model.toProp());
      model.print();

      // make an upload
      if (!comController->upload(model))
        fsController.logDiag(model.toError());
      Serial.println("Transitioning to SLEEP...");
      state = SLEEP;
      break;

    case SLEEP:
      // flush pending GNSS data
      gnssController->reset();

      // disable the radio
      pmController.disableRadio();

      // set the wake alarm
      rtcController.setWakeAlarm();

      // enter low power mode
      pmController.sleep();
      pmController.enableGNSS();
      delay(200);
      pmController.disableGNSS();
      Serial.println("Transitioning to WAKE...");
      state = WAKE;
      break;
    }
  }
}

void wait() {
  Serial.println("waiting for input");
  while (!Serial.available())
    ;
  while (Serial.available()) {
    Serial.read();
  }
}

void test() {
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
      model.setError(WRITE_ERR);
      Serial.println(model.toError());
      fsController.logDiag(model.toError());
    }
    delay(2000);
    Serial.println("Simulated Wake...");
    pmController.enableGNSS();
    rtcController.setPollAlarm();
    while (1) {
      if (gnssController->poll(model))
        fsController.logData(model.toData(0));
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
      model.setError(WRITE_ERR);
      Serial.println(model.toError());
      fsController.logDiag(model.toError());
    }
    delay(2000);
    Serial.println("Throwing random error");
    delay(2000);
    model.setError(ACK_ERR);
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
    Serial.println(model.toProp());
    Serial.println("Writing data to SD...");
    delay(2000);
    fsController.logDiag(model.toDiag());
    fsController.logDiag(model.toProp());
    Serial.println("Complete");
    break;

  case '6':
    Serial.println("\n\nTesting COMController");
    delay(2000);
    Serial.println("Creating Request packet");
    manager.status(model);
    Serial.println(model.toDiag());
    comController->request(model);
    delay(2000);
    Serial.println("Creating Upload Packet");
    Serial.println(model.toData(3));
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

  case '7':
    Serial.println("\n\nTesting COMController");
    Serial.println("Creating Request packet");
    manager.status(model);
    Serial.println(model.toDiag());
    comController->request(model);
    break;
  case '8':
    Serial.print("CD pin: ");
    if (comController->channelBusy(model))
      Serial.println("BUSY");
    else
      Serial.println("NOT BUSY");
    break;
  case 'a':
    max3243.enable();
    pmController.enableRadio();
    break;
  case 'c':
    pmController.disableRadio();
    max3243.disable();
    break;
  case 'd':
    Serial1.println(" MOOOSE MOOOSE MOOOSE MOOOSE  MOOOSE MOOOSE  MOOOSE "
                    "MOOOSE  MOOOSE MOOOSE  MOOOSE MOOOSE  MOOOSE MOOOSE ");
    break;
  case 'b':
    for (int i = 0; i < 2000; i++) {
      if (Serial1.available()) {
        Serial.print((char)Serial1.read());
      }
    }
    break;
  }
}