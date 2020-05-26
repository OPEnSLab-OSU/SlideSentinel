#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include <Arduino.h>
#include "BaseModel.h"
#include "constants.hpp"

using namespace errorMsg;

// TODO make a base Model class, RoverModel and BaseModel inherit from this class and we can keep Controller base class code generic

class Controller {
public:
  Controller();
  virtual bool init() = 0;
  virtual void status(BaseModel &model) = 0;
  virtual void update(BaseModel &model) = 0;
};

#endif // _CONTROLLER_H_