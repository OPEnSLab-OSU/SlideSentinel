#ifndef _SHADOW_H_
#define _SHADOW_H_

#define SS_DATA "GNSS"

#include <Arduino.h>
#include <string.h>
#include "ArduinoJson.h"
#include "Diagnostics.h"
#include "Properties.h"
#include "constants.hpp"

class Shadow {
private:
  uint8_t m_id;
  Properties m_props;
  Diagnostics m_diag;
  char m_data[MAX_DATA_LEN];
  char m_buf[MAX_DATA_LEN];
  void m_clear();
  bool m_serializePkt(JsonDocument &doc);

public:
  Shadow();
  int getId();
  int getWakeTime();
  char *toProps();
  char *toDiag();
  char *toData();
  void setProps(char *buf);
  void setDiag(char *buf);
  void setData(char *buf);
  void print();
};

#endif // _SHADOW_H_