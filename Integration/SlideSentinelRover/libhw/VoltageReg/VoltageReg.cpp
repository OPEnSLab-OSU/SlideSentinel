#include "VoltageReg.h"

PoluluVoltageReg::PoluluVoltageReg(uint8_t en) {
  m_en = en;
  pinMode(m_en, OUTPUT);
}

void PoluluVoltageReg::enable() { digitalWrite(m_en, HIGH); }

void PoluluVoltageReg::disable() { digitalWrite(m_en, LOW); }