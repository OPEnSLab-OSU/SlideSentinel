#include "ArduinoJson.h"
#include "SSInterface.h"
#include <Arduino.h>
#include <RHReliableDatagram.h>
#include <RH_Serial.h>
#include <SD.h>
#include <SPI.h>
#include <wiring_private.h>

#define REQ 1
#define UPL 2
#define RES 3

// RADIO INTERFACE
#define RST 5

// Switch Pin Def
#define SPDT_SEL A0

// Relay constants
#define GNSS_ON_PIN A2
#define GNSS_OFF_PIN 12

// SD CARD CONSTANTS
#define SD_CS 10

// PMC REGULATOR
#define VCC2_EN 21 // SCL

// Switch Pin Def
#define SPDT_SEL A0

// Radio Serial
#define ROCK_TX 11
#define ROCK_RX 12
#define ROCK_ONOFF 9
#define ROCK_NETAV A5
#define ROCK_RINGAL A3

// Reliable Datagram usage
#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2
RH_Serial driver(Serial1);
RHReliableDatagram manager(driver, SERVER_ADDRESS);
uint8_t buf[RH_SERIAL_MAX_MESSAGE_LEN];
char packet[1000];
uint8_t data[] = "{\"type\":\"ACK\"}";
StaticJsonDocument<RH_SERIAL_MAX_MESSAGE_LEN> doc;
const char *type;

SSInterface interface(Serial1, 115200, CLIENT_ADDRESS, SERVER_ADDRESS, 2000, 3,
                      true);

#define IRIDIUM_BAUD 115200
Uart Serial2(&sercom1, ROCK_RX, ROCK_TX, SERCOM_RX_PAD_3, UART_TX_PAD_0);
void SERCOM1_Handler() { Serial2.IrqHandler(); }

void Serial2Setup(uint16_t baudrate) {
  Serial2.begin(baudrate);
  pinPeripheral(ROCK_TX,
                PIO_SERCOM); // Private functions for serial communication
  pinPeripheral(ROCK_RX, PIO_SERCOM);
}

void useRelay(uint8_t pin) {
  digitalWrite(pin, HIGH);
  delay(4);
  digitalWrite(pin, LOW);
}

void setup() {
  Serial.begin(115200);

  // Radio Serial
  Serial1.begin(115200);

  // Set up Reliable datagram socket
  if (!manager.init())
    Serial.println("init failed");
  interface.init();

  // Iridium Serial
  Serial2Setup(IRIDIUM_BAUD);
  pinMode(ROCK_ONOFF, OUTPUT);
  pinMode(ROCK_NETAV, INPUT);
  pinMode(ROCK_RINGAL, INPUT);

  // setup SD
  pinMode(SD_CS, OUTPUT);
  setup_sd();

  // setup Relay
  pinMode(GNSS_ON_PIN, OUTPUT);
  pinMode(GNSS_OFF_PIN, OUTPUT);
  useRelay(GNSS_OFF_PIN);

  digitalWrite(VCC2_EN, HIGH);

  // SPDT INIT, Feather receives data
  pinMode(SPDT_SEL, OUTPUT);
  digitalWrite(SPDT_SEL, HIGH);
}

void loop() {
  // //test();
  // memset(buf, '\0', sizeof(buf));
  // if (manager.available())
  // {
  //     Serial.println("here");
  //     // Turn this high so the base can send an ACK!
  //     digitalWrite(SPDT_SEL, HIGH);
  //     // Wait for a message addressed to us from the client
  //     uint8_t len = sizeof(buf);
  //     uint8_t from;
  //     if (manager.recvfromAck(buf, &len, &from))
  //     {
  //         Serial.print("got request from : 0x");
  //         Serial.print(from, HEX);
  //         Serial.print(": ");
  //         Serial.println((char *)buf);
  //         deserializeJson(doc, buf);
  //         type = doc["type"];

  //         // Send a reply back to the originator client
  //         if (!manager.sendtoWait(data, sizeof(data), from)) //SEND CONFIG
  //         DATA TIED TO THIS
  //             Serial.println("sendtoWait failed");
  //     }
  //     // Start RTK
  //     if (strcmp(type, "RTS") == 0)
  //     {
  //         Serial.println("RTS received");
  //         Serial.println("(A0 HIGH) GNSS -----> RADIO");
  //         digitalWrite(SPDT_SEL, LOW);
  //         useRelay(GNSS_ON_PIN);
  //     }
  //     // End RTK
  //     else
  //     {
  //         Serial.println("(A0 HIGH) FEATHER_M0 -----> RADIO");
  //         digitalWrite(SPDT_SEL, HIGH);
  //         useRelay(GNSS_OFF_PIN);
  //         Serial.println("not RTS");
  //     }
  // }

  // test();

  test2();
  // memset(packet, '\0', 1000);
  // if (interface.available()) {
  //   Serial.println("here");
  //   // Turn this high so the base can send an ACK!
  //   digitalWrite(SPDT_SEL, HIGH);
  //   // Wait for a message addressed to us from the client
  //   if (interface.receivePacket(packet)) { // SEND CONFIG BACK, FLUSH
  //   BUFFERS!!
  //                                          // THINK ABOUT THIS STUFF
  //     Serial.print("data received");
  //     Serial.println(packet);
  //     char test[] = "{\"TYPE\":\"ACK\",\"PROP\":[3000,-1,2,2,10,100000,3]}";
  //     if (!interface.sendPacket(RES, test))
  //       Serial.println("FAILED to respond");
  //   }
  // }
}

// #define TIMEOUT 0
// #define RETRIES 1
// #define WAKE_TIME 2
// #define SLEEP_TIME 3
// #define SENSITIVITY 4
// #define LOG_FREQ 5
// #define THRESHOLD 6

void test2() {
  int state = 0;
  while (1) {
    char cmd;
    if (Serial.available()) {
      cmd = Serial.read();
      switch (cmd) {
      case '0':
        state = 0;
        break;
      case '1':
        state = 1;
        break;
      }
    }
    if (state == 1) {
      Serial.println("sending");
      Serial1.println(
          "TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST "
          "TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST "
          "TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST "
          "TEST TEST TEST TEST TEST TEST TEST TEST TEST");
    }
    if (Serial1.available()) {
      Serial.println("read");
      Serial.println(Serial1.read());
    }
  }
}

void test() {
  char cmd;
  if (Serial.available()) {
    cmd = Serial.read();
    switch (cmd) {
    case '1':
      Serial.println("gnss on");
      useRelay(GNSS_ON_PIN);
      break;
    case '2':
      Serial.println("gnss off");
      useRelay(GNSS_OFF_PIN);
      break;
    case '3':
      Serial1.println(
          "TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST "
          "TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST "
          "TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST "
          "TEST TEST TEST TEST TEST TEST TEST TEST TEST");
      Serial.println("Writing data out...");
      break;
    case '4':
      Serial.println("(A0 LOW) GNSS -----> RADIO");
      digitalWrite(SPDT_SEL, LOW);
      break;
    case '5':
      Serial.println("(A0 HIGH) FEATHER_M0 -----> RADIO");
      digitalWrite(SPDT_SEL, HIGH);
      break;
    case '6':
      Serial2.println(
          "TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST "
          "TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST "
          "TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST "
          "TEST TEST TEST TEST TEST TEST TEST TEST TEST");
      Serial.println("Writing data out...");
      break;
    case '7':
      digitalWrite(ROCK_ONOFF, HIGH);
      Serial.println("Turning SATCOM on");
      break;
    case '8':
      digitalWrite(ROCK_ONOFF, LOW);
      Serial.println("Turning SATCOM off");
      break;
    case '9':
      digitalWrite(VCC2_EN, HIGH);
      Serial.println("Turning on second voltage rail");
      break;
    case '0':
      digitalWrite(VCC2_EN, LOW);
      Serial.println("Turning off second voltage rail");
      break;
    }
  }
  if (Serial1.available()) {
    char c = Serial1.read();
    Serial.print(c);
  }
}

void setup_sd() {
  Serial.println("Initializing SD card...");

  if (!SD.begin(SD_CS)) {
    Serial.println("SD Initialization failed!");
    Serial.println("Will continue anyway, but SD functions will be skipped");
  } else {
    Serial.println("initialization complete");
  }
}

// void loop() { advancedTest2(); }

// void advancedTest2() {
//   char cmd = '\0';
//   int count = 0;
//   char test[] = "{\"TYPE\":\"ACK\",\"STATE\":[3000,-1,3,4,10,200000]}";

//   if (Serial.available())
//     cmd = Serial.read();

//   switch (cmd) {
//   case '1': // PMController
//     Serial.println("\n\nTesting PMCOntroller");
//     delay(2000);
//     Serial.println("Toggling GNSS");
//     pmController.enableGNSS();
//     delay(2000);
//     pmController.disableGNSS();
//     Serial.println("Toggling RADIO");
//     pmController.enableRadio();
//     delay(2000);
//     pmController.disableRadio();
//     Serial.print("Battery Voltage: ");
//     Serial.println(pmController.readBatStr());
//     delay(2000);
//     Serial.print("Sleeping Tap Device to wake.....:\n");
//     pmController.sleep();
//     // ...
//     delay(2000);
//     Serial.println("Awake from sleep!");
//     delay(2000);
//     Serial.println("Collecting Status");
//     pmController.status(model);
//     delay(2000);
//     model.print();
//     model.clear();
//     break;

//   case '2':
//     Serial.println("\n\nTesting IMUCOntroller");
//     delay(2000);
//     Serial.print("Collecting Status");
//     imuController.status(model);
//     model.print();
//     model.clear();
//     break;

//   case '3':
//     Serial.println("\n\nTesting RTCCOntroller");
//     delay(2000);
//     Serial.print("Timestamp: ");
//     Serial.println(rtcController.getTimestamp());
//     Serial.println("Setting 1 min wake alarm");
//     // internal testing method
//     count = 0;
//     rtcController.setWakeAlarm();
//     while (1) {
//       delay(1000);
//       Serial.print(count);
//       Serial.println(" seconds");
//       count++;
//       if (rtcController.alarmDone())
//         break;
//     }
//     Serial.println("Wake alarm triggered!");
//     delay(2000);
//     Serial.println("Setting 1 min poll alarm");
//     count = 0;
//     rtcController.setPollAlarm();
//     while (1) {
//       delay(1000);
//       Serial.print(count);
//       Serial.println(" seconds");
//       count++;
//       if (rtcController.alarmDone())
//         break;
//     }
//     Serial.println("Poll alarm triggered!");
//     delay(2000);
//     Serial.print("Collecting Status");
//     rtcController.status(model);
//     model.print();
//     model.clear();
//     break;

//     // TODO update routine so we can dynamically change props and test
//     // log frequency not effecting logging rate?
//     // model not getting updated
//   case '4':
//     Serial.println("\n\nTesting GNSSController");
//     delay(2000);
//     Serial.println("Polling for data");
//     pmController.enableGNSS();
//     rtcController.setPollAlarm();
//     while (1) {
//       if (gnssController->poll(model))
//         model.print();
//       if (rtcController.alarmDone())
//         break;
//     }
//     pmController.disableGNSS();
//     gnssController->status(model);
//     model.print();
//     model.clear();
//     break;

//   case '5':
//     Serial.println("\n\nTesting FSController");
//     delay(2000);
//     Serial.print("Creating new timestampped directory: ");
//     delay(2000);
//     Serial.println(rtcController.getTimestamp());
//     if (fsController.setupWakeCycle(rtcController.getTimestamp(),
//                                     gnssController->getFormat()))
//       Serial.println("Timestampped directory created!");
//     else {
//       Serial.println("Throwing write error");
//       model.setError(WRITE_ERR);
//       Serial.println(model.toError());
//       fsController.logDiag(model.toError());
//     }
//     delay(2000);
//     Serial.println("Simulated Wake...");
//     pmController.enableGNSS();
//     rtcController.setPollAlarm();
//     while (1) {
//       if (gnssController->poll(model))
//         fsController.logData(model.toData(0));
//       if (rtcController.alarmDone())
//         break;
//     }
//     pmController.disableGNSS();
//     Serial.println("Wake Cycle complete!");
//     delay(2000);
//     if (fsController.setupWakeCycle(rtcController.getTimestamp(),
//                                     gnssController->getFormat()))
//       Serial.println("Timestampped directory created!");
//     else {
//       Serial.println("Throwing write error");
//       model.setError(WRITE_ERR);
//       Serial.println(model.toError());
//       fsController.logDiag(model.toError());
//     }
//     delay(2000);
//     Serial.println("Throwing random error");
//     delay(2000);
//     model.setError(ACK_ERR);
//     Serial.println(model.toError());
//     fsController.logDiag(model.toError());
//     Serial.println("Get state of device");
//     delay(2000);
//     manager.status(model);
//     Serial.println("Status of machine: ");
//     model.print();
//     delay(2000);
//     Serial.println("Created diagnostic and state packet:");
//     Serial.println(model.toDiag());
//     Serial.println(model.toState());
//     Serial.println("Writing data to SD...");
//     delay(2000);
//     fsController.logDiag(model.toDiag());
//     fsController.logDiag(model.toState());
//     Serial.println("Complete");
//     break;

//   case '6':
//     Serial.println("\n\nTesting COMController");
//     delay(2000);
//     Serial.println("Creating Request packet");
//     manager.status(model);
//     Serial.println(model.toDiag());
//     comController->request(model);
//     delay(2000);
//     Serial.println("Creating Upload Packet");
//     Serial.println(model.toData(3));
//     comController->upload(model);
//     model.clear();
//     Serial.println("Handling response");
//     delay(2000);
//     model.handleRes(test);
//     Serial.println("Collecting status");
//     delay(2000);
//     comController->status(model);
//     model.print();
//     model.clear();
//     break;

//   case '7':
//     Serial.println("\n\nTesting COMController");
//     Serial.println("Creating Request packet");
//     manager.status(model);
//     Serial.println(model.toDiag());
//     comController->request(model);
//     break;
//   }
// }

// /******** Diagnostics ********/
// #define IMU_FLAG 0
// #define BAT 1
// #define SPACE 2
// #define CYCLES 3
// #define DROPPED_PKTS 4
// #define ERR_COUNT 5

// /******** Data ********/
// #define FIX_MODE 0
// #define GPS_TIME_WN 1
// #define GPS_TIME_TOW 2
// #define POS_LLH_LAT 3
// #define POS_LLH_LON 4
// #define POS_LLH_HEIGHT 5
// #define POS_LLH_N_SATS 6
// #define BASELINE_N 7
// #define BASELINE_E 8
// #define BASELINE_D 9
// #define VEL_N 10
// #define VEL_E 11
// #define VEL_D 12
// #define DOPS_GDOP 13
// #define DOPS_HDOP 14
// #define DOPS_PDOP 15
// #define DOPS_TDOP 16
// #define DOPS_VDOP 17

// void SSModel::clear() {
//   // clear state
//   m_propHandler.clear();

//   // clear diagnostic
//   m_imu_flag = false;
//   m_bat = 0;
//   m_space = 0;
//   m_cycles = 0;
//   m_dropped_pkts = 0;
//   m_err_count = 0;

//   // clear data
//   m_mode = 0;
//   m_gps_time.wn = 0;
//   m_gps_time.tow = 0;
//   m_pos_llh.flags = 0;
//   m_pos_llh.lat = 0;
//   m_pos_llh.lon = 0;
//   m_pos_llh.height = 0;
//   m_pos_llh.n_sats = 0;
//   m_baseline_ned.n = 0;
//   m_baseline_ned.e = 0;
//   m_baseline_ned.d = 0;
//   m_vel_ned.n = 0;
//   m_vel_ned.e = 0;
//   m_vel_ned.d = 0;
//   m_dops.gdop = 0;
//   m_dops.hdop = 0;
//   m_dops.pdop = 0;
//   m_dops.tdop = 0;
//   m_dops.vdop = 0;
// }
