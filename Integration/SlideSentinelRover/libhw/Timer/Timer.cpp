#include "Timer.h"

Timer::Timer(int timeoutBuffer) : m_timeoutBuffer(timeoutBuffer){};

void Timer::startTimer(int wakeTime) {
  m_timeout = (wakeTime + m_timeoutBuffer) * 1000;
  m_start = millis();
  m_armed = true;
}

bool Timer::timerDone() {
  if (!(m_armed && ((millis() - m_start) > m_timeout)))
    return false;
  m_armed = false;
  return true;
}