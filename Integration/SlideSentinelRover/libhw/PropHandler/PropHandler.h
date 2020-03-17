#ifndef _PROPHANDLER_H_
#define _PROPHANDLER_H_

#include <Arduino.h>
#include "ArduinoJson.h"

#define SS_PROP "PROP"
#define MAX_PROP_LEN 200  
#define NUM_PROP 7
#define INVALID_PROP -1
#define TIMEOUT 0
#define RETRIES 1
#define WAKE_TIME 2
#define SLEEP_TIME 3
#define SENSITIVITY 4
#define LOG_FREQ 5
#define THRESHOLD 6

class PropHandler {
private:
  int m_prop[NUM_PROP];
  void m_clear();

public:
  PropHandler();
  void m_writeProp(JsonDocument &doc);
  void m_readProp(char* buf);
  bool valid(int prop);
  int get(int prop);
  void set(int prop, int val);
  void clear();
};

#endif // _PROPHANDLER_H_
