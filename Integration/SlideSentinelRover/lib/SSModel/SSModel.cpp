#include "SSModel.h"
#include "Console.h"

// TODO build out error logging so we know what state we are in when
// errors occur
// TODO error handle on rover so configurable state data is only ever changed if
// parameterized properly
// FIXME Error count is wonky
SSModel::SSModel() : m_err_count(0) { clear(); }

void SSModel::handleRes(char *buf) { m_propHandler.m_readProp(buf); }

char *SSModel::toDiag() {
  StaticJsonDocument<MAX_DATA_LEN> doc;
  m_addDiag(doc);
  if (!m_serializePkt(doc))
    setError(SER_ERR);
  return m_buffer;
}

char *SSModel::toProp() {
  StaticJsonDocument<MAX_DATA_LEN> doc;
  m_propHandler.m_writeProp(doc);
  if (!m_serializePkt(doc))
    setError(SER_ERR);
  return m_buffer;
}

// TODO test the outcome when we get a value lower than threshold
char *SSModel::toData(int threshold) {
  StaticJsonDocument<MAX_DATA_LEN> doc;
  if (m_mode >= threshold)
    m_addData(doc);
  if (!m_serializePkt(doc))
    setError(SER_ERR);
  return m_buffer;
}

char *SSModel::toError() { return m_err; }

bool SSModel::m_serializePkt(JsonDocument &doc) {
  m_clear();
  auto error = serializeJson(doc, m_buffer);
  if (error)
    return false;
  return true;
}

void SSModel::m_addDiag(JsonDocument &doc) {
  JsonArray data = doc.createNestedArray(SS_DIAGNOSTIC);
  data.add(m_imu_flag);
  data.add(m_bat);
  data.add(m_space);
  data.add(m_cycles);
  data.add(m_dropped_pkts);
}

void SSModel::m_addData(JsonDocument &doc) {
  JsonArray data = doc.createNestedArray(SS_DATA);
  data.add(m_mode);
  data.add(m_gps_time.wn);
  data.add(m_gps_time.tow);
  data.add(m_pos_llh.lat);
  data.add(m_pos_llh.lon);
  data.add(m_pos_llh.height);
  data.add(m_pos_llh.n_sats);
  data.add(m_baseline_ned.n);
  data.add(m_baseline_ned.e);
  data.add(m_baseline_ned.d);
  data.add(m_vel_ned.n);
  data.add(m_vel_ned.e);
  data.add(m_vel_ned.d);
  data.add(m_dops.gdop);
  data.add(m_dops.hdop);
  data.add(m_dops.pdop);
  data.add(m_dops.tdop);
  data.add(m_dops.vdop);
}

void SSModel::m_clear() { memset(m_buffer, '\0', sizeof(char) * MAX_DATA_LEN); }

int SSModel::getProp(int prop) { return m_propHandler.get(prop); }
void SSModel::setProp(int prop, int val) { m_propHandler.set(prop, val); }
bool SSModel::validProp(int prop) { return m_propHandler.valid(prop); }

void SSModel::setPos_llh(msg_pos_llh_t pos_llh) { m_pos_llh = pos_llh; }
void SSModel::setBaseline_ned(msg_baseline_ned_t baseline_ned) {
  m_baseline_ned = baseline_ned;
}
void SSModel::setMsg_vel_ned_t(msg_vel_ned_t vel_ned) { m_vel_ned = vel_ned; }
void SSModel::setMsg_dops_t(msg_dops_t dops) { m_dops = dops; }
void SSModel::setMsg_gps_time_t(msg_gps_time_t gps_time) {
  m_gps_time = gps_time;
}
void SSModel::setMode(uint8_t mode) { m_mode = mode; }
void SSModel::setDropped_pkts(uint16_t dropped_pkts) {
  m_dropped_pkts = dropped_pkts;
}
void SSModel::setIMUflag(bool imu_flag) { m_imu_flag = imu_flag; }
void SSModel::setBat(float bat) { m_bat = bat; }
void SSModel::setSpace(uint32_t space) { m_space = space; }
void SSModel::setCycles(uint16_t cycles) { m_cycles = cycles; }

void SSModel::setError(const char *err) {
  m_err_count++;
  m_err = (char *)err;
}

void SSModel::print() {
  console.debug("\n\n------- CONFIG -------\n");
  console.debug("Timeout: ");
  console.debug(getProp(TIMEOUT));
  console.debug("\nRetries: ");
  console.debug(getProp(RETRIES));
  console.debug("\nWake Time: ");
  console.debug(getProp(WAKE_TIME));
  console.debug("\nSleep Time: ");
  console.debug(getProp(SLEEP_TIME));
  console.debug("\nSensitivity: ");
  console.debug(getProp(SENSITIVITY));
  console.debug("\nLog Frequency: ");
  console.debug(getProp(LOG_FREQ));

  console.debug("\n\n------- DIAGNOSTIC -------\n");
  console.debug("\nIMU Flag: ");
  if (m_imu_flag)
    console.debug("TRUE");
  else
    console.debug("FALSE");
  console.debug("\nBattery: ");
  console.debug(m_bat);
  console.debug("\nSD Card Space: ");
  console.debug(m_space);
  console.debug("\nCycles: ");
  console.debug(m_cycles);
  console.debug("\nDropped Packets: ");
  console.debug(m_dropped_pkts);
  console.debug("\nError Count: ");
  console.debug(m_err_count);

  console.debug("\n\n------- DATA -------\n");
  console.debug("\nGPS time week: ");
  console.debug((int)m_gps_time.wn);
  console.debug("\nGPS time second: ");
  console.debug(((float)m_gps_time.tow) / 1e3);
  console.debug("\nRTK mode: ");
  console.debug(m_mode);
  console.debug("\nLatitude: ");
  console.debug(m_pos_llh.lat);
  console.debug("\nLongitude: ");
  console.debug(m_pos_llh.lon);
  console.debug("\nHeight: ");
  console.debug(m_pos_llh.height);
  console.debug("\nSatellites: ");
  console.debug(m_pos_llh.n_sats);
  console.debug("\nBaseline N: ");
  console.debug((int)m_baseline_ned.n);
  console.debug("\nBaseline E: ");
  console.debug((int)m_baseline_ned.e);
  console.debug("\nBaseline D: ");
  console.debug((int)m_baseline_ned.d);
  console.debug("\nVelocity N: ");
  console.debug((int)m_vel_ned.n);
  console.debug("\nVelocity E: ");
  console.debug((int)m_vel_ned.e);
  console.debug("\nVelocity D: ");
  console.debug((int)m_vel_ned.d);
  console.debug("\nGDOP: ");
  console.debug(((float)m_dops.gdop / 100));
  console.debug("\nHDOP: ");
  console.debug(((float)m_dops.hdop / 100));
  console.debug("\nPDOP: ");
  console.debug(((float)m_dops.pdop / 100));
  console.debug("\nTDOP: ");
  console.debug(((float)m_dops.tdop / 100));
  console.debug("\nVDOP: ");
  console.debug(((float)m_dops.vdop / 100));
  console.debug("\n");
}

void SSModel::clear() {
  // clear state
  m_propHandler.clear();

  // clear diagnostic
  m_imu_flag = false;
  m_bat = 0;
  m_space = 0;
  m_cycles = 0;
  m_dropped_pkts = 0;
  m_err_count = 0;

  // clear data
  m_mode = 0;
  m_gps_time.wn = 0;
  m_gps_time.tow = 0;
  m_pos_llh.flags = 0;
  m_pos_llh.lat = 0;
  m_pos_llh.lon = 0;
  m_pos_llh.height = 0;
  m_pos_llh.n_sats = 0;
  m_baseline_ned.n = 0;
  m_baseline_ned.e = 0;
  m_baseline_ned.d = 0;
  m_vel_ned.n = 0;
  m_vel_ned.e = 0;
  m_vel_ned.d = 0;
  m_dops.gdop = 0;
  m_dops.hdop = 0;
  m_dops.pdop = 0;
  m_dops.tdop = 0;
  m_dops.vdop = 0;
}
