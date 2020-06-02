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
#include "FeatherTrace.h"

FEATHERTRACE_BIND_ALL()

// NOTE 3.5mm pins on swift breakout are open if RS232 jumper not applied!
// TODO clearing the wake flag if it triggers
// TODO Rename SSModel to RoverModel, the base station will have BaseModel
// TODO Console class is useless, remove it and write better debugging functionality

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

void save_fault(const FeatherTrace::FaultData& data) {
    // initialize SDCard temporarily
    if (fsController.init()) {
        char fname[32];
        snprintf(fname, sizeof(fname), "/fault_%lu.txt", data.failnum);
        ofstream str(fname, ios::out | ios::trunc);
        if (str.is_open()) {
            str << "Fault! Caused: " << FeatherTrace::GetCauseString(data.cause) << '\n';
            str << "\tFault during recording: " << (data.is_corrupted ? "Yes" : "No") << '\n';
            if (!data.is_corrupted) {
                str << "\tAt: " << data.file << ':' << data.line << '\n';
            }
            str << "\tInterrupt type: " << data.interrupt_type << '\n';
            str << "\tStacktrace:\n";
            for (size_t i = 0; ; i++) {
                char buf[32];
                snprintf(buf, sizeof(buf), "\t\t0x%08lx\n", data.stacktrace[i]);
                str << buf;
                if (i + 1 >= MAX_STRACE || data.stacktrace[i + 1] == 0)
                    break;
            }
            if (data.interrupt_type != 0) {
                str << "\tRegisters:\n";
                char buf[32];
                for (unsigned int i = 0; i < 13; i++) {
                    snprintf(buf, sizeof(buf), "\t\tR%u: 0x%08lx\n", i, data.regs[i]);
                    str << buf;
                }
                snprintf(buf, sizeof(buf), "\t\tSP: 0x%08lx\n", data.regs[13]);
                str << buf;
                snprintf(buf, sizeof(buf), "\t\tLR: 0x%08lx\n", data.regs[14]);
                str << buf;
                snprintf(buf, sizeof(buf), "\t\tPC: 0x%08lx\n", data.regs[15]);
                str << buf;
                snprintf(buf, sizeof(buf), "\t\txPSR: 0x%08lx\n", data.xpsr);
                str << buf;
            }
            str << "\tFailures since upload: " << data.failnum << '\n';
            str.close();
        }
    }
}

void setup() {
  // Hypnos nonsense
  // FIXME: remove
  /*
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW); // Sets pin 5, the pin with the 3.3V rail, to output and enables the rail
  pinMode(6, OUTPUT);
  digitalWrite(6, HIGH); // Sets pin 6, the pin with the 5V rail, to output and enables the rail
  */

  Serial.begin(115200);
  while (!Serial)
    yield();

  // SPI INIT
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV8);

  FeatherTrace::StartWDT(FeatherTrace::WDTTimeout::WDT_8S);

  // FeatherTrace init
  if (FeatherTrace::DidFault()) {
      FeatherTrace::PrintFault(Serial); MARK;
      save_fault(FeatherTrace::GetFault()); MARK;
  }

  // Place instatiation here, Serial1 is not in the same compilation unit
  // FIXME: Should not use a pointer to a static variable in a function
  static COMController _comController(radio, max3243, mux, Serial1, RADIO_BAUD,
                                      CLIENT_ADDR, SERVER_ADDR, INIT_TIMEOUT,
                                      INIT_RETRIES); MARK;
  comController = &_comController; MARK;
  // FIXME: Should not use a pointer to a static variable in a function
  static GNSSController _gnssController(Serial2, GNSS_BAUD, GNSS_RX, GNSS_TX,
                                        INIT_LOG_FREQ); MARK;
  gnssController = &_gnssController; MARK;

  manager.add(&_comController); MARK;
  manager.add(&_gnssController); MARK;
  manager.add(&fsController); MARK;
  manager.add(&rtcController); MARK;
  manager.add(&imuController);
  manager.add(&pmController);
  if (!manager.init()) { MARK;
    Serial.println("Fatal: initialization failed!");
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    MARK;
    delay(5000);
    FeatherTrace::Fault(FeatherTrace::FaultCause::FAULT_USER);
  }

}

enum State { WAKE, HANDSHAKE, UPDATE, POLL, UPLOAD, SLEEP };

void execute();

void loop() { MARK; execute(); }

static State state = WAKE;

void execute() { MARK;
    switch (state) {
    case WAKE: MARK;
      // create new directory for the wake cycle
      fsController.setupWakeCycle(rtcController.getTimestamp(),
                                  gnssController->getFormat());

      // enable the radio
      pmController.enableRadio();

      Serial.println("Transitioning to HANDSHAKE...");
      state = HANDSHAKE; // EDIT!
      break;

    case HANDSHAKE: MARK;
      // collect system status
      manager.status(model);

      model.print();

      // log system status
      fsController.logDiag(model.toDiag());
      fsController.logDiag(model.toProp());

      // make a request, send diagnostics/receive props
      if (!comController->request(model)) {
        fsController.logDiag(model.toError());
        rtcController.incrementBackoff();
        Serial.println("Transitioning to SLEEP...");
        state = SLEEP;
        break;
      }

      Serial.println("Transitioning to UPDATE...");
      state = UPDATE;
      break;

    case UPDATE: MARK;
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

    case POLL: MARK;
      // check for data from the GNSS receiver
      if (gnssController->poll(model))
        fsController.logData(model.toData(model.getProp(THRESHOLD)));

      // check ifs the alarm is triggered
      if (rtcController.alarmDone()) {
        Serial.println("Transitioning to UPLOAD...");
        state = UPLOAD;
      }
      break;

    case UPLOAD: MARK;
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

    case SLEEP: MARK;
      // flush pending GNSS data
      gnssController->reset();

      // disable the radio
      pmController.disableRadio();

      // set the wake alarm
      rtcController.setWakeAlarm();

      // disable WDT
      FeatherTrace::StopWDT();

      // enter low power mode
      pmController.sleep();

      // re-enable WDT
      FeatherTrace::StartWDT(FeatherTrace::WDTTimeout::WDT_8S);
      // enable GNSS
      pmController.enableGNSS(); MARK;
      delay(200); MARK;
      pmController.disableGNSS();
      Serial.println("Transitioning to WAKE...");
      state = WAKE;
      break;
    }
}