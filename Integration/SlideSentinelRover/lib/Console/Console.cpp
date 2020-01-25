#include "Console.h"

Console::Console() : m_debug (true) {};

void Console::setDebug(bool debug) 
{
    m_debug = debug;
}

void Console::debug(const char* message)
{
    if(m_debug)
        Serial.println(message);
}

Console console; 