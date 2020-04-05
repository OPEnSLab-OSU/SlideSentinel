#include "COMController.h"
#include "Console.h"

COMController::COMController(Freewave &radio, MAX3243 &max3243,
                             SN74LVC2G53 &mux, HardwareSerial &serial,
                             uint32_t baud, uint8_t clientId, uint8_t serverId,
                             uint16_t timeout, uint8_t retries)
    : m_interface(serial, baud, clientId, serverId, timeout, retries, false),
      m_radio(radio), m_max3243(max3243), m_mux(mux), m_serial(serial),
      m_timer(0), m_dropped_pkts(0), m_threshold(4) {}

bool COMController::init() {
  m_interface.init();
  m_max3243.disable();
  m_mux.comY1();
  console.debug("COMController initialized.\n");
  return true;
}

void COMController::m_clearBuffer() {
  memset(m_buf, '\0', sizeof(char) * MAX_DATA_LEN);
}

bool COMController::request(SSModel &model) {
  m_mux.comY1();
  if (m_radio.getZ9C())
    m_max3243.enable();

  if (!m_interface.sendPacket(REQ, model.toDiag())) {
    console.debug("Failed to make request.\n");
    m_droppedPkt();
    m_max3243.disable();
    model.setError(ACK_ERR);
    return false;
  }

  m_clearBuffer();
  if (!m_interface.receivePacket(m_buf)) {
    m_droppedPkt();
    m_max3243.disable();
    model.setError(REPLY_ERR);
    return false;
  }

  console.debug("successfully received config, RADIO ----> GNSS.\n");
  m_mux.comY2();
  console.debug("Received packet: ");
  console.debug(m_buf);
  console.debug("\n");
  model.handleRes(m_buf);
  return true;
}

bool COMController::upload(SSModel &model) {
  m_clearBuffer();
  console.debug("RADIO ----> FEATHER.\n");
  m_mux.comY1();

  if (!m_interface.sendPacket(UPL, model.toData(m_threshold))) {
    console.debug("\nFailed to upload packet.\n");
    m_droppedPkt();
    model.setError(ACK_ERR);
    return false;
  }

  if (m_radio.getZ9C())
    m_max3243.disable();

  console.debug("\nSuccesfully uploaded\n");
  return true;
}

void COMController::resetRadio() { m_radio.reset(); }

bool COMController::channelBusy(SSModel &model) {
  m_interface.clear();
  m_timer.startTimer(5);
  while (!m_timer.timerDone()) {
    if (m_serial.available()) {
      model.setError(CHNNL_ERR);
      return true;
    }
  }
  return false;
}

void COMController::m_setTimeout(uint16_t timeout) {
  m_interface.setTimeout(timeout);
}

void COMController::m_setRetries(uint16_t retries) {
  m_interface.setRetries(retries);
}

void COMController::m_droppedPkt() { m_dropped_pkts++; }

void COMController::status(SSModel &model) {
  model.setProp(TIMEOUT, m_interface.getTimeout());
  model.setProp(RETRIES, m_interface.getRetries());
  model.setProp(THRESHOLD, m_threshold);
  model.setDroppedPkts(m_dropped_pkts);
}

void COMController::update(SSModel &model) {
  if (model.validProp(TIMEOUT))
    m_setTimeout(model.getProp(TIMEOUT));
  if (model.validProp(RETRIES))
    m_setRetries(model.getProp(RETRIES));
}
