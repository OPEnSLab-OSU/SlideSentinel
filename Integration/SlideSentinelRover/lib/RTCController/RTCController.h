#ifndef _RTCCONTROLLER_H_
#define _RTCCONTROLLER_H_
#define MAX_TIMESTAMP_LEN 15

#include "Controller.h"
#include "RTClibExtended.h"
#include <Arduino.h>
#include <Wire.h>

class RTCController : public Controller {
private:
  RTC_DS3231 &m_RTC;
  static uint8_t m_pin;
  int m_wakeTime;  // state
  int m_sleepTime; // state
  static volatile bool m_flag;
  DateTime m_date;
  char m_timestamp[MAX_TIMESTAMP_LEN];
  int m_backoffCounter;

  void m_setDate();
  void m_clearAlarm();
  void m_setFlag();
  bool m_getFlag();
  void m_setAlarm(int time);
  void m_setWakeTime(int wakeTime);
  void m_setSleepTime(int sleepTime);

public:
  RTCController(RTC_DS3231 &RTC_DS, uint8_t pin, uint16_t wakeTime,
                uint16_t sleepTime);
  void setPollAlarm();
  void setWakeAlarm();
  bool alarmDone();
  static void RTC_ISR();
  char *getTimestamp();
  void incrementBackoff();
  bool init();
  void status(SSModel &model);
  void update(SSModel &model);
};

#endif // _RTCCONTROLLER_H_

// 20th lathe turning wood turing