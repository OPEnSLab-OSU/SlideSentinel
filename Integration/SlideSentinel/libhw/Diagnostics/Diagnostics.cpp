#include "Diagnostics.h"
#include <Plog.h>

Diagnostics::Diagnostics() { clear(); }

void Diagnostics::write(JsonDocument &doc) {
  JsonArray data = doc.createNestedArray(SS_DIAG);
  data.add(m_imu_flag);
  data.add(m_bat);
  data.add(m_space);
  data.add(m_cycles);
  data.add(m_dropped_pkts);
  data.add(m_err_count);
  data.add(m_convergence_time);
}

void Diagnostics::read(char *buf) {
  StaticJsonDocument<MAX_DIAG_LEN> json;
  auto err = deserializeJson(json, buf);
  if (err) {
    clear();
    return;
  }
  setFlag(json[SS_DIAG][IMU_FLAG]);
  setBat(json[SS_DIAG][BAT]);
  setSpace(json[SS_DIAG][SPACE]);
  setCycles(json[SS_DIAG][CYCLES]);
  setDroppedPkts(json[SS_DIAG][DROPPED_PKTS]);
  setErrCount(json[SS_DIAG][ERR_COUNT]);
  setConvergenceTime(json[SS_DIAG][CONV_TIME]);
}

void Diagnostics::print() {
  LOGD << "**** DIAGNOSTICS ****";
  LOGD << "\tIMU Flag: " << imu();
  LOGD << "\tBattery Voltage: " << bat();
  LOGD << "\tSD Space: " << space();
  LOGD << "\tWake Cycles: " << cycles();
  LOGD << "\tDropped Packets: " << droppedPkts();
  LOGD << "\tError Count: " << errCount();
  LOGD << "\tConvergence Time: " << convergenceTime();
  LOGD << "*************************";
}

void Diagnostics::print_serial(){
  Serial.println("**** DIAGNOSTICS ****");
  Serial.println("\tIMU Flag: " + imu());
  Serial.println("\tBattery Voltage: " + String(bat()));
  Serial.println("\tSD Space: " + String(space()));
  Serial.println("\tWake Cycles: " + cycles());
  Serial.println("\tDropped Packets: " + droppedPkts());
  Serial.println("\tError Count: " + errCount());
  Serial.println("\tConvergence Time: " + String(convergenceTime()));
  Serial.println("*************************");
}

// getters
bool Diagnostics::imu() { return m_imu_flag; }
float Diagnostics::bat() { return m_bat; }
float Diagnostics::space() { return m_space; }
int Diagnostics::cycles() { return m_cycles; }
int Diagnostics::droppedPkts() { return m_dropped_pkts; }
int Diagnostics::errCount() { return m_err_count; }
float Diagnostics::convergenceTime() { return m_convergence_time; }

// setters
void Diagnostics::setFlag(bool flag) { m_imu_flag = flag; }
void Diagnostics::setBat(float bat) { m_bat = bat; }
void Diagnostics::setSpace(float space) { m_space = space; }
void Diagnostics::setCycles(int cycles) { m_cycles = cycles; }
void Diagnostics::setDroppedPkts(int drop) { m_dropped_pkts = drop; }
void Diagnostics::setErrCount(int errCount) { m_err_count = errCount; }
void Diagnostics::setConvergenceTime(float conv) { m_convergence_time = conv; }

void Diagnostics::clear() {
  setFlag(false);
  setBat(0);
  setSpace(0);
  setCycles(0);
  setDroppedPkts(0);
  setErrCount(0);
  setConvergenceTime(0);
}