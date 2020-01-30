#ifndef _MEMORYCONTROLLER_H_
#define _MEMORYCONTROLLER_H_

#include <Arduino.h>
#include "ArduinoJson.h"

/* 
JSON messaging protocol
Types: 
    - DAT
        - GNSS positional?
        - Accelerometer data 
        - Rover State
            - Battery voltage
            - Wake frequency 
            - Wake duration
            - failed packet count
    - RTS
    - ACK
    - ERR
        - Error messages, written to Error Logs
        - Add timestamp, state
    - CONF
    - ALRT
*/

// First think about how we want to store our data!

class MemoryController {
    private: 

    public: 
        MemoryController();
        bool write(JsonDocument& doc);
        bool read(JsonDocument &doc);
};

#endif // _MEMORYCONTROLLER_H_