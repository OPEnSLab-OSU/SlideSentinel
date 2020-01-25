#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <Arduino.h>

class Console
{
    private:
        bool m_debug;

    public:
        Console();
        void setDebug(bool debug);
        void debug(const char* message);
};

extern Console console;

#endif // _CONSOLE_H_