#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include <Arduino.h>
#include "ArduinoJson.h"
#include "State.h"

class Controller
{
    public: 
        Controller(const char* header, State* state);
        virtual bool init() = 0;
        virtual void status(uint8_t verbosity, JsonDocument &doc) = 0; 

    protected:
        const char* m_HEADER;  
        State* m_state;
};

#endif // _CONTROLLER_H_