#include "SN74LVC2G53.h"

SN74LVC2G53::SN74LVC2G53(uint8_t switchPin, uint8_t controlSwitch)
    : m_switchPin(switchPin)
    , m_controlSwitch(controlSwitch)
    {
    if(m_controlSwitch != -1)
        pinMode(m_controlSwitch, OUTPUT);

    pinMode(m_switchPin, OUTPUT);
    }

bool SN74LVC2G53::getDirection(){
    if(m_switchDir == true){
        Serial.println("Flow between COM and Y1");
    else 
        Serial.println("Flow between COM and Y2");

    return m_switchDir;
}

bool SN74LVC2G53::enableSwitch(){
    if(m_controlSwitch == -1)
        return false;

    digitalWrite(m_controlSwitch, LOW);
    Serial.println("Mux Enabled");
    return true;
}

bool SN74LVC2G53::disableSwitch(){
    if(m_controlSwitch == -1)
        return false;

    digitalWrite(m_controlSwitch, HIGH);
    Serial.println("Mux Disabled");
    return true;
}

void SN74LVC2G53::COMtoY1(){
    digitalWrite(m_switchPin, LOW);
    m_switchDir = true;
    printDirection();
}

void SN74LVC2G53::COMtoY2(){
    digitalWrite(m_switchPin, HIGH);
    m_switchDir = false;
    printDirection();
}