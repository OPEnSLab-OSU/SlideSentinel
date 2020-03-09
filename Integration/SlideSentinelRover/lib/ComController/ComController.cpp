#include "COMController.h"
#include "Console.h"

// COMController::COMController(Freewave &radio, MAX3243 &max3243,
//                              SN74LVC2G53 &mux, HardwareSerial &serial,
//                              uint32_t baud, uint8_t clientId, uint8_t
//                              serverId, uint16_t timeout, uint8_t retries)
//     : Controller("COM"), m_radio(radio), m_max3243(max3243), m_mux(mux),
//       m_serial(serial), m_baud(baud), m_clientId(clientId),
//       m_serverId(serverId), m_driver(m_serial), m_manager(m_driver,
//       m_clientId), m_timeout(timeout), m_retries(retries), m_dropped_pkts(0)
//       {}

COMController::COMController(Freewave &radio, MAX3243 &max3243,
                             SN74LVC2G53 &mux, HardwareSerial &serial,
                             uint32_t baud, uint8_t clientId, uint8_t serverId,
                             uint16_t timeout, uint8_t retries)
    : Controller("COM"),
      m_interface(serial, baud, clientId, serverId, timeout, retries, false),
      m_radio(radio), m_max3243(max3243), m_mux(mux), m_dropped_pkts(0) {}

// bool COMController::init() {
//   m_serial.begin(m_baud);
//   m_max3243.disable();
//   m_mux.comY1();
//   m_manager.init();
//   console.debug("COMController initialized.\n");
//   return true;
// }

bool COMController::init() {
  m_interface.init();
  m_max3243.disable();
  m_mux.comY1();
  console.debug("COMController initialized.\n");
  return true;
}

void COMController::m_clearBuffer() {
  memset(m_buf, '\0', sizeof(char) * RH_SERIAL_MAX_MESSAGE_LEN);
}

// bool COMController::m_send(char msg[]) {
//   uint8_t size = strlen(msg);
//   if (m_manager.sendtoWait((uint8_t *)msg, size, m_serverId))
//     return true;
//   else
//     return false;
// }

// bool COMController::m_receive(char buf[]) {
//   uint8_t len = RH_SERIAL_MAX_MESSAGE_LEN;
//   uint8_t from;
//   if (m_manager.recvfromAckTimeout((uint8_t *)buf, &len, m_timeout, &from))
//     return true;
//   else
//     return false;
// }

bool COMController::request(SSModel &model) {
  m_mux.comY1();
  if (m_radio.getZ9C())
    m_max3243.enable();

  if (!m_interface.sendPacket(REQ, model.toData())) {
    console.debug("FAILED!");
    m_droppedPkt();
    m_max3243.disable();
    model.setError(ACK_ERR);
    return false;
  }

  // if (!m_send(model.toReq())) {
  //   m_droppedPkt();
  //   m_max3243.disable();
  //   model.setError(ACK_ERR);
  //   return false;
  // }

  // m_clearBuffer();
  // if (!m_receive(m_buf)) {
  //   m_droppedPkt();
  //   m_max3243.disable();
  //   model.setError(REPLY_ERR);
  //   return false;
  // }

  console.debug("successfully received config, RADIO ----> GNSS");
  m_mux.comY2();
  model.handleRes(m_buf);
  return true;
}

bool COMController::upload(SSModel &model) {
  m_clearBuffer();
  console.debug("RADIO ----> FEATHER");
  m_mux.comY1();

  // if (!m_send(model.toUpl())) {
  //   m_droppedPkt();
  //   model.setError(ACK_ERR);
  //   return false;
  // }

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
