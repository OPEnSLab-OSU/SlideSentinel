#include "COMController.h"
#include "Console.h"
#include "FeatherTrace.h"

COMController::COMController(Freewave &radio, MAX3243 &max3243,
                             SN74LVC2G53 &mux, HardwareSerial &serial,
                             uint32_t baud, uint8_t clientId, uint8_t serverId,
                             uint16_t timeout, uint8_t retries)
    : m_interface(serial, baud, clientId, serverId, timeout, retries, false),
      m_radio(radio), m_max3243(max3243), m_mux(mux), m_serial(serial),
      m_timer(0), m_dropped_pkts(0), m_threshold(0) {}

bool COMController::init() { MARK;
  m_interface.init();
  m_max3243.disable();
  m_mux.comY1();
  console.debug("COMController initialized.\n"); MARK;
  return true;
}

void COMController::m_clearBuffer() {
  memset(m_buf, '\0', sizeof(char) * MAX_DATA_LEN);
}

bool COMController::request(SSModel &model) { MARK;
  m_mux.comY1();
  if (m_radio.getZ9C())
    m_max3243.enable();

  // while(1){
  //   m_interface.sendPacket(REQ, model.toDiag());
  // }
  if (!m_interface.sendPacket(REQ, model.toDiag())) { MARK;
    console.debug("Failed to make request.\n");
    m_droppedPkt();
    m_max3243.disable();
    model.setError(ACK_ERR);
    return false;
  }
  MARK;

  // NOTE case: the base is servicing another rover. The base will take the
  // diagnostic data because it could be a wake alert, but will not reply if it
  // is busy servicing another rover

  // // FIXME VERY IMPORTANT! This needs to be changed, the rover should get 
  // the props stored in the base back but should not poll.
  // This should terminate because their could be new data sent
  // that needs to hit the rover on its next wake cycle
  m_clearBuffer();
  MARK;
  if (!m_interface.receivePacket(m_buf)) { MARK;
    m_droppedPkt();
    m_max3243.disable();
    model.setError(REPLY_ERR);
    return false;
  }
  MARK;

  console.debug("\nsuccessfully received config, RADIO ----> GNSS.\n");
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

  console.debug("Data from wake cycle: \n");
  console.debug(model.toData(m_threshold));
  MARK;
  if (!m_interface.sendPacket(UPL, model.toData(m_threshold))) { MARK;
    console.debug("\nFailed to upload packet.\n");
    m_droppedPkt();
    model.setError(ACK_ERR);
    return false;
  }
  MARK;

  if (m_radio.getZ9C())
    m_max3243.disable();

  console.debug("\nSuccesfully uploaded\n");
  return true;
}

void COMController::resetRadio() { MARK; m_radio.reset(); }

bool COMController::channelBusy(SSModel &model) {
  m_interface.clearSerial();
  m_timer.startTimer(5);
  MARK;
  while (!m_timer.timerDone()) { MARK;
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

void COMController::m_setThreshold(uint8_t threshold) {
  m_threshold = threshold;
}

void COMController::m_droppedPkt() { m_dropped_pkts++; }

void COMController::status(SSModel &model) {
  model.setProp(TIMEOUT, m_interface.getTimeout());
  model.setProp(RETRIES, m_interface.getRetries());
  model.setProp(THRESHOLD, m_threshold);
  model.setDroppedPkts(m_dropped_pkts);
}

void COMController::update(SSModel &model) {
  m_setTimeout(model.getProp(TIMEOUT));
  m_setRetries(model.getProp(RETRIES));
  m_setThreshold(model.getProp(THRESHOLD));
}
