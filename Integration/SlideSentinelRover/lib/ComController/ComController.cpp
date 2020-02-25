#include "COMController.h"
#include "Console.h"

COMController::COMController(State *state, Freewave *radio, MAX3243 *max3243,
                             SN74LVC2G53 *mux, HardwareSerial *serial,
                             uint32_t baud, uint8_t clientId, uint8_t serverId)
    : Controller("COM", state), m_radio(radio), m_max3243(max3243), m_mux(mux),
      m_serial(serial), m_baud(baud), m_clientId(clientId),
      m_serverId(serverId),
      m_ACK_ERR("{\"ID\":\"LOG\",\"MSG\":\"ERR: no ACK from server\"}"),
      m_REP_ERR("{\"ID\":\"LOG\",\"MSG\":\"ERR: no reply from server\"}") {
  m_driver = new RH_Serial(*m_serial);
  m_manager = new RHReliableDatagram(*m_driver, m_clientId);
}

bool COMController::init() {
  m_serial->begin(m_baud);
  m_max3243->disable();
  m_mux->comY1();
  m_manager->init();
}

void COMController::m_clearBuffer() {
  memset(m_buf, '\0', sizeof(char) * RH_SERIAL_MAX_MESSAGE_LEN);
}

bool COMController::m_send(char msg[]) {
  m_manager->setRetries(m_state->retries);
  uint8_t size = strlen(msg);
  if (m_manager->sendtoWait((uint8_t *)msg, size, m_serverId))
    return true;
  else
    return false;
}

bool COMController::m_receive(char buf[]) {
  uint8_t len = RH_SERIAL_MAX_MESSAGE_LEN;
  uint8_t from;
  if (m_manager->recvfromAckTimeout((uint8_t *)buf, &len, m_state->timeout,
                                    &from))
    return true;
  else
    return false;
}

void COMController::m_createRTS(JsonDocument &doc) {
  m_clearBuffer();
  doc["ID"] = "RTS";
  serializeJson(doc, m_buf);
  doc.clear();
}

bool COMController::request(JsonDocument &doc) {
  m_mux->comY1();
  if (m_radio->getZ9C())
    m_max3243->enable();

  m_createRTS(doc);
  console.debug(m_buf);
  if (!m_send(m_buf)) {
    console.debug("No ACK from server");
    m_max3243->disable();
    deserializeJson(doc, m_ACK_ERR);
    return false;
  }

  m_clearBuffer();
  if (!m_receive(m_buf)) {
    console.debug("Server ACK but no reply");
    m_max3243->disable();
    deserializeJson(doc, m_REP_ERR);
    return false;
  }

  console.debug("successfully received config, RADIO ----> GNSS");
  m_mux->comY2();
  DeserializationError error = deserializeJson(doc, m_buf);
  if (error)
    console.debug("Failed to deserialize response from base"); // ERROR STATE
  return true;
}

// TODO clear the document
bool COMController::upload(JsonDocument &doc) {
  m_clearBuffer();
  serializeJson(doc, m_buf);
  console.debug("RADIO ----> FEATHER");
  m_mux->comY1();

  if (!m_send(m_buf)) {
    console.debug("Failed to upload");
    doc.clear();
    deserializeJson(doc, m_ACK_ERR);
    return false;
  }

  if (m_radio->getZ9C())
    m_max3243->disable();

  console.debug("Succesfully uploaded");
  return true;
}

void COMController::resetRadio() { m_radio->reset(); }

bool COMController::channelBusy() { return m_radio->channel_busy(); }

void COMController::status(uint8_t verbosity, JsonDocument &doc) {}