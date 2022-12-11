#include "SN74LVC2G53.h"

SN74LVC2G53::SN74LVC2G53(int sel, int inh)
  : m_sel(sel)
  , m_inh(inh) 
{
  pinMode(m_sel, OUTPUT);
  if (m_inh >= 0)
    pinMode(m_inh, OUTPUT);
}

void SN74LVC2G53::comY1() { digitalWrite(m_sel, LOW); }

void SN74LVC2G53::comY2() { digitalWrite(m_sel, HIGH); }

void SN74LVC2G53::enable() { if (m_inh >= 0) digitalWrite(m_inh, LOW); }

void SN74LVC2G53::disable() { if (m_inh >= 0) digitalWrite(m_inh, HIGH); }
