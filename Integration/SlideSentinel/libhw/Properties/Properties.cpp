#include "Properties.h"
#include <Plog.h>

Properties::Properties() {}

int Properties::get(int prop) { return m_prop[prop]; }

void Properties::set(int prop, int val) {
  if (val != -1)
    m_prop[prop] = val;
}

void Properties::init() {
  for (int i = 0; i < NUM_PROP; i++)
    m_prop[i] = INVALID_PROP;
}

void Properties::write(JsonDocument &doc) {
  JsonArray data = doc.createNestedArray(SS_PROP);
  data.add(get(TIMEOUT));
  data.add(get(RETRIES));
  data.add(get(WAKE_TIME));
  data.add(get(SLEEP_TIME));
  data.add(get(SENSITIVITY));
  data.add(get(LOG_FREQ));
  data.add(get(THRESHOLD));
}

void Properties::read(char *buf) {
  StaticJsonDocument<MAX_PROP_LEN> json;
  auto err = deserializeJson(json, buf);
  if (err)
    return;
  set(TIMEOUT, json[SS_PROP][TIMEOUT] != INVALID_PROP ? json[SS_PROP][TIMEOUT] : get(TIMEOUT));
  set(RETRIES, json[SS_PROP][RETRIES] != INVALID_PROP ? json[SS_PROP][RETRIES] : get(RETRIES));
  set(WAKE_TIME, json[SS_PROP][WAKE_TIME] != INVALID_PROP ? json[SS_PROP][WAKE_TIME] : get(WAKE_TIME));
  set(SLEEP_TIME, json[SS_PROP][SLEEP_TIME] != INVALID_PROP ? json[SS_PROP][SLEEP_TIME] : get(SLEEP_TIME));
  set(SENSITIVITY, json[SS_PROP][SENSITIVITY] != INVALID_PROP ? json[SS_PROP][SENSITIVITY] : get(SENSITIVITY));
  set(LOG_FREQ, json[SS_PROP][LOG_FREQ] != INVALID_PROP ? json[SS_PROP][LOG_FREQ] : get(LOG_FREQ));
  set(THRESHOLD, json[SS_PROP][THRESHOLD] != INVALID_PROP ? json[SS_PROP][THRESHOLD] : get(THRESHOLD));
  print();
}

void Properties::print() {
  LOGD << "******* PROPERTIES ********";
  LOGD << "\tTimeout: " <<  get(TIMEOUT);
  LOGD << "\tRetries: " << get(RETRIES);
  LOGD << "\tWake Time: " << get(WAKE_TIME);
  LOGD << "\tSleep Time: " << get(SLEEP_TIME);
  LOGD << "\tSensitivity: " << get(SENSITIVITY);
  LOGD << "\tLog Frequency: " << get(LOG_FREQ);
  LOGD << "\tThreshold: " << get(THRESHOLD);
  LOGD << "**************************";
}