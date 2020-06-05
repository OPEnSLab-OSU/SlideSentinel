#ifndef _BASEMODEL_H_
#define _BASEMODEL_H_

#include <Arduino.h>
#include "ArduinoJson.h"
#include "Diagnostics.h"
#include "Properties.h"
#include "Shadow.h"
#include "rover_config.h"

#define BASE_DIAG "BDIAG"
#define PACKET "PKT"
#define NUM_ROVER "NUM"
#define SHADOW "SHDW"

#define ID_FLAG 0
#define DIAG_FLAG 1
#define PROP_FLAG 2
#define DATA_FLAG 3

using namespace errorMsg;

// TODO maintain a timer for the last time each rover checked in with the base station
class BaseModel {
private:
  int m_numRovers;
  int m_roverServeId;       // Id of the rover being serviced, the base expects an upload from this rover
  int m_roverRecentId;      // Id of the rover which most recently made contact with the base
  Shadow *m_shadow;
  char *m_err;

  unsigned long m_stopwatch;
  int m_num_uploads;
  int m_num_requests;
  float m_sdSpace;
  uint8_t m_lastSdError;
  char m_buf[MAX_DATA_LEN];
  void m_clear();

  void m_add(int id, JsonArray &data, uint8_t sel);

public:
  BaseModel(int numRovers);
  
  // produces uploadable packet consisting of diagnostics and positional data
  char *toPacket(int id, uint8_t sel);
  char *toShadow();
  char *getBaseDiagnostics();
  
  char *getDiag(int id);
  bool getRoverIMUFlag(int id);
  int getRoverWakeTime(int id);

  char *getProps(int id);
  char *getData(int id);
  void setDiag(int id, char *buf);
  void setProps(int id, char *buf);
  void setData(int id, char *buf);
  void print();

  // id of the currently serviced rover
  void setRoverServe(int rover_id);
  void setRoverRecent(int rover_id);
  int getRoverServe();
  int getRoverRecent();

  void setError(char *err);
  char *getError();

  // internal diagnostics
  void setStopwatch(unsigned long stopwatch);
  void setNumUploads(int num_uploads);
  void setNumRequests(int num_requests);
  void setSdSpace(float sdSpace);
  void setSdError(uint8_t sderror);
};

#endif // _BASEMODEL_H_