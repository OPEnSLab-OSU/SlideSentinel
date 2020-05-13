#include "BaseModel.h"
#include "COMController.h"
#include "FSController.h"
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

#define SD_CS 10
#define SD_RST 6

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
FSController fsController(SD_CS, SD_RST, NUM_ROVERS);

// Model
BaseModel model(NUM_ROVERS);

// needs to be global so as to be referencable
char test_buf[50];

#define GNSS_ON_PIN A2
#define GNSS_OFF_PIN 12
void useRelay(uint8_t pin) {
  digitalWrite(pin, HIGH);
  delay(4);
  digitalWrite(pin, LOW);
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  // SPI INIT
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV8);

  pinMode(GNSS_ON_PIN, OUTPUT);
  pinMode(GNSS_OFF_PIN, OUTPUT);
  Serial.println("gnss off");
  useRelay(GNSS_OFF_PIN);
  delay(1000);
  Serial.println("gnss on");
  useRelay(GNSS_ON_PIN);

  static COMController _comController(radio, mux, Serial1, RADIO_BAUD,
                                      CLIENT_ADDRESS, SERVER_ADDRESS,
                                      INIT_TIMEOUT, INIT_RETRIES);
  comController = &_comController;
  comController->init();
  fsController.init();

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

void loop() {
  State state = LISTEN;

  while (1) {


    if(Serial.available()){
      char cmd = Serial.read();
      if(cmd == '1'){
        Serial.println("\n------- BASE STATUS --------");
        Serial.println(model.getRoverShadow());
      }
      if(cmd == '2'){
        Serial.println("\n------- BASE DIAGNOSTICS --------");
        comController->status(model);
        fsController.status(model);
        Serial.println(model.getBaseDiagnostics());
      }
      if(cmd == '3'){
        Serial.println("\n------- ROVER STATUS --------");
        model.print();
      }
    }


    switch (state) {
    case LISTEN:
      if (comController->listen(model)) {
        if (model.getRoverIMUFlag(model.getRoverAlert())) {
          state = SATCOM;
          Serial.println("IMU FLAG FROM ROVER!");
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
      Serial.println("SATCOM");
      Serial.print("Uploading Alert from rover: ");
      Serial.println(model.getRoverAlert());
      state = LISTEN;
      break;
    }
  }
}
