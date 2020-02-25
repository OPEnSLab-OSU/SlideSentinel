#ifndef _STATE_H_
#define _STATE_H_

#include <Arduino.h>
#include "ArduinoJson.h"

class State {
public:
  State(uint16_t timeout, uint8_t retries, uint32_t wakeTime,
        uint32_t sleepTime, uint8_t sensitivity, uint32_t logFreq);

  // ComController
  uint16_t timeout;
  uint8_t retries;

  // RTCController
  uint32_t wakeTime;
  uint32_t sleepTime;

  // IMUController
  uint8_t sensitivity;

  // GNSSController
  uint32_t logFreq;

  void update(JsonDocument &doc);
};

#endif // _STATE_H_