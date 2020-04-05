#include "BaseModel.h"
#include "COMController.h"
#include <Arduino.h>

#define RST 5
#define SPDT_SEL 14 // A0
#define RADIO_BAUD 115200
#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2
#define INIT_TIMEOUT 2000
#define INIT_RETRIES 3
#define IS_Z9C true
#define NUM_ROVERS 2

// TODO ensure that satcom updating the shadow with queued prop data is -1 where
// invalid! AT init every prop is -1
// TODO you updated the properties class and the SSInterface class, make sure to
// place most recently updated back in the Rover code
// TODO Priority upload on IMU WAKE condition
// TODO log the state of the system when errors are thrown, log rover ID when erros are thrown
// TODO implement exp backoff


// COMController
RH_Serial driver(Serial1);
RHReliableDatagram manager(driver, SERVER_ADDRESS);
Freewave radio(RST, -1, IS_Z9C);
SN74LVC2G53 mux(SPDT_SEL, -1);
COMController *comController;

// Model
BaseModel model(NUM_ROVERS);

void setup() {
  static COMController _comController(radio, mux, Serial1, RADIO_BAUD,
                                      CLIENT_ADDRESS, SERVER_ADDRESS,
                                      INIT_TIMEOUT, INIT_RETRIES);
  comController = &_comController;
  comController->init();
}

enum State { IDLE, SERVICE, SATCOM };

void loop() {
  State state = IDLE;

  while (1) {
    switch (state) {
    case IDLE:
      if (comController->listenUpl(model)) {
        state = SERVICE;
        break;
      }
      break;
    case SERVICE:
      if (comController->timeout()) {
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
