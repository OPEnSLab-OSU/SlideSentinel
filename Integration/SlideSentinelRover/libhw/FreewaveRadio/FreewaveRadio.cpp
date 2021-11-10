#include "FreewaveRadio.h"
#include "FeatherTrace.h"

FreewaveRadio::FreewaveRadio(uint8_t reset, uint8_t cd)
    : m_rst(reset), m_cd(cd) {
  pinMode(m_rst, OUTPUT);
  pinMode(m_cd, INPUT);
  digitalWrite(m_rst, HIGH);
}

bool FreewaveRadio::channel_busy() { return digitalRead(m_cd) == HIGH; }



void FreewaveRadio::reset() {
  digitalWrite(m_rst, LOW);
  MARK;
  delay(2000);
  MARK;
  digitalWrite(m_rst, HIGH);
}