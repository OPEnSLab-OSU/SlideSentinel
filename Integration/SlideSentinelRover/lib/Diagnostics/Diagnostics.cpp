#include "Diagnostics.h"
#include "Console.h"

Diagnostics::Diagnostics() { clear(); }

void Diagnostics::write(JsonDocument &doc) {
  JsonArray data = doc.createNestedArray(SS_DIAG);
  data.add(m_imu_flag);
  data.add(m_bat);
  data.add(m_space);
  data.add(m_cycles);
  data.add(m_dropped_pkts);
}

void Diagnostics::read(char *buf) {
  StaticJsonDocument<MAX_DIAG_LEN> json;
  auto err = deserializeJson(json, buf);
  if (err) {
    clear();
    return;
  }
  setFlag(json[IMU_FLAG]);
  setBat(json[BAT]);
  setSpace(json[SPACE]);
  setCycles(json[CYCLES]);
  setDroppedPkts(json[DROPPED_PKTS]);
  setErrCount(json[ERR_COUNT]);

  console.debug("\n\n**** DIAGNOSTICS RECEIVED ****\n");
  console.debug("IMU Flag: ");
  console.debug(imu());
  console.debug("\nBattery Voltage: ");
  console.debug(bat());
  console.debug("\nSD Space: ");
  console.debug(space());
  console.debug("\nWake Cycles: ");
  console.debug(cycles());
  console.debug("\nDropped Packets: ");
  console.debug(droppedPkts());
  console.debug("\nError Count: ");
  console.debug(errCount());
  console.debug("\n*************************\n\n");
}

// getters
bool Diagnostics::imu() { return m_imu_flag; }
float Diagnostics::bat() { return m_bat; }
float Diagnostics::space() { return m_space; }
int Diagnostics::cycles() { return m_cycles; }
int Diagnostics::droppedPkts() { return m_dropped_pkts; }
int Diagnostics::errCount() { return m_err_count; }

// setters
void Diagnostics::setFlag(bool flag) { m_imu_flag = flag; }
void Diagnostics::setBat(float bat) { m_bat = bat; }
void Diagnostics::setSpace(float space) { m_space = space; }
void Diagnostics::setCycles(int cycles) { m_cycles = cycles; }
void Diagnostics::setDroppedPkts(int drop) { m_dropped_pkts = drop; }
void Diagnostics::setErrCount(int errCount) { m_err_count = errCount; }

void Diagnostics::clear() {
  setFlag(false);
  setBat(0);
  setSpace(0);
  setCycles(0);
  setDroppedPkts(0);
  setErrCount(0);
}