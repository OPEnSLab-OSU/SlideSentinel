#ifndef _POLULUVRM_H_
#define _POLULUVRM_H_

#include <Arduino.h>
/**
 * @brief The PoluluVoltageReg (VR) is the implementation class for the D36V50F(3/5) voltage regulator which
 * takes in 12 volts from Vin on the PCB and outputs either 3.3V or 5V. This class can only enable low power mode 
 * if the VCC2_EN jumper is in place on the PCB.
 *  
 */


class PoluluVRM {
private:
  uint8_t m_en;

public:
/* @param uint8_t en Represents the pin number, in the Rover's case, this is pin 13.*/
  PoluluVRM(uint8_t en);

  /* When enabled, the VR will output either 3.3V or 5V depending on the model.*/
  void enable();

  /* When disabled, the VR will only draw enough power to power itself, around 10-20uA. */
  void disable();
};

#endif // _POLULUVRM_H_
