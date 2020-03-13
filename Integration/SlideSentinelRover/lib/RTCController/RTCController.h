#ifndef _RTCCONTROLLER_H_
#define _RTCCONTROLLER_H_
#define MAX_TIMESTAMP_LEN 15

#include <Arduino.h>
#include <Wire.h>
#include "Controller.h"
#include "RTClibExtended.h"

class RTCController : public Controller {
private:
  RTC_DS3231 &m_RTC;
  static uint8_t m_pin;
  uint16_t m_wakeTime;  // state
  uint16_t m_sleepTime; // state
  static volatile bool m_flag;
  DateTime m_date;
  char m_timestamp[MAX_TIMESTAMP_LEN];

  void m_setDate();
  void m_clearAlarm();
  void m_setFlag();
  bool m_getFlag();
  void m_setAlarm(int time);

public:
  RTCController(RTC_DS3231 &RTC_DS, uint8_t pin, uint16_t wakeTime, uint16_t sleepTime);
  void setPollAlarm();
  void setWakeAlarm();
  bool alarmDone();
  static void RTC_ISR();
  char *getTimestamp();
  bool init();
  void status(SSModel &model);
  void update(SSModel &model);
  void m_setWakeTime(uint16_t wakeTime);
  void m_setSleepTime(uint16_t sleepTime);
};

#endif // _RTCCONTROLLER_H_

// 20th lathe turning wood turing