#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include <Arduino.h>
#include "SSModel.h"
#include "constants.hpp"

using namespace errorMsg;


/**
 * The controller class is used as a parent object for each controller 
 * (Each controller is derived from this class). This is because each controller 
 * contains the 3 functions, init(), status(), and update(). These polymorphic 
 * function calls are managed by the ConManager class. 
 */
class Controller {
public:

  /**
   * Class constructor for Controller. Nothing happens here.
   */
  Controller();

  /**
   * Parent function defined as virtual. No functionality here.
   */
  virtual bool init() = 0;

  /**
   * Parent function defined as virtual. No functionality here.
   */
  virtual void status(SSModel &model) = 0;

  /**
   * Parent function defined as virtual. No functionality here.
   */
  virtual void update(SSModel &model) = 0;
};

#endif // _CONTROLLER_H_