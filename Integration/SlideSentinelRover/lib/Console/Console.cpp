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

void Console::log(int val)
{
    if(m_debug)
        Serial.print(val);
}

Console console; 