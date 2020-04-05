#ifndef _PROPERTIES_H_
#define _PROPERTIES_H_

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

class Properties {
private:
  int m_prop[NUM_PROP];

public:
  Properties();
  void write(JsonDocument &doc);
  void read(char* buf);
  bool valid(int prop);
  int get(int prop);
  void set(int prop, int val);
  void init();
  void print();
};

#endif // _PROPERTIES_H_
