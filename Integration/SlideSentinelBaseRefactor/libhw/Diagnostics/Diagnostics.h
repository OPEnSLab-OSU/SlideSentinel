#ifndef _DIAGNOSTICS_H_
#define _DIAGNOSTICS_H_

#include <Arduino.h>
#include "ArduinoJson.h"

#define SS_DIAG "DIAG"
#define MAX_DIAG_LEN 200
#define IMU_FLAG 0
#define BAT 1
#define SPACE 2
#define CYCLES 3
#define DROPPED_PKTS 4
#define ERR_COUNT 5
#define CONV_TIME 6

class Diagnostics {
private:
  bool m_imu_flag;
  float m_bat;
  float m_space;
  int m_cycles;
  int m_dropped_pkts;
  int m_err_count;
  float m_convergence_time;

public:
  Diagnostics();
  void write(JsonDocument &doc);
  void read(char *buf);

  // getters
  bool imu();
  float bat();
  float space();
  int cycles();
  int droppedPkts();
  int errCount();
  float convergenceTime();

  // setters
  void setFlag(bool flag);
  void setBat(float bat);
  void setSpace(float space);
  void setCycles(int cycles);
  void setDroppedPkts(int drop);
  void setErrCount(int errCount);
  void setConvergenceTime(float conv);

  void clear();
  void print();
};

#endif // _DIAGNOSTICS_H_
