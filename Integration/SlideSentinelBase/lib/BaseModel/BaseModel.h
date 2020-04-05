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
  Shadow *m_shadow;
  char* m_err

public:
  BaseModel(int numRovers);
  bool dataReady();
  char *getDiag(int id);
  char *getProps(int id);
  char *getData(int id);
  void setDiag(int id, char *buf);
  void setProps(int id, char *buf);
  void setData(int id, char *buf);
  void print();

  void setError(char *err);
  char* getError();
};

#endif // _BASEMODEL_H_