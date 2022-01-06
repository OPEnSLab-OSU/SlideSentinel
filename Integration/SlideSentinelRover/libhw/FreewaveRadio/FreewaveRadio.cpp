#include "FreewaveRadio.h"
#include "FeatherTrace.h"

Freewave::Freewave(/*uint8_t reset, uint8_t cd, RadioType radioType*/){ //add vairables
  pinMode(m_rst, OUTPUT);
  pinMode(m_cd, INPUT);
  digitalWrite(m_rst, HIGH);
}

bool Freewave::channel_busy() { return digitalRead(m_cd) == HIGH; }



void Freewave::reset() {
  digitalWrite(m_rst, LOW);
  delay(2000);
  digitalWrite(m_rst, HIGH);
}

Freewave::RadioType Freewave::getType(){
  return m_radioType;
}