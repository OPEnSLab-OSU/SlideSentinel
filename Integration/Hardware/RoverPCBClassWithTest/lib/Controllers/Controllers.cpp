#include "Controllers.h"

SerialCommunication::SerialCommunication(uint8_t FORCEOFF_N)
    : MAX3243(FORCEOFF_N)
    { }

void SerialCommunictaion::setRS232(){
    enable();
    m_s_comm_type = 1;
}

void SerialCommunication::setTTL(){
    disable();
    m_s_comm_type = 2;
}

void SerialCommunication::printSerialCommunicationType(){
    if(m_s_comm_type == 1)
        Serial.println("MAX3243 Enabled, Converting RS232 Signals From Radio to TTL");
    else 
        Serial.println("MAX3243 Disabled, Direct TTL Signals From Radio to Feather")
}

Radio::Radio(HardwareSerial* device, uint8_t reset, uint8_t cd, uint8_t client, uint8_t server, bool is_z9c, uint8_t SPDT_SEL)
    : Freewave(device, reset, cd, client, server, is_z9c)
    , SN74LVC2G53(SPDT_SEL)
    { 
        radioToFeather();
    }

bool Radio::m_getRadioRxDirection(){
    return getDirection();
}

void Radio::radioToFeather(){
    COMtoY1();
}

void Radio::radioToGNSS(){
    COMtoY2();
}

void Radio::sendMessage(uint8_t* mssg, uint8_t len){
    if(len > RH_SERIAL_MAX_MESSAGE_LEN){
        m_sendLargeMessage(mssg, len);
        return;    
    }

    if(send(mssg, len)){
        //For these prints may want to add a DEBUG feature to only enable them when DEBUG == true
        Serial.println("Message Successfully Sent!");
        return;
    }
    Serial.println("Message Failed to Send!");
}

void Radio::receiveMessage(uint8_t* mssg){
    if(getRadioRxDirection() == false){
        Serial.println("Switch radioToFeather to receive transmission");
        return;
    }

    
}

// This function will probably need a modification to how it copies arrays. Mem allocation type stuff but logic good
void Radio::m_sendLargeMessage(uint8_t *mssg, uint8_t len){
    uint8_t message_windows = floor(len / RH_SERIAL_MAX_MESSAGE_LEN);
    uint8_t* tempMssg;
    bool mssgSent = true;
    for(uint8_t i = 0; i < message_windows; i++){
        tempMssg = copy(mssg + (i * RH_SERIAL_MAX_MESSAGE_LEN), mssg + ((i + 1) * RH_SERIAL_MAX_MESSAGE_LEN) - 1, tempMssg);
        if(!send(tempMssg, sizeof(tempMessage))){
            Serial.println("Sub string message failed to send!");
            mssgSent = false;
        }  
    }

    if(len % RH_SERIAL_MAX_MESSAGE_LEN != 0){
        uint8_t remainingMssgLen = len - (RH_SERIAL_MAX_MESSAGE_LEN * message_windows);
        tempMssg = copy(mssg + (message_windows * RH_SERIAL_MAX_MESSAGE_LEN), mssg + (message_windows * RH_SERIAL_MAX_MESSAGE_LEN) + remainingMssgLen);
        if(!send(tempMssg, sizeof(tempMssg)))
            Serial.println("Last sub string message failed to send!")
    }

    if(mssgSent == true)
        Serial.println("Message Successfully Sent!");
    else
        Serial.println("Message Failed to Send!");
}