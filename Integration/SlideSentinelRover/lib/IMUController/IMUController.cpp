#include "IMUController.h"

#define CTRL_REG3 0b11000000
#define CTRL_REG4 0b00100001
#define CTRL_REG5 0b00100000
#define REG_TRANS_CFG 0b00001110
#define REG_TRANS_CT 0b00000000

uint8_t IMUController::m_pin;
volatile bool IMUController::m_flag = false;

IMUController::IMUController(uint8_t pin, uint8_t sensitivity)
    : Controller("IMU"), m_sensitivity(sensitivity) {
  m_pin = pin;
  digitalWrite(m_pin, INPUT_PULLUP);
}

void IMUController::IMU_ISR() {
  detachInterrupt(digitalPinToInterrupt(m_pin));
  m_flag = true;
  attachInterrupt(digitalPinToInterrupt(m_pin), IMU_ISR, FALLING);
}

bool IMUController::init() {
  if (!m_dev.begin()) // calls Wire.begin()
    return false;

  m_dev.setRange(MMA8451_RANGE_2_G);
  m_dev.setDataRate(MMA8451_DATARATE_6_25HZ);
  m_dev.writeRegister8_public(MMA8451_REG_CTRL_REG3, CTRL_REG3);
  m_dev.writeRegister8_public(MMA8451_REG_CTRL_REG4, CTRL_REG4);
  m_dev.writeRegister8_public(MMA8451_REG_CTRL_REG5, CTRL_REG5);
  m_dev.writeRegister8_public(MMA8451_REG_TRANSIENT_CFG, REG_TRANS_CFG);
  m_dev.writeRegister8_public(MMA8451_REG_TRANSIENT_THS, m_sensitivity);
  m_dev.writeRegister8_public(MMA8451_REG_TRANSIENT_CT, REG_TRANS_CT);
  attachInterrupt(digitalPinToInterrupt(m_pin), IMU_ISR, FALLING);
  return true;
}

bool IMUController::getFlag() { return m_flag; }

void IMUController::setFlag() { m_flag = false; }

void IMUController::update(JsonDocument &doc) {}

void IMUController::status(uint8_t verbosity, JsonDocument &doc) {}