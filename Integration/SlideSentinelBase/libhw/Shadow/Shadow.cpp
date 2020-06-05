#include "Shadow.h"
#include <Plog.h>

Shadow::Shadow() : m_data{0} { m_props.init(); }

void Shadow::m_clear() { memset(m_buf, '\0', sizeof(char) * MAX_DATA_LEN); }

int Shadow::getId() { return m_id; }

void Shadow::setId(int id) {
  m_id = id;
}

char *Shadow::toProps() {
  StaticJsonDocument<MAX_DATA_LEN> doc;
  m_props.write(doc);
  m_serializePkt(doc);
  return m_buf;
}

char *Shadow::toDiag() {
  StaticJsonDocument<MAX_DATA_LEN> doc;
  m_diag.write(doc);
  m_serializePkt(doc);
  return m_buf;
}

char *Shadow::toData() {
  m_clear();
  strcpy(m_buf, m_data);
  StaticJsonDocument<MAX_DATA_LEN> doc;
  JsonArray data = doc.createNestedArray(SS_DATA);
  data.add(m_buf);
  m_serializePkt(doc);
  return m_buf;
}

char *Shadow::toId() {
  StaticJsonDocument<MAX_DATA_LEN> doc;
  doc[SS_ID] = m_id;
  m_serializePkt(doc);
  return m_buf;
}

void Shadow::m_serializePkt(JsonDocument &doc) {
  m_clear();
  serializeJson(doc, m_buf);
}

void Shadow::print() {
  m_props.print();
  m_diag.print();
  LOGD << m_data;
}

void Shadow::setProps(char *buf) { m_props.read(buf); }

void Shadow::setDiag(char *buf) { m_diag.read(buf); }

void Shadow::setData(char *buf) { strcpy(m_data, buf); }

bool Shadow::getIMUFlag() { return m_diag.imu(); }

int Shadow::getWakeTime() { return m_props.get(WAKE_TIME); }
