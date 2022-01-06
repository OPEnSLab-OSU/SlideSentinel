#include "PoluluVRM.h"

PoluluVRM::PoluluVRM(uint8_t en) {
  m_en = en;
  pinMode(m_en, OUTPUT);
}


void PoluluVRM::enable() { 

  //Write high to the m_en pin, this will supply 3-5V to the m_en pin disabling sleep mode
  digitalWrite(m_en, HIGH); 

}

void PoluluVRM::disable() { 

    //Write low to the m_en pin, this will supply 1.5V to the m_en pin enabling sleep mode

  digitalWrite(m_en, LOW); 

}