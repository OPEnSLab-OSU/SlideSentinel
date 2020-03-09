#include "SSModel.h"
#include "Console.h"

// FIXME Error count is wonky
SSModel::SSModel(int rover_id) : m_id(rover_id), m_err_count(0) { clear(); }

// This data transfer protocol will ensure that -1 are received for invalid
// This function does not perform error handling, however a deserialization
// error will only occur if bytes get dropped across the RF link
void SSModel::handleRes(char *buf) {
  StaticJsonDocument<MAX_DATA_LEN> json;
  deserializeJson(json, buf);
  m_stateData[TIMEOUT] = json[SS_STATE][TIMEOUT];
  m_stateData[RETRIES] = json[SS_STATE][RETRIES];
  m_stateData[WAKE_TIME] = json[SS_STATE][WAKE_TIME];
  m_stateData[SLEEP_TIME] = json[SS_STATE][SLEEP_TIME];
  m_stateData[SENSITIVITY] = json[SS_STATE][SENSITIVITY];
  m_stateData[LOG_FREQ] = json[SS_STATE][LOG_FREQ];
  console.debug("\n**** CONFIG RECEIVED ****\n");
  console.debug("Timeout: ");
  console.debug(m_stateData[TIMEOUT]);
  console.debug("\nRetries: ");
  console.debug(m_stateData[RETRIES]);
  console.debug("\nWake Time: ");
  console.debug(m_stateData[WAKE_TIME]);
  console.debug("\nSleep Time: ");
  console.debug(m_stateData[SLEEP_TIME]);
  console.debug("\nSensitivity: ");
  console.debug(m_stateData[SENSITIVITY]);
  console.debug("\nLog Frequency: ");
  console.debug(m_stateData[LOG_FREQ]);
  console.debug("\n*************************\n");
}

char *SSModel::toReq() {
  StaticJsonDocument<MAX_DATA_LEN> doc;
  m_addId(doc);
  m_addHeader(doc, REQ);
  m_addDiag(doc);
  if (!m_serializePkt(doc))
    setError(SER_ERR);
  return m_buffer;
}

char *SSModel::toUpl() {
  StaticJsonDocument<MAX_DATA_LEN> doc;
  m_addId(doc);
  m_addHeader(doc, UPL);
  m_addDiag(doc);
  // only add data if we have an RTK fix
  if (m_mode >= 4)
    m_addData(doc);
  if (!m_serializePkt(doc))
    setError(SER_ERR);
  return m_buffer;
}

char *SSModel::toDiag() {
  StaticJsonDocument<MAX_DATA_LEN> doc;
  m_addDiag(doc);
  if (!m_serializePkt(doc))
    setError(SER_ERR);
  return m_buffer;
}

char *SSModel::toState() {
  StaticJsonDocument<MAX_DATA_LEN> doc;
  m_addState(doc);
  if (!m_serializePkt(doc))
    setError(SER_ERR);
  return m_buffer;
}

char *SSModel::toData() {
  StaticJsonDocument<MAX_DATA_LEN> doc;
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

void SSModel::m_addState(JsonDocument &doc) {
  JsonArray data = doc.createNestedArray(SS_STATE);
  data.add(m_stateData[TIMEOUT]);
  data.add(m_stateData[RETRIES]);
  data.add(m_stateData[WAKE_TIME]);
  data.add(m_stateData[SLEEP_TIME]);
  data.add(m_stateData[SENSITIVITY]);
  data.add(m_stateData[LOG_FREQ]);
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

void SSModel::m_addHeader(JsonDocument &doc, const char *type) {
  doc[TYPE] = type;
}

void SSModel::m_addId(JsonDocument &doc) { doc[ROVER_ID] = m_id; }

void SSModel::m_clear() { memset(m_buffer, '\0', sizeof(char) * MAX_DATA_LEN); }

bool SSModel::valid(int val) {
  if (val >= 0)
    return true;
  return false;
}

int SSModel::timeout() { return m_stateData[TIMEOUT]; }
int SSModel::retries() { return m_stateData[RETRIES]; }
int SSModel::wakeTime() { return m_stateData[WAKE_TIME]; }
int SSModel::sleepTime() { return m_stateData[SLEEP_TIME]; }
int SSModel::logFreq() { return m_stateData[LOG_FREQ]; }
int SSModel::sensitivity() { return m_stateData[SENSITIVITY]; }

// GNSSController
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
void SSModel::setLogFreq(uint32_t logFreq) { m_stateData[LOG_FREQ] = logFreq; }

// RTCController
void SSModel::setWakeTime(uint16_t wakeTime) {
  m_stateData[WAKE_TIME] = wakeTime;
}
void SSModel::setSleepTime(uint16_t sleepTime) {
  m_stateData[SLEEP_TIME] = sleepTime;
}

// COMController
void SSModel::setTimeout(uint16_t timeout) { m_stateData[TIMEOUT] = timeout; }
void SSModel::setRetries(uint8_t retries) { m_stateData[RETRIES] = retries; }
void SSModel::setDropped_pkts(uint16_t dropped_pkts) {
  m_dropped_pkts = dropped_pkts;
}

// IMUController
void SSModel::setSensitivity(uint8_t sensitivity) {
  m_stateData[SENSITIVITY] = sensitivity;
}
void SSModel::setIMUflag(bool imu_flag) { m_imu_flag = imu_flag; }

// PMController
void SSModel::setBat(float bat) { m_bat = bat; }

// FSController
void SSModel::setSpace(uint32_t space) { m_space = space; }
void SSModel::setCycles(uint16_t cycles) { m_cycles = cycles; }

// Error
void SSModel::setError(const char *err) {
  m_err_count++;
  m_err = (char *)err;
}

void SSModel::print() {
  console.debug("\n\n------- CONFIG -------\n");
  console.debug("Timeout: ");
  console.debug(m_stateData[TIMEOUT]);
  console.debug("\nRetries: ");
  console.debug(m_stateData[RETRIES]);
  console.debug("\nWake Time: ");
  console.debug(m_stateData[WAKE_TIME]);
  console.debug("\nSleep Time: ");
  console.debug(m_stateData[SLEEP_TIME]);
  console.debug("\nSensitivity: ");
  console.debug(m_stateData[SENSITIVITY]);
  console.debug("\nLog Frequency: ");
  console.debug(m_stateData[LOG_FREQ]);

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
  for (int i = 0; i < NUM_STATE; i++)
    m_stateData[i] = INVALID_STATE;

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
