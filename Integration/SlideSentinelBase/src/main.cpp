#include "BaseModel.h"
#include "COMController.h"
#include <Arduino.h>

#define RST 5
#define SPDT_SEL 14 // A0
#define RADIO_BAUD 115200
#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 0
#define INIT_TIMEOUT 2000
#define INIT_RETRIES 3
#define IS_Z9C true
#define NUM_ROVERS 2

// TODO you updated the properties class and the SSInterface class, make sure to
// place most recently updated back in the Rover code
// TODO Priority upload on IMU WAKE condition
// TODO log the state of the system when errors are thrown, log rover ID when
// erros are thrown

// COMController
RH_Serial driver(Serial1);
RHReliableDatagram manager(driver, SERVER_ADDRESS);
Freewave radio(RST, -1, IS_Z9C);
SN74LVC2G53 mux(SPDT_SEL, -1);
COMController *comController;

// Model
BaseModel model(NUM_ROVERS);

// needs to be global so as to be referencable
char test_buf[50];

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  static COMController _comController(radio, mux, Serial1, RADIO_BAUD,
                                      CLIENT_ADDRESS, SERVER_ADDRESS,
                                      INIT_TIMEOUT, INIT_RETRIES);
  comController = &_comController;
  comController->init();

  // NOTE
  // Initial state of all rovers on the network
  // props are only updated if the injecting value is not -1.
  // Rover receives base station copy of props every handshake
  // invalid data is handled earlier in the pipeline
  StaticJsonDocument<MAX_DATA_LEN> doc;
  JsonArray data = doc.createNestedArray(SS_PROP);
  data.add(2000);
  data.add(3);
  data.add(2);
  data.add(3);
  data.add(0xff);
  data.add(200000);
  data.add(0);
  serializeJson(doc, test_buf);
  model.setProps(1, test_buf);
  model.print();
}

enum State { IDLE, SERVICE, SATCOM };

void loop() {
  State state = IDLE;

  while (1) {
    switch (state) {
    case IDLE:
      if (comController->listenReq(model)) {
        Serial.println("RECEIVED REQUEST FROM ROVER!");
        if (model.getRoverIMUFlag(model.getRover())) {
          state = SATCOM;
          Serial.println("IMU FLAG FROM ROVER!");
        }
        state = SERVICE;
        break;
      }
      break;
    case SERVICE:
      if (comController->timeout()) {
        Serial.println("Service timeout occured");
        state = IDLE;
        break;
      }
      if (comController->listenUpl(model)) {
        state = SATCOM;
        break;
      }
      break;
    case SATCOM:
      Serial.println("SATCOM");
      state = IDLE;
      break;
    }
  }
}
