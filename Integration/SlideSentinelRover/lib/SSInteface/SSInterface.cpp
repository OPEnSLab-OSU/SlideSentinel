#include "SSInterface.h"
#include "Console.h"

// Note, both base and rover need to use timeout receiving
// Note, base needs to pass serverID to manager, rover needs to pass clientID Figure out the ID stuff!

// FIXME  FLush serial buffer on failure!
SSInterface::SSInterface(HardwareSerial &serial, uint32_t baud,
                         uint8_t clientId, uint8_t serverId, uint16_t timeout,
                         uint8_t retries, bool isBase)
    : m_serial(serial), m_baud(baud), m_clientId(clientId),
      m_serverId(serverId), m_driver(m_serial), m_manager(m_driver, m_clientId),
      m_timeout(timeout), m_retries(retries), m_base(isBase),
      m_blen(RH_SERIAL_MAX_MESSAGE_LEN - 1){};

bool SSInterface::sendPacket(const char *type, char *packet) {
  // determine the number of fragments
  // console.debug("\nin sendPacket:");
  // console.debug(packet);
  _setOutFrag(packet);
  // console.debug("\noutFrag:");
  // console.debug(m_outFrag);

  // create the header in the buffer
  _header(type);

  // console.debug("\nheader:");
  // console.debug(m_buf);

  // send the header
  if (!_send())
    return false;

  for (int i = 0; i < m_outFrag; i++) {
    _clearBuffer(); // not sure I need this
    memcpy(m_buf, packet + (i * m_blen), m_blen);
    // console.debug("\n\n");
    // console.debug("Fragment ");
    // console.debug(i);
    // console.debug("\n");
    // console.debug(m_buf);
    // console.debug("\n\n");
    if (!_send())
      return false;
  }
  return true;
}

// return an int corresponding to the type of request?
bool SSInterface::receivePacket(char *buffer) {
  if (!_receive())
    return false;

  _readHeader(m_buf);
  for (int i = 0; i < m_inFrag; i++) {
    if (!_receive())
      return false;
    memcpy(buffer + (i * m_blen), m_buf, m_blen);
  }
  return true;
}

bool SSInterface::_receive() {
  uint8_t len = RH_SERIAL_MAX_MESSAGE_LEN;
  uint8_t from;
  _clearBuffer();
  if (m_manager.recvfromAckTimeout((uint8_t *)m_buf, &len, m_timeout, &from))
    return true;
  return false;
}

// TODO set the ID of the node we received from and the type!!
int SSInterface::_readHeader(char *buf) {
  StaticJsonDocument<RH_SERIAL_MAX_MESSAGE_LEN> doc;
  auto error = deserializeJson(doc, buf);
  if (error)
    return false;
  m_inFrag = doc[FRAGMENT_NUM];
}

void SSInterface::init() {
  m_serial.begin(m_baud);
  m_manager.init();
}

char *SSInterface::_getBuf() { return m_buf; }

bool SSInterface::_send() {
  uint8_t size = strlen(m_buf);
  if (m_manager.sendtoWait((uint8_t *)m_buf, size, m_serverId))   // ON THE ROVER
    return true;
  return false;
}

void SSInterface::_clearBuffer() {
  memset(m_buf, '\0', sizeof(char) * RH_SERIAL_MAX_MESSAGE_LEN);
}

void SSInterface::_header(const char *type) {
  StaticJsonDocument<RH_SERIAL_MAX_MESSAGE_LEN> doc;
  if (!m_base)
    _addId(doc);
  _addType(doc, type);
  _addFragment(doc);
  serializeJsonPretty(doc, Serial);
  _serializePkt(doc);
}

void SSInterface::_addType(JsonDocument &doc, const char *type) {
  doc[TYPE] = type;
}

void SSInterface::_addId(JsonDocument &doc) { doc[ROVER_ID] = m_clientId; }

void SSInterface::_setOutFrag(char *buf) {
  int len = strlen(buf);
  int num = len / m_blen;
  if (len % m_blen)
    num++;
  m_outFrag = num;
}

void SSInterface::_addFragment(JsonDocument &doc) {
  doc[FRAGMENT_NUM] = m_outFrag;
}

void SSInterface::_serializePkt(JsonDocument &doc) {
  _clearBuffer();
  serializeJson(doc, m_buf);
}

void SSInterface::setTimeout(uint16_t timeout) { m_timeout = timeout; }

void SSInterface::setRetries(uint16_t retries) {
  m_retries = retries;
  m_manager.setRetries(m_retries);
}

void SSInterface::setClient(uint8_t addr) { m_clientId = addr; }
void SSInterface::setServer(uint8_t addr) { m_serverId = addr; }

int SSInterface::getTimeout() { return m_timeout; }
int SSInterface::getRetries() { return m_retries; }

bool SSInterface::available() { return m_manager.available(); }