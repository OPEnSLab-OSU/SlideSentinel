#ifndef _IMUCONTROLLER_H_
#define _IMUCONTROLLER_H_

#include <Arduino.h>
#include "Adafruit_MMA8451.h"
#include "Controller.h"

class IMUController : public Controller {
private:
  Adafruit_MMA8451 m_dev;
  static uint8_t m_pin;
  static volatile bool m_flag;
  uint8_t m_sensitivity; // state
  bool m_getFlag();
  void m_setFlag();
  void m_setSensitivity(uint8_t sensitivity);

public:
  IMUController(uint8_t pin, uint8_t sensitivity);
  bool init();
  bool getWakeStatus();
  static void IMU_ISR();
  void status(SSModel &model);
  void update(SSModel &model);
};

#endif // _IMUCONTROLLER_H_