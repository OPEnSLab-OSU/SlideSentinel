#include "IMUController.h"

#define CTRL_REG3 0b11000000
#define CTRL_REG4 0b00100001
#define CTRL_REG5 0b00100000
#define REG_TRANS_CFG 0b00001110
#define REG_TRANS_CT 0b00000000

IMUController::IMUController(uint8_t pin, uint8_t sensitivity)
    : Controller("IMU"), m_pin(pin), m_sensitivity(sensitivity) {
  digitalWrite(m_pin, INPUT_PULLUP);
  m_flag = false;
}

bool IMUController::init() {
  // interesting way of wrapping the ISR in the class
  instance = this;

  if (!m_accelerometer.begin())
    return false;

  m_accelerometer.setRange(MMA8451_RANGE_2_G);
  m_accelerometer.setDataRate(MMA8451_DATARATE_6_25HZ);
  m_accelerometer.writeRegister8_public(MMA8451_REG_CTRL_REG3, CTRL_REG3);
  m_accelerometer.writeRegister8_public(MMA8451_REG_CTRL_REG4, CTRL_REG4);
  m_accelerometer.writeRegister8_public(MMA8451_REG_CTRL_REG5, CTRL_REG5);
  m_accelerometer.writeRegister8_public(MMA8451_REG_TRANSIENT_CFG,
                                        REG_TRANS_CFG);
  m_accelerometer.writeRegister8_public(MMA8451_REG_TRANSIENT_THS,
                                        m_sensitivity);
  m_accelerometer.writeRegister8_public(MMA8451_REG_TRANSIENT_CT, REG_TRANS_CT);
  attachInterrupt(digitalPinToInterrupt(m_pin), imu_ISR, FALLING);
  return true;
}

void IMUController::imu_ISR() { instance->m_ISR(); }

void IMUController::m_ISR() {
  detachInterrupt(digitalPinToInterrupt(m_pin));
  m_flag = true;
  attachInterrupt(digitalPinToInterrupt(m_pin), imu_ISR, CHANGE);
}

bool IMUController::getFlag() { return m_flag; }

void IMUController::update(JsonDocument &doc) {}

void IMUController::status(uint8_t verbosity, JsonDocument &doc) {}