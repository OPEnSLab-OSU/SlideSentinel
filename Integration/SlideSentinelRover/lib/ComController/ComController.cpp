#include "COMController.h"
#include "Console.h"

COMController::COMController(Freewave &radio, MAX3243 &max3243,
                             SN74LVC2G53 &mux, HardwareSerial &serial,
                             uint32_t baud, uint8_t clientId, uint8_t serverId,
                             uint16_t timeout, uint8_t retries)
    : m_interface(serial, baud, clientId, serverId, timeout, retries, false),
      m_radio(radio), m_max3243(max3243), m_mux(mux), m_dropped_pkts(0),
      m_threshold(0) {}

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
  model.handleRes(m_buf);
  return true;
}

bool COMController::upload(SSModel &model) {
  m_clearBuffer();
  console.debug("RADIO ----> FEATHER.\n");
  m_mux.comY1();

  if (!m_interface.sendPacket(UPL, model.toData(m_threshold))) {
    console.debug("Failed to upload packet.\n");
    m_droppedPkt();
    model.setError(ACK_ERR);
    return false;
  }

  if (m_radio.getZ9C())
    m_max3243.disable();

  console.debug("Succesfully uploaded");
  return true;
}

void COMController::resetRadio() { m_radio.reset(); }

bool COMController::channelBusy() { return m_radio.channel_busy(); }

void COMController::m_setTimeout(uint16_t timeout) {
  m_interface.setTimeout(timeout);
}

void COMController::m_setRetries(uint16_t retries) {
  m_interface.setRetries(retries);
}

void COMController::m_droppedPkt() { m_dropped_pkts++; }

void COMController::status(SSModel &model) {
  model.setTimeout(m_interface.getTimeout());
  model.setRetries(m_interface.getRetries());
  model.setDropped_pkts(m_dropped_pkts);
}

void COMController::update(SSModel &model) {
  if (model.valid(model.timeout()))
    m_setTimeout(model.timeout());
  if (model.valid(model.retries()))
    m_setTimeout(model.retries());
}
