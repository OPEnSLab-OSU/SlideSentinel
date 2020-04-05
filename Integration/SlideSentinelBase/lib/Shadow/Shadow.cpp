#include "Shadow.h"
#include "Console.h"

Shadow::Shadow() { m_props.init(); }

void Shadow::m_clear() { memset(m_buf, '\0', sizeof(char) * MAX_DATA_LEN); }

int Shadow::getId() { return m_id; }

int Shadow::getWakeTime() { return m_props.get(WAKE_TIME); }

char *Shadow::toProps() {
  StaticJsonDocument<MAX_DATA_LEN> doc;
  m_props.write(doc);
  if (!m_serializePkt(doc))
    return NULL;
  return m_buf;
}

char *Shadow::toDiag() {
  StaticJsonDocument<MAX_DATA_LEN> doc;
  m_diag.write(doc);
  if (!m_serializePkt(doc))
    return NULL;
  return m_buf;
}

char *Shadow::toData() {
  m_clear();
  strcpy(m_buf, m_data);
  return m_buf;
}

bool Shadow::m_serializePkt(JsonDocument &doc) {
  m_clear();
  auto error = serializeJson(doc, m_buf);
  if (error)
    return false;
  return true;
}

void Shadow::print() {
  m_props.print();
  m_diag.print();
  console.debug("\n");
  console.debug(m_data);
}

void Shadow::setProps(char *buf) { m_props.read(buf); }

void Shadow::setDiag(char *buf) { m_diag.read(buf); }

void Shadow::setData(char *buf) { strcpy(m_data, buf); }
