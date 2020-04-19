#include "Properties.h"
#include "Console.h"

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
  set(TIMEOUT, json[SS_PROP][TIMEOUT]);
  set(RETRIES, json[SS_PROP][RETRIES]);
  set(WAKE_TIME, json[SS_PROP][WAKE_TIME]);
  set(SLEEP_TIME, json[SS_PROP][SLEEP_TIME]);
  set(SENSITIVITY, json[SS_PROP][SENSITIVITY]);
  set(LOG_FREQ, json[SS_PROP][LOG_FREQ]);
  set(THRESHOLD, json[SS_PROP][THRESHOLD]);
  print();
}

void Properties::print() {
  console.debug("******* PROPERTIES ********");
  console.debug("\nTimeout: ");
  console.debug(get(TIMEOUT));
  console.debug("\nRetries: ");
  console.debug(get(RETRIES));
  console.debug("\nWake Time: ");
  console.debug(get(WAKE_TIME));
  console.debug("\nSleep Time: ");
  console.debug(get(SLEEP_TIME));
  console.debug("\nSensitivity: ");
  console.debug(get(SENSITIVITY));
  console.debug("\nLog Frequency: ");
  console.debug(get(LOG_FREQ));
  console.debug("\nThreshold: ");
  console.debug(get(THRESHOLD));
  console.debug("\n**************************\n");
}