#ifndef _RTCCONTROLLER_H_
#define _RTCCONTROLLER_H_
#define MAX_TIMESTAMP_LEN 15

#include <Arduino.h>
#include <Wire.h>
#include "ArduinoJson.h"
#include "Controller.h"
#include "RTClibExtended.h"

class RTCController : public Controller {
private:
  RTC_DS3231 *m_RTC;
  static uint8_t m_pin;
  static volatile bool m_flag;
  int m_wakeTime;  // in minutes
  int m_sleepTime; // in minutes
  DateTime m_date;
  char m_timestamp[MAX_TIMESTAMP_LEN];

  void m_setDate();
  void m_clearAlarm();
  void m_setFlag();
  bool m_getFlag();
  void m_setAlarm(int time);

public:
  RTCController(RTC_DS3231 *RTC_DS, uint8_t pin, uint8_t wakeTime,
                uint8_t sleepTime);
  void setPollAlarm();
  void setWakeAlarm();
  bool alarmDone();
  static void RTC_ISR();
  char *getTimestamp();
  bool init();
  void update(JsonDocument &doc);
  void status(uint8_t verbosity, JsonDocument &doc);
};

#endif // _RTCCONTROLLER_H_