#ifndef _RTCCONTROLLER_H_
#define _RTCCONTROLLER_H_
#define MAX_TIMESTAMP_LEN 15

#include <Arduino.h>
#include <Wire.h>
#include "ArduinoJson.h"
#include "Controller.h"
#include "RTClibExtended.h"

// State waketime, sleeptime
class RTCController : public Controller {
private:
  RTC_DS3231 &m_RTC;
  static uint8_t m_pin;
  static volatile bool m_flag;
  DateTime m_date;
  char m_timestamp[MAX_TIMESTAMP_LEN];

  void m_setDate();
  void m_clearAlarm();
  void m_setFlag();
  bool m_getFlag();
  void m_setAlarm(int time);

public:
  RTCController(Prop &prop, RTC_DS3231 &RTC_DS, uint8_t pin);
  void setPollAlarm();
  void setWakeAlarm();
  bool alarmDone();
  static void RTC_ISR();
  char *getTimestamp();
  bool init();
  void status(uint8_t verbosity, JsonDocument &doc);
};

#endif // _RTCCONTROLLER_H_