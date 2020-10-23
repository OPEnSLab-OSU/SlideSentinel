#include "Timer.h"

Timer::Timer(int timeoutBuffer) : m_timeoutBuffer(timeoutBuffer){};

Timer::Timer() : m_armed(false){};


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
  if (!m_armed){
    m_start = millis();
    Serial.print("Started Convergence Timer for RTK Fix: ");
    Serial.println(m_start);
    m_armed = true;
  }
}

float Timer::stopwatch() {
  float time = float(millis() - m_start) / 60000;
  Serial.println(time);
  return time;
}