#include "ComController.h"
#include "Console.h"

ComController::ComController(Freewave *radio, MAX3243 *max3243, SN74LVC2G53 *mux, HardwareSerial *serial, uint32_t baud, uint8_t clientId, uint8_t serverId) : 
    m_radio(radio), m_max3243(max3243), m_mux(mux), m_serial(serial), m_baud(baud), m_clientId(clientId), m_serverId(serverId), m_timeout(2000), 
    m_RTS("{\"type\":\"RTS\"}"), m_ACK_ERR("{\"type\":\"ACK_ERR\"}"), m_REP_ERR("{\"type\":\"REP_ERR\"}")
{
    m_serial->begin(m_baud);
    m_mux->comY1();
    m_max3243->disable();
    m_driver = new RH_Serial(*m_serial);
    m_manager = new RHReliableDatagram(*m_driver, m_clientId);
    m_manager->init();
}

void ComController::_clearBuffer()
{
    memset(m_buf, '\0', sizeof(m_buf) / sizeof(m_buf[0]));
}

void ComController::setTimeout(uint16_t time)
{
    m_timeout = time;
}

void ComController::setRetries(uint8_t num)
{
    m_manager->setRetries(num);
}

bool ComController::_send(char msg[])
{
    uint8_t size = strlen(msg);
    Serial.println(msg);
    Serial.println(size);
    Serial.println(m_serverId);
    if (m_manager->sendtoWait((uint8_t *)msg, size, m_serverId))
        return true;
    else
        return false;
}

bool ComController::_receive(char buf[])
{
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (m_manager->recvfromAckTimeout((uint8_t *)buf, &len, m_timeout, &from))
        return true;
    else
        return false;
}

bool ComController::request(JsonDocument &doc)
{   
    m_mux->comY1();
    if(m_radio->getZ9C())
        m_max3243->enable();

    if (!_send((char *)m_RTS))
    {
        console.debug("No ACK from server"); 
        m_max3243->disable(); 
        deserializeJson(doc, m_ACK_ERR);
        return false;
    }

    _clearBuffer();
    if (!_receive(m_buf))
    {
        console.debug("Server ACK but no reply"); 
        m_max3243->disable(); 
        deserializeJson(doc, m_REP_ERR);
        return false;
    }

    console.debug("successfully received config, RADIO ----> GNSS");
    m_mux->comY2();
    DeserializationError error = deserializeJson(doc, m_buf);
    if (error)
        console.debug("Failed to deserialize response from base");      // ERROR STATE
    return true;
}

bool ComController::upload(JsonDocument &doc)
{
    _clearBuffer();
    serializeJson(doc, m_buf);
    console.debug("RADIO ----> FEATHER");
    m_mux->comY1();
    if (!_send(m_buf)){
        console.debug("Failed to upload"); 
        deserializeJson(doc, m_ACK_ERR);
        return false;
    }

    m_max3243->disable(); 
    console.debug("Succesfully uploaded"); 
    return true;
}