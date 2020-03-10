#include "IMUController.h"
#include "Console.h"

// Consult the datasheet for MMA8451 to change these values
#define CTRL_REG3 0b11000000
#define CTRL_REG4 0b00100001
#define CTRL_REG5 0b00100000
#define REG_TRANS_CFG 0b00001110
#define REG_TRANS_CT 0b00000000

uint8_t IMUController::m_pin;
volatile bool IMUController::m_flag = false;

IMUController::IMUController(uint8_t pin, uint8_t sensitivity)
    : m_sensitivity(sensitivity) {
  m_pin = pin;
}

// TODO continuously occuring interrupts, timestamping interrupts and
// increase logging interval reactively
void IMUController::IMU_ISR() {
  detachInterrupt(digitalPinToInterrupt(m_pin));
  m_flag = true;
  console.debug("IMU WAKE");
  attachInterrupt(digitalPinToInterrupt(m_pin), IMU_ISR, FALLING);
}

bool IMUController::init() {
  digitalWrite(m_pin, INPUT_PULLUP);
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
  console.debug("IMUController initialized.\n");
  return true;
}

bool IMUController::getWakeStatus() {
  if (!m_getFlag())
    return false;
  m_setFlag();
  return true;
}

bool IMUController::m_getFlag() { return m_flag; }

void IMUController::m_setFlag() { m_flag = false; }

void IMUController::m_setSensitivity(uint8_t sensitivity) {
  m_sensitivity = sensitivity;
  m_dev.writeRegister8_public(MMA8451_REG_TRANSIENT_THS, m_sensitivity);
}

void IMUController::status(SSModel &model) {
  model.setSensitivity(m_sensitivity);
  model.setIMUflag(getWakeStatus());
}

void IMUController::update(SSModel &model) {
  if (model.valid(model.sensitivity()))
    m_setSensitivity(model.sensitivity());
}
