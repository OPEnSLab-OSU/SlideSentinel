#include "Timer.h"

Timer::Timer(int timeoutBuffer) : m_timeoutBuffer(timeoutBuffer){};

void Timer::startTimer(int wakeTime) {
  m_timeout = (wakeTime + m_timeoutBuffer) * 1000;
  m_start = millis();
  m_armed = true;
}

void Timer::stopTimer() { m_armed = false; }

bool Timer::timerDone() {
  if (!(m_armed && ((millis() - m_start) > m_timeout)))
    return false;
  m_armed = false;
  return true;
}

void Timer::startStopwatch() {
  if (!m_armed)
    m_start = millis();
}

unsigned long Timer::stopwatch() {
  return m_armed ? 0 : (millis() - m_start) / 60000;
}