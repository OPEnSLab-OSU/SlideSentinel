#include "BaseModel.h"
#include <Plog.h>

BaseModel::BaseModel(int numRovers) : m_numRovers(numRovers) {
  m_shadow = new Shadow[numRovers];
  StaticJsonDocument<MAX_DATA_LEN> doc;
  JsonArray data = doc.createNestedArray(SS_PROP);
  char buf[100];
  data.add(DEF_TIMOUT);
  data.add(DEF_RETRIES);
  data.add(DEF_WAKE_TIME);
  data.add(DEF_SLEEP_TIME);
  data.add(DEF_SENSITIVITY);
  data.add(DEF_LOG_FREQ);
  data.add(DEF_THRESHOLD);
  for (int i = 0; i < m_numRovers; i++) {
    serializeJson(doc, buf);
    setProps(i + 1, buf);
    m_shadow[i].setId(i + 1);
  }
}

char *BaseModel::getDiag(int id) { return m_shadow[id - 1].toDiag(); }

bool BaseModel::getRoverIMUFlag(int id) {
  return m_shadow[id - 1].getIMUFlag();
}

int BaseModel::getRoverWakeTime(int id) {
  return m_shadow[id - 1].getWakeTime();
}

char *BaseModel::getProps(int id) { return m_shadow[id - 1].toProps(); }

char *BaseModel::getData(int id) { return m_shadow[id - 1].toData(); }

void BaseModel::setDiag(int id, char *buf) { m_shadow[id - 1].setDiag(buf); }

void BaseModel::setProps(int id, char *buf) { m_shadow[id - 1].setProps(buf); }

void BaseModel::setData(int id, char *buf) { m_shadow[id - 1].setData(buf); }

void BaseModel::print() {
  for (int i = 0; i < m_numRovers; i++) {
    LOGD << "Rover " << i << " basemodel: ";
    m_shadow[i].print();
  }
}

void BaseModel::setError(char *err) { m_err = err; }

char *BaseModel::getError() { return m_err; }

void BaseModel::setRoverServe(int rover_id) { m_roverServeId = rover_id; }
void BaseModel::setRoverRecent(int rover_id) { m_roverRecentId = rover_id; }

int BaseModel::getRoverServe() { return m_roverServeId; }
int BaseModel::getRoverRecent() { return m_roverRecentId; }

void BaseModel::m_clear() { memset(m_buf, '\0', sizeof(char) * MAX_DATA_LEN); }
void BaseModel::setStopwatch(unsigned long stopwatch) {
  m_stopwatch = stopwatch;
}
void BaseModel::setNumUploads(int num_uploads) { m_num_uploads = num_uploads; }
void BaseModel::setNumRequests(int num_requests) {
  m_num_requests = num_requests;
}
void BaseModel::setSdSpace(float sdSpace) { m_sdSpace = sdSpace; }
void BaseModel::setSdError(uint8_t sderror) { m_lastSdError = sderror; }

char *BaseModel::getBaseDiagnostics() {
  StaticJsonDocument<MAX_DATA_LEN> doc;
  JsonArray data = doc.createNestedArray(BASE_DIAG);
  data.add(m_stopwatch);
  data.add(m_num_uploads);
  data.add(m_num_requests);
  data.add(m_sdSpace);
  data.add(m_lastSdError);
  m_clear();
  serializeJson(doc, m_buf);
  return m_buf;
}

void BaseModel::m_add(int id, JsonArray &data, uint8_t sel) {
  /*
  if (sel & (1 << ID_FLAG))
    data.add(m_shadow[id].toId());
  if (sel & (1 << DIAG_FLAG))
    data.add(m_shadow[id].toDiag());
  if (sel & (1 << PROP_FLAG))
    data.add(m_shadow[id].toData());
  if (sel & (1 << DATA_FLAG))
    data.add(m_shadow[id].toProps());
*/
  data.add(m_shadow[id].toId());
  data.add(m_shadow[id].toDiag());
  data.add(m_shadow[id].toData());
}

char *BaseModel::toShadow() {
  StaticJsonDocument<MAX_DATA_LEN> doc;
  doc[NUM_ROVER] = m_numRovers;
  JsonArray data = doc.createNestedArray(SHADOW);
  for (int i = 0; i < m_numRovers; i++)
    m_add(i, data, (_BV(ID_FLAG) | _BV(DIAG_FLAG) | _BV(PROP_FLAG)));
  m_clear();
  serializeJson(doc, m_buf);
  return m_buf;
}

char *BaseModel::toPacket(int id, uint8_t sel) {
  StaticJsonDocument<MAX_DATA_LEN> doc;
  JsonArray data = doc.createNestedArray(PACKET);
  m_add(id - 1, data, sel);
  m_clear();
  serializeJson(doc, m_buf);
  return m_buf;
}
