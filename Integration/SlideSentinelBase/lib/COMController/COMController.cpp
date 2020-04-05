#include "COMController.h"
#include "Console.h"

// TODO add error logging
// TODO make timer buffer external in config file for the base station
// TODO clean state of ComController on Timeout

#define TIMEOUT_BUF 300 // 5 minutes

COMController::COMController(Freewave &radio, SN74LVC2G53 &mux,
                             HardwareSerial &serial, uint32_t baud,
                             uint8_t clientId, uint8_t serverId,
                             uint16_t timeout, uint8_t retries)
    : m_interface(serial, baud, clientId, serverId, timeout, retries, true),
      m_radio(radio), m_mux(mux), m_timer(TIMEOUT_BUF) {}

bool COMController::init() {
  m_interface.init();
  m_reset();
  console.debug("COMController initialized.\n");
  return true;
}

void COMController::m_reset() {
  m_mux.comY2();
  m_clear();
}

void COMController::m_clear() {
  memset(m_buf, '\0', sizeof(char) * MAX_DATA_LEN);
}

bool COMController::m_available() { return m_interface.available(); }

bool COMController::listenUpl(BaseModel &model) {
  if (!m_available())
    return false;

  m_reset();

  if (!m_interface.receivePacket(m_buf))
    return false;

  int rover_id = m_interface.getId();
  int type = m_interface.getType();

  if (type != UPL)
    return false;

  model.setData(rover_id, m_buf);
  return true;
}

bool COMController::listenReq(BaseModel &model) {
  if (!m_available())
    return false;

  m_reset();

  if (!m_interface.receivePacket(m_buf))
    return false;

  int rover_id = m_interface.getId();
  int type = m_interface.getType();

  if (type != REQ)
    return false;

  model.setDiag(rover_id, m_buf);
  if (!m_interface.sendPacket(RES, model.getProps(rover_id)))
    return false;

  m_timer.startTimer(m_interface.getTimeout());
  m_mux.comY1();
  return true;
}

void COMController::resetRadio() { m_radio.reset(); }

void COMController::m_setTimeout(uint16_t timeout) {
  m_interface.setTimeout(timeout);
}

void COMController::m_setRetries(uint16_t retries) {
  m_interface.setRetries(retries);
}

bool COMController::timeout() { return m_timer.timerDone(); }

void COMController::status(BaseModel &model) {}

void COMController::update(BaseModel &model) {}
