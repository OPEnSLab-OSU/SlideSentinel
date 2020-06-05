#ifndef _SHADOW_H_
#define _SHADOW_H_

#define SS_DATA "GNSS"
#define SS_ID "ID"

#include <Arduino.h>
#include <string.h>
#include "ArduinoJson.h"
#include "Diagnostics.h"
#include "Properties.h"
#include "constants.hpp"

class Shadow {
private:
  Properties m_props;
  Diagnostics m_diag;
  int m_id; 
  char m_data[MAX_DATA_LEN];
  char m_buf[MAX_DATA_LEN];
  void m_clear();
  void m_serializePkt(JsonDocument &doc);

public:
  Shadow();
  int getId();
  void setId(int id);
  char *toProps();
  char *toDiag();
  char *toData();
  char *toId();
  void setProps(char *buf);
  void setDiag(char *buf);
  void setData(char *buf);
  bool getIMUFlag();
  int getWakeTime();
  void print();
};

#endif // _SHADOW_H_