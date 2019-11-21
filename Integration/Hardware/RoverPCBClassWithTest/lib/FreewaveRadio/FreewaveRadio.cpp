#include "FreewaveRadio.h"

Freewave::Freewave(HardwareSerial* device, uint8_t reset, uint8_t cd, uint8_t client, uint8_t server, bool is_z9c)
    : m_device(device)
    , m_reset_pin(reset)
    , m_cd_pin(cd)
    , m_CLIENT_ADDRESS(client)
    , m_SERVER_ADDRESS(server)
    , m_is_z9c(is_z9c)
    {
        m_manager(m_device, m_CLIENT_ADDRESS);
        pinMode(m_reset_pin, OUTPUT);
        pinMode(m_cd_pin, INPUT);
    }   



bool Freewave::read(uint8_t* buf, uint8_t* from){
    if(!available())
        return false;

    uint8_t len, to, id, flags;
    return m_manager.recvfromAck(buf, &_len, &from, &to, &id, &flags);
}


bool Freewave::send(unint8_t* data, uint8_t len){
    if(len > RH_SERIAL_MAX_MESSAGE_LEN)
        return false;
        // splitTranmissionMessage()

    return m_manager.sendtoWait(data, len, m_SERVER_ADDRESS);
}


bool Freewave::channel_busy(){
    return digitalRead(m_cd_pin);
}

void Freewave::reset(){

}