#include <Arduino.h>
#include <FeatherTrace.h>
#include <Plog.h>

#include "BaseModel.h"
#include "COMController.h"
#include "EventQueue.h"
#include "FSController.h"
#include "PLOGSynchronizer.h"
#include "SatCommController.h"
#include "SatCommDriver.h"

FEATHERTRACE_BIND_ALL()

#define RST 5
#define SPDT_SEL 14// A0
#define RADIO_BAUD 115200
#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 0
#define INIT_TIMEOUT 2000
#define INIT_RETRIES 3
#define IS_Z9C true
#define NUM_ROVERS 1

#define SD_CS 10
#define SD_RST 6
#define ENABLE_SATCOM true

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


using SatCommStateMachine = EventQueue<SatComm::Controller, SatComm::Driver, PLOGSynchronizer>;
using SatCommController = SatComm::Controller<SatCommStateMachine>;


// Model
// initialize each rovers shadow to the default settings
BaseModel model(NUM_ROVERS);

// Global logging initializers
static plog::SerialAppender<plog::TxtFormatter> serialAppender(Serial);
static plog::RollingFileAppender<plog::TxtFormatter> fa("ss_logs.txt");

#define GNSS_ON_PIN A2
#define GNSS_OFF_PIN 12
void useRelay(uint8_t pin) {
  digitalWrite(pin, HIGH);
  delay(4);
  digitalWrite(pin, LOW);
}

void printFault(const FeatherTrace::FaultData &data) {
  // Load the fault data from flash
  // print it the printer
  LOGF << "Fault! Caused: " << FeatherTrace::GetCauseString(data.cause);
  LOGF << "\tFault during recording: " << (data.is_corrupted ? "Yes" : "No");
  if (!data.is_corrupted) {
    LOGF << "\tAt: " << data.file << ':' << data.line;
  }
  LOGF << "\tInterrupt type: " << data.interrupt_type;
  LOGF << "\tStacktrace:";
  for (size_t i = 0;; i++) {
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

  fa.sync();
}

void setup() {
  // LED init
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(115200);


  // SPI INIT
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV8);

  // state machine start
  // this synchronizes PLOGs time, so it must come before PLOG
  if(ENABLE_SATCOM) {
    SatCommStateMachine::reset();
    SatCommStateMachine::start();
  }

  // PLOG init
  plog::init(plog::debug, &serialAppender).addAppender(&fa);
  plog::TimeSync(DateTime(__DATE__, __TIME__), -7);

  // filesystem init
  while (!fsController.init()) {
    // TODO: What happens here? Panic?
    LOGF << "FS Controller failed to initialize!";
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(5000);
  }
  digitalWrite(LED_BUILTIN, LOW);

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

  // SatComm init
  if (ENABLE_SATCOM)
    SatCommStateMachine::dispatch(PowerUp{});

  static COMController _comController(radio, mux, Serial1, RADIO_BAUD,
                                      CLIENT_ADDRESS, SERVER_ADDRESS,
                                      INIT_TIMEOUT, INIT_RETRIES);
  comController = &_comController;
  comController->init();

  fa.sync();
}

void loop() {

  // reinitialize the SD card if needed
  fsController.checkSD();

  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == '1') {
      LOGD << "------- ROVER SHADOW'S --------";
      LOGD << model.toShadow();
    }
    if (cmd == '2') {
      comController->status(model);
      fsController.status(model);
      LOGD << "------- BASE DIAGNOSTICS --------";
      LOGD << model.getBaseDiagnostics();
    }
    if (cmd == '3') {
      LOGD << "------- ROVER STATUS --------";
      model.print();
    }
    if (cmd == '4') {
      LOGD << "UPDATING SHADOW";
      // char buffer[100];
      // StaticJsonDocument<MAX_DATA_LEN> docA;
      // StaticJsonDocument<MAX_DATA_LEN> doc;
      // JsonArray data = doc.createNestedArray(SS_PROP);
      // char buf[100];
      // data.add(3000);
      // data.add(-1);
      // data.add(3);
      // data.add(-1);
      // data.add(0xFF);
      // data.add(-1);
      // data.add(4);
      // serializeJson(doc, buffer);
      // docA["ID"] = 1;
      // docA["CONF"] = buffer;
      // char buf2[1000];
      // serializeJson(docA, buf2);
      // const char* p = docA["CONF"];
      // model.setProps(docA["ID"], (char*)p);
      // EXAMPLE
      // {"ID":1,"CONF":"{\"PROP\":[3000,-1,3,-1,255,-1,4]}"}
    }
  }

  // sync logs
  fa.sync();
  // handle SatComm state
  if (ENABLE_SATCOM) {
    if (!SatCommStateMachine::next()) {
      // TODO: panic behavior?
      LOGF << "SatComm panicked!";
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(5000);
      SatCommStateMachine::reset();
      SatCommStateMachine::start();
      SatCommStateMachine::dispatch(PowerUp{});
    }
  }


  // NOTE all COMController code should be rewritten. This code became a mess due to time
  // constraints
  switch (comController->listen(model)) {
    case -1:
      break;
    case 1:
      if (comController->request(model.getRoverRecent(), model)) {
        fsController.logDiag(model.getRoverRecent(), model.getDiag(model.getRoverRecent()));
        if (model.getRoverIMUFlag(model.getRoverRecent())) {
          LOGD << "Uploading ALERT from rover: " << model.getRoverRecent();
          LOGD << "Packet: " << model.toPacket(model.getRoverRecent(), (_BV(ID_FLAG) | _BV(DIAG_FLAG))) << " Length: "
               << strlen(model.toPacket(model.getRoverRecent(), (_BV(ID_FLAG) | _BV(DIAG_FLAG))));

          /* Commented out because uploading packets during a handshake wastes Rockblock+ credits.
          if (ENABLE_SATCOM) {
            SatCommController::queue(model.toPacket(model.getRoverRecent(), (_BV(ID_FLAG) | _BV(DIAG_FLAG))),
                                    strlen(model.toPacket(model.getRoverRecent(), (_BV(ID_FLAG) | _BV(DIAG_FLAG)))));
            SatCommController::send_now();
          }
          */
        }
      }
      break;
    case 2:
      if (comController->upload(model.getRoverServe(), model)) {
        fsController.logData(model.getRoverServe(), model.getData(model.getRoverServe()));
        LOGD << "Uploading positional data from rover ID: " << model.getRoverServe();
        if (ENABLE_SATCOM){
          SatCommController::queue(model.toPacket(model.getRoverServe(), (_BV(ID_FLAG) | _BV(DIAG_FLAG) | _BV(DATA_FLAG))),
                                  strlen(model.toPacket(model.getRoverServe(), (_BV(ID_FLAG) | _BV(DIAG_FLAG) | _BV(DATA_FLAG)))));
          SatCommController::send_now();
        }
      }
      break;
  }


  // handle incoming data from SatComm
  if (ENABLE_SATCOM) {
  while (SatCommController::available()) {
    // packets are assumed to a JSON encoded array
    // packets may or may not be null terminated.
    SatComm::Packet recv;
    SatCommController::receive(recv);
    // if the packet is zero bytes, continue
    if (recv.length == 0) {
      LOGW << "Received empty packet?";
      continue;
    }
    // if the packet is not null terminated, add a null terminator
    if (recv.bytes[recv.length - 1] != '\0')
      recv.bytes[recv.length++] = '\0';
    LOGI << "Recieved SatComm packet: " << recv.bytes.data();
    
    // use ArduinoJson to deserialize it
    StaticJsonDocument<MAX_DATA_LEN> doc;
    DeserializationError err = deserializeJson(doc, recv.bytes.data());
    if (err != DeserializationError::Ok) {
      LOGE << "Deserialization failed with error: " << err.c_str();
      continue;

    
    }
    SatCommController::queue(model.getBaseDiagnostics(),strlen(model.getBaseDiagnostics()));
    SatCommController::send_now();


    // read in the prop data
    //
    // **** FORMAT SERIAL ****
    // {"ID":2,"CONF":"{\"PROP\":[2000,4,1,1,47,80000,3]}"}
    //
    // **** FORMAT PRETTTY ****
    // {
    //   "ID": 2,
    //   "CONF": "{\"PROP\":[2000,4,1,1,47,80000,3]}"
    // }
    const char *conf = doc["CONF"];
    model.setProps(doc["ID"], (char *) conf);
  }
  }

  fa.sync();

  // TODO: Low battery behavior
}
