#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include <Arduino.h>
#include "ArduinoJson.h"
#include "SSModel.h"

class Controller {
public:
  Controller(const char *header);
  virtual bool init() = 0;
  virtual void status(SSModel &model) = 0;
  virtual void update(SSModel &model) = 0;

protected:
  const char *m_HEADER;
};

#endif // _CONTROLLER_H_