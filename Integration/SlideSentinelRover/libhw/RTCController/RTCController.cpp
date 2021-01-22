#include "RTCController.h"
#include "Console.h"
#include "FeatherTrace.h"

uint8_t RTCController::m_pin;

RTCController::RTCController(RTC_DS3231 &RTC_DS, uint8_t pin, uint16_t wakeTime,
                             uint16_t sleepTime)
    : m_RTC(RTC_DS), m_wakeTime(wakeTime), m_sleepTime(sleepTime),
      m_timer(0) {
  m_pin = pin;
  // Enable sprintf function on SAMD21
  asm(".global _printf_float");
}

void RTCController::m_clearAlarm() { MARK;
  m_RTC.armAlarm(1, false);
  m_RTC.clearAlarm(1);
  m_RTC.alarmInterrupt(1, false);
  m_RTC.armAlarm(2, false);
  m_RTC.clearAlarm(2);
  m_RTC.alarmInterrupt(2, false);
}

void RTCController::RTC_ISR() {
  detachInterrupt(digitalPinToInterrupt(m_pin));
}


bool RTCController::init() { MARK;
  pinMode(m_pin, INPUT_PULLUP);
  Wire.begin();
  if (!m_RTC.begin()) {
      console.debug("Could not find RTC!");
      return false;
  }
  MARK;
  // the RTC will start at zero after a power off
  // get the current time, and synchronize it if it's before the compile time
  // FIXME: This synchronizes to PST/PDT, not UTC like the base
  const DateTime synctime = DateTime(__DATE__, __TIME__);
  m_setDate();
  if (m_date.unixtime() < synctime.unixtime()) {
      console.debug("Synchronizing to:");
      console.debug((int)synctime.unixtime());
      m_RTC.adjust(synctime);
  }

  // clear any pending alarms
  m_clearAlarm();

  // Set SQW pin to OFF (in my case it was set by default to 1Hz)
  // The output of the DS3231 INT pin is connected to this pin
  // It must be connected to arduino Interrupt pin for wake-up
  m_RTC.writeSqwPinMode(DS3231_OFF);
  console.debug("RTCController initialized.\n");
  return true;
}

void RTCController::m_setAlarm(int time) { MARK;
  m_clearAlarm();
  m_setDate();
  uint8_t min = (m_date.minute() + time) % 60;
  uint8_t hr = (m_date.hour() + ((m_date.minute() + time) / 60)) % 24;
  m_RTC.setAlarm(ALM1_MATCH_HOURS, min, hr, 0);
  m_RTC.alarmInterrupt(1, true);
  attachInterrupt(digitalPinToInterrupt(m_pin), RTC_ISR, FALLING);

  console.debug("\nCurrent Time: ");
  console.debug(m_date.hour());
  console.debug(":");
  console.debug(m_date.minute());
  console.debug("\n");

  console.debug("Setting alarm for ");
  console.debug(hr);
  console.debug(":");
  console.debug(min);
  console.debug("\n");
}

char *RTCController::getTimestamp() { MARK;
  memset(m_timestamp, '\0', sizeof(char) * MAX_TIMESTAMP_LEN);
  m_setDate();
  sprintf(m_timestamp, "%.2d.%.2d.%.2d.%.2d.%.2d", m_date.month(), m_date.day(),
          m_date.hour(), m_date.minute(), m_date.second());
  return m_timestamp;
}

void RTCController::setPollAlarm() { MARK;
  // reset the backoff counter if no collision occured
  m_timer.startTimer(m_wakeTime*60);
}

void RTCController::setWakeAlarm() { MARK;
  m_setAlarm(m_sleepTime);
}


bool RTCController::alarmDone() { MARK;
  return m_timer.timerDone();
}

void RTCController::m_setDate() { MARK; m_date = m_RTC.now(); }

void RTCController::m_setWakeTime(int wakeTime) { m_wakeTime = wakeTime; }

void RTCController::m_setSleepTime(int sleepTime) { m_sleepTime = sleepTime; }

void RTCController::status(SSModel &model) {
  model.setProp(WAKE_TIME, m_wakeTime);
  model.setProp(SLEEP_TIME, m_sleepTime);
}

void RTCController::update(SSModel &model) { MARK;
  m_setWakeTime(model.getProp(WAKE_TIME));
  m_setSleepTime(model.getProp(SLEEP_TIME));
}
