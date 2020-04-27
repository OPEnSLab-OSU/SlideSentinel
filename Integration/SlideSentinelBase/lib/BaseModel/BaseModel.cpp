#include "BaseModel.h"
#include "Console.h"

BaseModel::BaseModel(int numRovers) : m_numRovers(numRovers) {
  m_shadow = new Shadow[numRovers];
}

char *BaseModel::getDiag(int id) { return m_shadow[id - 1].toDiag(); }

bool BaseModel::getRoverIMUFlag(int id) {
  return m_shadow[id - 1].getIMUFlag();
}

int BaseModel::getRoverWakeTime(int id) {
  return m_shadow[id - 1].getWakeTime();
}

char *BaseModel::getProps(int id) { return m_shadow[id - 1].toProps(); }

char *BaseModel::getData(int id) { return m_shadow[id - 1].toData(); }

void BaseModel::setDiag(int id, char *buf) { m_shadow[id - 1].setDiag(buf); }

void BaseModel::setProps(int id, char *buf) { m_shadow[id - 1].setProps(buf); }

void BaseModel::setData(int id, char *buf) { m_shadow[id - 1].setData(buf); }

void BaseModel::print() {
  for (int i = 0; i < m_numRovers; i++) {
    console.debug("\n\nRover ");
    console.debug(i);
    console.debug(":\n");
    m_shadow[i].print();
  }
}

void BaseModel::setError(char *err) { m_err = err; }

char *BaseModel::getError() { return m_err; }

void BaseModel::setRoverServe(int rover_id) { m_roverServeId = rover_id; }
void BaseModel::setRoverAlert(int rover_id) { m_roverAlertId = rover_id; }

int BaseModel::getRoverServe() { return m_roverServeId; }
int BaseModel::getRoverAlert() { return m_roverAlertId; }