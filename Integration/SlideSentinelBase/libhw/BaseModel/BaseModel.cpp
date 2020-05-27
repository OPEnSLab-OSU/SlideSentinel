#include "BaseModel.h"
#include <Plog.h>

BaseModel::BaseModel(int numRovers) : m_numRovers(numRovers) {
  m_shadow = new Shadow[numRovers];
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
void BaseModel::setRoverAlert(int rover_id) { m_roverAlertId = rover_id; }

int BaseModel::getRoverServe() { return m_roverServeId; }
int BaseModel::getRoverAlert() { return m_roverAlertId; }

// unsigned long m_stopwatch;
// int m_num_uploads;
// int m_num_requests;
// float m_sdSpace;
// char m_buf[MAX_DATA_LEN];

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

char *BaseModel::getRoverShadow() {
  StaticJsonDocument<MAX_DATA_LEN> doc;
  char buf[255];
  for (int i = 0; i < m_numRovers; i++) {
    itoa(i, buf, 10);
    JsonArray data = doc.createNestedArray(buf);
    data.add(m_shadow[i].toDiag());
    data.add(m_shadow[i].toProps());
  }
  m_clear();
  serializeJson(doc, m_buf);
  return m_buf;
}