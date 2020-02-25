#include "Battery.h"
#include "COMController.h"
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
#include "State.h"
#include "VoltageReg.h"
#include "config_2.0.0.h"
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

// TODO centrailze "MSG" headers and reference this in all all files for ensured
// consistency
/****** Test Routine ******/
#define ADVANCED true

State state(INIT_TIMEOUT, INIT_RETRIES, INIT_WAKETIME,
            INIT_SLEEPTIME, INIT_SENSITIVITY, INIT_LOG_FREQ);

/****** Mail ******/
StaticJsonDocument<1000> doc;

/****** FSController Init ******/
FSController fsController(&state, SD_CS, SD_RST);

/****** ComController Init ******/
Freewave radio(RST, CD, IS_Z9C);
SN74LVC2G53 mux(SPDT_SEL, -1);
MAX3243 max3243(FORCEOFF_N);
COMController *comController;

/****** PMController Init ******/
PoluluVoltageReg vcc2(VCC2_EN); // TODO rename this class to the id of the
                                // voltage regulator from the manufacturer
MAX4280 max4280(MAX_CS, &SPI);
Battery batReader(BAT);
PMController pmController(&state, &max4280, &vcc2, &batReader, false, true);

/****** IMUController Init ******/
IMUController imuController(&state, ACCEL_INT);

/****** GNSSController Init ******/
Uart Serial2(&sercom1, GNSS_RX, GNSS_TX, SERCOM_RX_PAD_3, UART_TX_PAD_0);
void SERCOM1_Handler() { Serial2.IrqHandler(); }
GNSSController *gnssController;

/****** RTCController Init ******/
RTC_DS3231 RTC_DS;
RTCController rtcController(&state, &RTC_DS, RTC_INT);

bool pollFlag = false;
void advancedTest() {
  char cmd;
  char test[] = "{\"sensor\":\"gps\",\"time\":1351824120}";

  char fileName[30];
  memset(fileName, '\0', sizeof(char) * 30);

  if (Serial.available()) {
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
    case 'q':
      Serial.println("Resetting the radio, DRIVING RST LOW");
      comController->resetRadio();
      break;
    case 'w':
      Serial.print("STATE of CD pin: ");
      if (comController->channelBusy())
        Serial.println("BUSY");
      else
        Serial.println("NOT BUSY");
      break;
    case 'e':
      Serial.print("Printing timestamp: ");
      Serial.println(rtcController.getTimestamp());
      break;
    case 't':
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
    case 'y':
      Serial.print("reading battery voltage: ");
      Serial.println(pmController.readBat());
      break;
    case 'u':
      Serial.print("reading battery voltage string: ");
      Serial.println(pmController.readBatStr());
      break;
    case 'i':
      Serial.println("Turning off VCC2");
      vcc2.disable();
      break;
    case 'o':
      Serial.println("Turning on VCC2");
      vcc2.enable();
      break;
    case 'p':
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

    case 's':
      Serial.println("Creating new directory");
      Serial.println(rtcController.getTimestamp());
      fsController.setupWakeCycle(rtcController.getTimestamp(),
                                  gnssController->getFormat());
      break;

    case 'd':
      Serial.println("Attempting to send with no connection");
      comController->request(doc);
      serializeJsonPretty(doc, Serial);
      fsController.log(doc);
      break;

    case 'a':
      Serial.println("Turning BASE station OFF");
      deserializeJson(doc, test);
      comController->upload(doc);
      pmController.disableGNSS();
      delay(500);
      break;
    case 'f':
      if (!pollFlag) {
        Serial.println("Polling...");
        pollFlag = true;
      } else {
        Serial.println("Done Polling...");
        pollFlag = false;
      }
      break;
    case 'g':
      Serial.println("Setting poll alarm for: ");
      rtcController.setPollAlarm();
      break;
    case 'h':
      Serial.println("Setting wake alarm for: ");
      rtcController.setWakeAlarm();
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

  // SPI INIT
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV8);

  // Place instatiation here, Serial1 is not in the same compilation unit as
  static COMController _comController(&state, &radio, &max3243, &mux, &Serial1,
                                      RADIO_BAUD, CLIENT_ADDR, SERVER_ADDR);
  comController = &_comController;
  static GNSSController _gnssController(&state, &Serial2, GNSS_BAUD, GNSS_RX,
                                        GNSS_TX);
  gnssController = &_gnssController;

  // TODO make a vector CONManager
  if (comController->init())
    Serial.println("initialized COMCONTROLLER");

  if (gnssController->init())
    Serial.println("initialized GNSSCONTROLLER");

  if (imuController.init())
    Serial.println("initialized IMUCONTROLLER");

  if (rtcController.init())d
    Serial.println("initialized RTCCONTROLLER");

  if (fsController.init())
    Serial.println("initialized FSCONTROLLER");

  if (pmController.init())
    Serial.println("initialized PMCONTROLLER");
}

void loop() {

  while (1) {

    if (ADVANCED)
      advancedTest();

    if (pollFlag && gnssController->poll(doc)) {
      serializeJsonPretty(doc, Serial);
      // TODO only log if we have valid data, in case data stays in the buffer
      // at sleep?
      fsController.log(doc);
    }

    if (imuController.getWakeStatus(doc)) {
      serializeJsonPretty(doc, Serial);
      Serial.println("IMU ALARM");
      doc.clear();
    }

    if (rtcController.alarmDone())
      Serial.println("RTC ALARM");
  }
}
