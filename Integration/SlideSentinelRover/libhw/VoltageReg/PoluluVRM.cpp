#include "PoluluVRM.h"

PoluluVRM::PoluluVRM(uint8_t en) {
  m_en = en;
  pinMode(m_en, OUTPUT);
}

void PoluluVRM::enable() { digitalWrite(m_en, HIGH); }

void PoluluVRM::disable() { digitalWrite(m_en, LOW); }