#ifndef _BASEMODEL_H_
#define _BASEMODEL_H_

#include <Arduino.h>
#include "ArduinoJson.h"
#include "Diagnostics.h"
#include "Properties.h"
#include "Shadow.h"

using namespace errorMsg;

class BaseModel {
private:
  int m_numRovers;
  int m_roverServeId;
  int m_roverAlertId;
  Shadow *m_shadow;
  char *m_err;

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
};

#endif // _BASEMODEL_H_