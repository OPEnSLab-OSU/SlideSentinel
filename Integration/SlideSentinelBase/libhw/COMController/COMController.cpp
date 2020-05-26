#include "COMController.h"
#include <Plog.h>

// TODO add error logging
// TODO make timer buffer external in config file for the base station
// TODO clean state of ComController on Timeout

#define TIMEOUT_BUF 300 // 5 minutes

COMController::COMController(Freewave &radio, SN74LVC2G53 &mux,
                             HardwareSerial &serial, uint32_t baud,
                             uint8_t clientId, uint8_t serverId,
                             uint16_t timeout, uint8_t retries)
    : m_interface(serial, baud, clientId, serverId, timeout, retries, true),
      m_radio(radio), m_mux(mux), m_timer(TIMEOUT_BUF), m_isServing(false),
      m_num_uploads(0), m_num_requests(0) {}

bool COMController::init() {
  m_interface.init();
  m_reset();
  LOGD << "COMController initialized";
  m_timer.startStopwatch();
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

int COMController::listen(BaseModel &model) {

  // check the timer has expired
  if (timeout()) {
    LOGD << "Serving " << model.getRoverServe() << ": Timeout occured terminating service.";
    m_isServing = false;
    m_timer.startStopwatch();
    m_reset();
  }

  if (!m_available())
    return false;

  m_reset();
  if (!m_interface.receivePacket(m_buf))
    return false;

  int rover_id = m_interface.getId();
  int type = m_interface.getType();

  LOGD << "********** Message from Rover " << rover_id << ": TYPE: " << type << " *************";
  model.setRoverAlert(rover_id);

  if (type == UPL)
    return m_upload(rover_id, model);
  else if (type == REQ)
    return m_request(rover_id, model);
}

bool COMController::m_request(int rover_id, BaseModel &model) {
  m_num_requests++;
  model.setDiag(rover_id, m_buf);

  // accept diagnostic info regardless of the base's servicing state
  LOGD << "Request Received, Diagnostics: " << model.getDiag(rover_id);
  LOGD << "Props: " << model.getProps(rover_id);

  // receive a request, if we are already servicing another rover
  // flix mux back to servicing state, do not reply so rover will go back to
  // sleep
  if (m_isServing) {
    LOGD << "Received REQ while already servicing another rover";
    m_mux.comY1();
    return true;
  }
  LOGD << "Received REQ, NOT servicing another rover, sending back Props...";
  if (!m_interface.sendPacket(RES, model.getProps(rover_id))) {
    LOGW << "FAILED TO REPLY WITH PROPS!";
    return false;
  }

  model.setRoverServe(rover_id);
  LOGD << "Setting alarm for: " << model.getRoverWakeTime(rover_id);
  m_timer.startTimer(model.getRoverWakeTime(rover_id) * 60);

  m_isServing = true;
  m_mux.comY1();
  LOGD << "Props reply complete, beginning to service";
  return true;
}

bool COMController::m_upload(int rover_id, BaseModel &model) {
  m_num_uploads++;
  LOGD << "Received Upload: " << m_buf;
  model.setData(rover_id, m_buf);

  // if we are serving, check that the upload is from the rover being serviced
  if (m_isServing && (model.getRoverServe() != rover_id)) {
    LOGD << "Received UPL while servicing a different rover";
    m_mux.comY1();
    return true;
  }

  LOGD << "Received UPL from rover being serviced";
  m_isServing = false;
  m_reset();
  m_timer.stopTimer();
  m_timer.startStopwatch();
}

void COMController::resetRadio() { m_radio.reset(); }

void COMController::m_setTimeout(uint16_t timeout) {
  m_interface.setTimeout(timeout);
}

void COMController::m_setRetries(uint16_t retries) {
  m_interface.setRetries(retries);
}

bool COMController::timeout() { return m_timer.timerDone(); }

void COMController::status(BaseModel &model) {
  model.setNumRequests(m_num_requests);
  model.setNumUploads(m_num_uploads);
  model.setStopwatch(m_timer.stopwatch());
}

void COMController::update(BaseModel &model) {}
