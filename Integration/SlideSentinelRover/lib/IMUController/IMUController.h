#ifndef _IMUCONTROLLER_H_
#define _IMUCONTROLLER_H_

#include <Arduino.h>
#include "Adafruit_MMA8451.h"
#include "ArduinoJson.h"
#include "Controller.h"

class IMUController : public Controller {

private:
  static IMUController *instance;
  Adafruit_MMA8451 m_accelerometer;
  uint8_t m_pin;
  uint8_t m_sensitivity;
  volatile bool m_flag;
  static void imu_ISR();
  void m_ISR();

public:
  IMUController(uint8_t pin, uint8_t sensitivity);
  bool init();
  bool getFlag();
  void update(JsonDocument &doc);
  void status(uint8_t verbosity, JsonDocument &doc);
};

#endif // _IMUCONTROLLER_H_