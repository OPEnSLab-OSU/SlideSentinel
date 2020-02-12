#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include <Arduino.h>
#include "ArduinoJson.h"

class Controller
{
    public: 
        Controller(const char* header);
        virtual void update(JsonDocument &doc) = 0;
        virtual void status(uint8_t verbosity, JsonDocument &doc) = 0; 

    protected:
        const char* m_HEADER;  
};

#endif // _CONTROLLER_H_