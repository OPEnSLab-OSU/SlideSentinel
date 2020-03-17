#ifndef _CONMANAGER_H_
#define _CONMANAGER_H_
#define MAX_CONTROLLERS 10

#include <Arduino.h>
#include "Controller.h"
#include "SSModel.h"

class ConManager {
private:
  int m_size;
  Controller *m_controllers[MAX_CONTROLLERS];

public:
  ConManager();
  void add(Controller *con);
  void status(SSModel &model); // sets a flag in the model, diag, state, data,
  void update(SSModel &model);
  bool init();
};

#endif // _CONMANAGER_H_