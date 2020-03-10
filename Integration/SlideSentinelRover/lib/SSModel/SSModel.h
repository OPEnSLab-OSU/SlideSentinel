#ifndef _SSMODEL_H_
#define _SSMODEL_H_

#define MAX_DATA_LEN 1000

// JSON constants
#define SS_STATE "STATE"     // State
#define SS_DIAGNOSTIC "DIAG" // Diagnostics
#define SS_DATA "DATA"       // Data

// /******** Diagnostics ********/
// #define IMU_FLAG 0
// #define BAT 1
// #define SPACE 2
// #define CYCLES 3
// #define DROPPED_PKTS 4
// #define ERR_COUNT 5

/******** State ********/
#define NUM_STATE 7
#define INVALID_STATE -1
#define TIMEOUT 0
#define RETRIES 1
#define WAKE_TIME 2
#define SLEEP_TIME 3
#define SENSITIVITY 4
#define LOG_FREQ 5
#define THRESHOLD 6

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

// return a status object for each status function
#include <Arduino.h>
#include "ArduinoJson.h"
#include "SwiftPiksi.h"
#include "constants.hpp"

using namespace ErrorMsg;

class SSModel {
private:

  int m_stateData[NUM_STATE]; // state

  bool m_imu_flag;         // diagnostics
  float m_bat;             // diagnostic
  float m_space;           // diagnostic
  uint16_t m_cycles;       // diagnostic
  uint16_t m_dropped_pkts; // diagnostic
  uint16_t m_err_count;    // diagnostic
  char *m_err;             // diagnostic

  // GNSSController
  msg_pos_llh_t m_pos_llh;           // data
  msg_baseline_ned_t m_baseline_ned; // data
  msg_vel_ned_t m_vel_ned;           // data
  msg_dops_t m_dops;                 // data
  msg_gps_time_t m_gps_time;         // data
  uint8_t m_mode;                    // data

  // container for all constructed data
  char m_buffer[MAX_DATA_LEN];

  void m_addDiag(JsonDocument &doc);
  void m_addState(JsonDocument &doc);
  void m_addData(JsonDocument &doc);
  void m_addHeader(JsonDocument &doc, const char *type);
  void m_addId(JsonDocument &doc);
  void m_clear();
  bool m_serializePkt(JsonDocument &doc);

public:
  SSModel();

  // checks if STATE data is valid
  bool valid(int val);

  // getters for STATE
  int timeout();
  int retries();
  int wakeTime();
  int sleepTime();
  int logFreq();
  int sensitivity();
  int threshold();

  // GNSSController
  void setPos_llh(msg_pos_llh_t pos_llh);
  void setBaseline_ned(msg_baseline_ned_t baseline_ned);
  void setMsg_vel_ned_t(msg_vel_ned_t vel_ned);
  void setMsg_dops_t(msg_dops_t dops);
  void setMsg_gps_time_t(msg_gps_time_t gps_time);
  void setMode(uint8_t mode);
  void setLogFreq(uint32_t logFreq);

  // RTCController
  void setWakeTime(uint16_t wakeTime);
  void setSleepTime(uint16_t sleepTime);

  // COMController
  void setTimeout(uint16_t timeout);
  void setRetries(uint8_t retries);
  void setDropped_pkts(uint16_t dropped_pkts);
  void setThreshold(uint8_t threshold);

  // IMUController
  void setSensitivity(uint8_t sensitivity);
  void setIMUflag(bool imu_flag);

  // PMController
  void setBat(float bat);

  // FSController
  void setSpace(uint32_t space);
  void setCycles(uint16_t cycles);

  // Error
  void setError(const char *err);
  
  // handles response data from the base station COMController::request()
  void handleRes(char *buf);

  // creates a char[] of data relevant to the sorted packet type
  char *toDiag();
  char *toState();
  char *toData(int threshold);
  char *toError();

  void print();
  void clear();
};

#endif // _SSMODEL_H_
