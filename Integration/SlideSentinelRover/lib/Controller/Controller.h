#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include <Arduino.h>
#include "ArduinoJson.h"
#include "Prop.h"

class Controller
{
    public: 
        Controller(const char* header, Prop& prop);
        virtual bool init() = 0;
        virtual void status(uint8_t verbosity, JsonDocument &doc) = 0; 

    protected:
        const char* m_HEADER;  
        Prop& m_prop;
};

#endif // _CONTROLLER_H_