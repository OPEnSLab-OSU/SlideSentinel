#ifndef _TIMER_H_
#define _TIMER_H_

#include <Arduino.h>

class Timer {
private:
  uint32_t m_timeout; // seconds
  int m_timeoutBuffer;
  uint32_t m_start;
  bool m_armed;

public:
  Timer();
  Timer(int timeoutBuffer);
  void startTimer(int wakeTime);
  void stopTimer();
  bool timerDone();
  void startStopwatch();
  float stopwatch();
};

#endif // _TIMER_H_