#include "ConManager.h"

ConManager::ConManager() : m_size(0) {}

void ConManager::add(Controller *con) {
  if (m_size < MAX_CONTROLLERS)
    m_controllers[m_size] = con;
  m_size++;
}

void ConManager::status(SSModel &model) {
  for (int i = 0; i < m_size; i++)
    m_controllers[i]->status(model);
}

void ConManager::update(SSModel &model) {
  for (int i = 0; i < m_size; i++)
    m_controllers[i]->update(model);
}

bool ConManager::init() {
  for (int i = 0; i < m_size; i++) {
    if (!m_controllers[i]->init())
      return false;
  }
  return true;
}