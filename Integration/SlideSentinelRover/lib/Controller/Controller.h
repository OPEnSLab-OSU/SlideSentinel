#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include <Arduino.h>
#include "SSModel.h"
#include "constants.hpp"

using namespace errorMsg;

class Controller {
public:
  Controller();
  virtual bool init() = 0;
  virtual void status(SSModel &model) = 0;
  virtual void update(SSModel &model) = 0;
};

#endif // _CONTROLLER_H_