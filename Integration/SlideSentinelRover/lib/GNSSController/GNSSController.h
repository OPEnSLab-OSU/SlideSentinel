#ifndef _GNSSCONTROLLER_H_
#define _GNSSCONTROLLER_H_

#include <Arduino.h>
#include "HardwareSerial.h"
#include "ArduinoJson.h"
#include "sbp.h"
#include "navigation.h"
#include "Tutorial.h"

class GNSSController
{
private:
    HardwareSerial *m_serial;
    void _clearBuffer();

public:
    GNSSController();
    bool poll(JsonDocument &doc);
};

#endif // _GNSSCONTROLLER_H_