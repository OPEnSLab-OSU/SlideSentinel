#include "Console.h"

Console::Console() : m_debug (true) {};

void Console::setDebug(bool debug) 
{
    m_debug = debug;
}

void Console::debug(const char* message)
{
    if(m_debug)
        Serial.print(message);
}

void Console::debug(int val)
{
    if(m_debug)
        Serial.print(val);
}

void Console::debug(float val)
{
    if(m_debug)
        Serial.print(val);
}

void Console::debug(double val)
{
    if(m_debug)
        Serial.print(val);
}

Console console; 