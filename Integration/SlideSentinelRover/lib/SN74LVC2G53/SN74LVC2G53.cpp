#include "SN74LVC2G53.h"

SN74LVC2G53::SN74LVC2G53(HardwareSerial &monitor, uint8_t switchPin, uint8_t controlSwitch)
    : m_monitor(monitor)
    , m_switchPin(switchPin)
    , m_controlSwitch(controlSwitch)
    {
    pinMode(m_switchPin, OUTPUT);
    pinMode(m_controlSwitch, OUTPUT);
    }

void SN74LVC2G53::currentDirection(){
    if(m_switchDir == 1)
        m_monitor.println("Radio Tx to Feather Rx");
    else 
        m_monitor.println("Radio Tx to GNSS Rx");
}

bool SN74LVC2G53::enableSwitch(){
    digitalWrite(m_controlSwitch, LOW);
    m_monitor.println("Mux Enabled");
    return true;
}

bool SN74LVC2G53::disableSwitch(){
    digitalWrite(m_controlSwitch, HIGH);
    m_monitor.println("Mux Disabled");
    return true;
}

void SN74LVC2G53::radioToFeather(){
    digitalWrite(m_switchPin, LOW);
    m_switchDir = 1;
    currentDirection();
}

void SN74LVC2G53::radioToGNSS(){
    digitalWrite(m_switchPin, HIGH);
    m_switchDir = 2;
    currentDirection();
}