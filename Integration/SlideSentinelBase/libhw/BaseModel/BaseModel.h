#ifndef _BASEMODEL_H_
#define _BASEMODEL_H_

#include <Arduino.h>
#include "ArduinoJson.h"
#include "Diagnostics.h"
#include "Properties.h"
#include "Shadow.h"
#include <Arduino.h>

#define BASE_DIAG "BDIAG"

using namespace errorMsg;

class BaseModel {
private:
  int m_numRovers;
  int m_roverServeId;
  int m_roverAlertId;
  Shadow *m_shadow;
  char *m_err;

  unsigned long m_stopwatch;
  int m_num_uploads;
  int m_num_requests;
  float m_sdSpace;
  uint8_t m_lastSdError;
  char m_buf[MAX_DATA_LEN];
  void m_clear();

public:
  BaseModel(int numRovers);
  bool dataReady();
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
  void setRoverAlert(int rover_id);
  int getRoverServe();
  int getRoverAlert();

  void setError(char *err);
  char *getError();

  // internal diagnostics
  void setStopwatch(unsigned long stopwatch);
  void setNumUploads(int num_uploads);
  void setNumRequests(int num_requests);
  void setSdSpace(float sdSpace);
  void setSdError(uint8_t sderror);
  char *getBaseDiagnostics();
  char *getRoverShadow();
};

#endif // _BASEMODEL_H_