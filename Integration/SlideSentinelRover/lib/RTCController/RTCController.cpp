#include "RTCController.h"
#include "Console.h"

uint8_t RTCController::m_pin;
volatile bool RTCController::m_flag = false;

RTCController::RTCController(RTC_DS3231 &RTC_DS, uint8_t pin, uint16_t wakeTime,
                             uint16_t sleepTime)
    : m_RTC(RTC_DS), m_wakeTime(wakeTime), m_sleepTime(sleepTime),
      m_backoffCounter(1) {
  m_pin = pin;
  // Enable sprintf function on SAMD21
  asm(".global _printf_float");
}

void RTCController::m_clearAlarm() {
  m_RTC.armAlarm(1, false);
  m_RTC.clearAlarm(1);
  m_RTC.alarmInterrupt(1, false);
  m_RTC.armAlarm(2, false);
  m_RTC.clearAlarm(2);
  m_RTC.alarmInterrupt(2, false);
}

void RTCController::RTC_ISR() {
  detachInterrupt(digitalPinToInterrupt(m_pin));
  m_flag = true;
}

void RTCController::m_setFlag() { m_flag = false; }

bool RTCController::m_getFlag() { return m_flag; }

bool RTCController::init() {
  pinMode(m_pin, INPUT_PULLUP);
  Wire.begin();
  if (!m_RTC.begin())
    return false;
  m_RTC.adjust(DateTime(__DATE__, __TIME__));

  // clear any pending alarms
  m_clearAlarm();

  // Set SQW pin to OFF (in my case it was set by default to 1Hz)
  // The output of the DS3231 INT pin is connected to this pin
  // It must be connected to arduino Interrupt pin for wake-up
  m_RTC.writeSqwPinMode(DS3231_OFF);
  console.debug("RTCController initialized.\n");
  return true;
}

// TODO Add second level precision
void RTCController::m_setAlarm(int time) {
  m_clearAlarm();
  m_setDate();
  uint8_t min = (m_date.minute() + time) % 60;
  uint8_t hr = (m_date.hour() + ((m_date.minute() + time) / 60)) % 24;
  m_RTC.setAlarm(ALM1_MATCH_HOURS, min, hr, 0);
  m_RTC.alarmInterrupt(1, true);
  attachInterrupt(digitalPinToInterrupt(m_pin), RTC_ISR, FALLING);

  console.debug("Setting alarm for ");
  console.debug(hr);
  console.debug(":");
  console.debug(min);
  console.debug("\n");
}

char *RTCController::getTimestamp() {
  memset(m_timestamp, '\0', sizeof(char) * MAX_TIMESTAMP_LEN);
  m_setDate();
  sprintf(m_timestamp, "%.2d.%.2d.%.2d.%.2d.%.2d", m_date.month(), m_date.day(),
          m_date.hour(), m_date.minute(), m_date.second());
  return m_timestamp;
}

void RTCController::setPollAlarm() {
  // reset the backoff counter if no collision occured
  m_backoffCounter = 1;
  m_setAlarm(m_wakeTime);
}

void RTCController::setWakeAlarm() {
  m_setAlarm(m_sleepTime * m_backoffCounter);
}

// need access to unconnected analog in for seeding 
// random number generator
void RTCController::incrementBackoff() {
  if (m_backoffCounter < 3)
    m_backoffCounter++;
}

bool RTCController::alarmDone() {
  if (!m_getFlag())
    return false;
  m_clearAlarm();
  m_setFlag();
  return true;
}

void RTCController::m_setDate() { m_date = m_RTC.now(); }

void RTCController::m_setWakeTime(int wakeTime) { m_wakeTime = wakeTime; }

void RTCController::m_setSleepTime(int sleepTime) { m_sleepTime = sleepTime; }

void RTCController::status(SSModel &model) {
  model.setProp(WAKE_TIME, m_wakeTime);
  model.setProp(SLEEP_TIME, m_sleepTime);
}

void RTCController::update(SSModel &model) {
  if (model.validProp(WAKE_TIME))
    m_setWakeTime(model.getProp(WAKE_TIME));
  if (model.validProp(SLEEP_TIME))
    m_setSleepTime(model.getProp(SLEEP_TIME));
}
