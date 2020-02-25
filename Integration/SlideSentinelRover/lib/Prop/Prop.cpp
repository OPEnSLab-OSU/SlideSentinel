#include "Prop.h"

Prop::Prop(uint16_t timeout, uint8_t retries, uint32_t wakeTime,
             uint32_t sleepTime, uint8_t sensitivity, uint32_t logFreq)
    : timeout(timeout), retries(retries), wakeTime(wakeTime),
      sleepTime(sleepTime), sensitivity(sensitivity), logFreq(logFreq) {}

void Prop::update(JsonDocument &doc) {}