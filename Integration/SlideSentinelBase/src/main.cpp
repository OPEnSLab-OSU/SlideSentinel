#include "BaseModel.h"
#include "COMController.h"
#include "FSController.h"
#include <Plog.h>
#include <Appenders/RollingFileAppender.h>
#include <Appenders/SerialAppender.h>
#include <Arduino.h>
#include <FeatherTrace.h>

FEATHERTRACE_BIND_ALL()

#define RST 5
#define SPDT_SEL 14 // A0
#define RADIO_BAUD 115200
#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 0
#define INIT_TIMEOUT 2000
#define INIT_RETRIES 3
#define IS_Z9C true
#define NUM_ROVERS 2

#define SD_CS 10 // TODO: Change back to 10
#define SD_RST 6

// TODO you updated the properties class and the SSInterface class, make sure to
// place most recently updated back in the Rover code
// TODO Priority upload on IMU WAKE condition
// TODO log the state of the system when errors are thrown, log rover ID when
// erros are thrown
// TODO: Test if the base can recover from removing the SDcard (does FSController need to be re-inited?)

// COMController
RH_Serial driver(Serial1);
RHReliableDatagram manager(driver, SERVER_ADDRESS);
Freewave radio(RST, IS_Z9C);
SN74LVC2G53 mux(SPDT_SEL, -1);
COMController *comController;
FSController fsController(SD_CS, SD_RST, NUM_ROVERS);

// Model
BaseModel model(NUM_ROVERS);

// Global logging initializers
static plog::SerialAppender<plog::TxtFormatter> serialAppender(Serial);
static plog::RollingFileAppender<plog::TxtFormatter> fa("ss_logs.txt");

// needs to be global so as to be referencable
char test_buf[50];

#define GNSS_ON_PIN A2
#define GNSS_OFF_PIN 12
void useRelay(uint8_t pin) {
  digitalWrite(pin, HIGH);
  delay(4);
  digitalWrite(pin, LOW);
}



void printFault(const FeatherTrace::FaultData& data) {
  // Load the fault data from flash
  // print it the printer
  LOGF << "Fault! Caused: " << FeatherTrace::GetCauseString(data.cause);
  LOGF << "\tFault during recording: " << (data.is_corrupted ? "Yes" : "No");
  if (!data.is_corrupted) {
    LOGF << "\tAt: " << data.file << ':' << data.line;
  }
  LOGF << "\tInterrupt type: " << data.interrupt_type;
  LOGF << "\tStacktrace:";
  for (size_t i = 0; ; i++) {
    LOGF.printf("\t\t0x%08lx", data.stacktrace[i]);
    if (i + 1 >= MAX_STRACE || data.stacktrace[i + 1] == 0)
      break;
  }
  if (data.interrupt_type != 0) {
    LOGF << "\tRegisters:";
    for (unsigned int i = 0; i < 13; i++)
      LOGF.printf("\t\tR%u: 0x%08lx", i, data.regs[i]);
    LOGF.printf("\t\tSP: 0x%08lx", data.regs[13]);
    LOGF.printf("\t\tLR: 0x%08lx", data.regs[14]);
    LOGF.printf("\t\tPC: 0x%08lx", data.regs[15]);
    LOGF.printf("\t\txPSR: 0x%08lx", data.xpsr);
  }
  LOGF << "\tFailures since upload: " << data.failnum;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    yield();

  // SPI INIT
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV8);

  // PLOG init
  // TODO: get time from GNSS to sync PLOG
  plog::TimeSync(DateTime(__DATE__, __TIME__), -7);
  plog::init(plog::debug, &serialAppender).addAppender(&fa);

  // filesystem init
  if (!fsController.init()) {
    // TODO: What happens here? Panic?
    LOGF << "FS Controller failed to initialize!";
  }

  // TODO: send fault data over GNSS
  if (FeatherTrace::DidFault())
    printFault(FeatherTrace::GetFault());

  pinMode(GNSS_ON_PIN, OUTPUT);
  pinMode(GNSS_OFF_PIN, OUTPUT);
  LOGI << "gnss off";
  useRelay(GNSS_OFF_PIN);
  delay(1000);
  LOGI << "gnss on";
  useRelay(GNSS_ON_PIN);

  static COMController _comController(radio, mux, Serial1, RADIO_BAUD,
                                      CLIENT_ADDRESS, SERVER_ADDRESS,
                                      INIT_TIMEOUT, INIT_RETRIES);
  comController = &_comController;
  comController->init();

  StaticJsonDocument<MAX_DATA_LEN> doc;
  JsonArray data = doc.createNestedArray(SS_PROP);
  data.add(2000);
  data.add(3);
  data.add(2);
  data.add(3);
  data.add(0x0f);
  data.add(200000);
  data.add(0);
  serializeJson(doc, test_buf);
  model.setProps(1, test_buf);

  StaticJsonDocument<MAX_DATA_LEN> doc2;
  JsonArray data2 = doc2.createNestedArray(SS_PROP);
  data2.add(2000);
  data2.add(4);
  data2.add(3);
  data2.add(3);
  data2.add(0x0f);
  data2.add(80000);
  data2.add(3);
  serializeJson(doc2, test_buf);
  model.setProps(2, test_buf);
  model.print();
}

enum State { LISTEN, SATCOM };

// collect string of all diagnostics and props for each rover
// collect string of Base station diagnostics: stopwatch, num_uploads, num_requests, SD card memory

static State state = LISTEN;

void loop() {
  fsController.checkSD();
  // parse serial commands
  if(Serial.available()){
    char cmd = Serial.read();
    if(cmd == '1'){
      LOGD << "------- BASE STATUS --------";
      LOGD << model.getRoverShadow();
    }
    if(cmd == '2'){
      comController->status(model);
      fsController.status(model);
      LOGD << "------- BASE DIAGNOSTICS --------";
      LOGD << model.getBaseDiagnostics();
    }
    if(cmd == '3'){
      LOGD << "------- ROVER STATUS --------";
      model.print();
    }
  }

  // handle state transitions
  switch (state) {
  case LISTEN:
    if (comController->listen(model)) {
      if (model.getRoverIMUFlag(model.getRoverAlert())) {
        LOGD << "LISTEN -> SATCOM";
        state = SATCOM;
      }
      fsController.logDiag(model.getRoverAlert(),
                           model.getDiag(model.getRoverAlert()));
      fsController.logProps(model.getRoverAlert(),
                            model.getProps(model.getRoverAlert()));
      break;
    }
    break;
  case SATCOM:
    fsController.logData(model.getRoverServe(),
                         model.getData(model.getRoverServe()));
    LOGD << "SATCOM";
    LOGD << "Uploading Alert from rover: " << model.getRoverAlert();
    LOGD << "SATCOM -> LISTEN";
    state = LISTEN;
    break;
  }
}
