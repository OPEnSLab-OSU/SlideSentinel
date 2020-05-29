#include "Console.h"
#include "FeatherTrace.h"

Console::Console() : m_debug (true) {};

void Console::setDebug(bool debug) 
{
    m_debug = debug;
}

void Console::debug(const char* message)
{ MARK;
    if(m_debug)
        Serial.print(message);
}

void Console::debug(int val)
{ MARK;
    if(m_debug)
        Serial.print(val);
}

void Console::debug(float val)
{ MARK;
    if(m_debug)
        Serial.print(val);
}

void Console::debug(double val)
{ MARK;
    if(m_debug)
        Serial.print(val);
}

Console console; 