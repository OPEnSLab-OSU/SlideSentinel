#include "ConManager.h"
#include "Console.h"
#include "FeatherTrace.h"

ConManager::ConManager() : m_size(0) {}

void ConManager::add(Controller *con) {
  if (m_size < MAX_CONTROLLERS)
    m_controllers[m_size] = con;
  m_size++;
}

void ConManager::status(SSModel &model) {
  console.debug("Collecting system status...\n");
  for (int i = 0; i < m_size; i++) { MARK;
      m_controllers[i]->status(model);
  }
}

void ConManager::update(SSModel &model) {
  console.debug("Updating system...\n");
  for (int i = 0; i < m_size; i++) { MARK;
      m_controllers[i]->update(model);
  }
}

bool ConManager::init() {
  for (int i = 0; i < m_size - 1; i++) {
    MARK;
    if (!m_controllers[i]->init())
      return false;
  }

  //This is specifically for initializing the FSController. Make sure it stays last in the list
  //We intend on running the system regardless of whether SD initializes or not.
  m_controllers[m_size-1]->init();
  return true;
}