#include "BaseModel.h"
#include "Console.h"

BaseModel::BaseModel(int numRovers) : m_numRovers(numRovers) {
  m_shadow = new Shadow[numRovers];
}

char *BaseModel::getDiag(int id) { return m_shadow[id].toDiag(); }

char *BaseModel::getProps(int id) { return m_shadow[id].toProps(); }

char *BaseModel::getData(int id) { return m_shadow[id].toData(); }

void BaseModel::setDiag(int id, char *buf) { m_shadow[id].setDiag(buf); }

void BaseModel::setProps(int id, char *buf) { m_shadow[id].setProps(buf); }

void BaseModel::setData(int id, char *buf) { m_shadow[id].setData(buf); }

bool BaseModel::dataReady() { return m_dataReady; }

void BaseModel::print() {
  for (int i = 0; i < m_numRovers; i++) {
    console.debug("\nRover ");
    console.debug(i);
    console.debug(":\n");
    m_shadow[i].print();
  }
}

void BaseModel::setError(char *err) { m_err = err; }

char *BaseModel::getError() { return m_err; }