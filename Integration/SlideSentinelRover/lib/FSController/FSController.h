#ifndef _FSCONTROLLER_H_
#define _FSCONTROLLER_H_

#include <SD.h>
#include "ArduinoJson.h"
#include "Controller.h"

class FSController : public Controller {
private:
  //const char *m_RTS;

public:
  FSController();
  void update(JsonDocument &doc);
  void status(uint8_t verbosity, JsonDocument &doc);
};

#endif // _FSCONTROLLER_H_