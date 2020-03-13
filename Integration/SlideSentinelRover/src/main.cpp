#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
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

// TODO send Tallysman an email about antenna
// TODO libraries from Eagle in the github updated
// TODO use a supported RTC library for DS3231
// TODO clearing the wake flag if it triggers
// TODO implement exponential backoff scheme by checking CD pin on Freewave
// TODO implement global configs, RAIL GNSS RECEIVER AND RADIO ARE ON, THE TYPE
// OF RADIO!

/****** Model ******/
SSModel model;
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

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

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
}

enum State { WAKE, HANDSHAKE, UPDATE, POLL, UPLOAD, SLEEP };

void loop() {
  State state = WAKE;

  while (1) {
    switch (state) {

    case WAKE:
      Serial.println("Checking if channel is busy...");
      // check if radio link is already busy
      if (comController->channelBusy()) {
        state = SLEEP;
        break;
      }

      // create new directory for the wake cycle
      fsController.setupWakeCycle(rtcController.getTimestamp(),
                                  gnssController->getFormat());

      // enable the radio
      pmController.enableRadio();
      Serial.println("Transitioning to HANDSHAKE...");
      state = HANDSHAKE;
      break;

    case HANDSHAKE:
      // collect system status
      manager.status(model);

      // log system status
      fsController.logDiag(model.toDiag());
      fsController.logDiag(model.toProp());

      // make a request
      if (!comController->request(model)) {
        fsController.logDiag(model.toError());
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
        fsController.logData(model.toData(0));

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
      gnssController->flush();

      // disable the radio
      pmController.disableRadio();

      // set the wake alarm
      rtcController.setWakeAlarm();

      // enter low power mode
      pmController.sleep();
      Serial.println("Transitioning to WAKE...");
      state = WAKE;
      break;
    }
  }
}
