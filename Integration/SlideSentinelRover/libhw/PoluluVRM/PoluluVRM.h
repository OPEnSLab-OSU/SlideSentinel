#ifndef _POLULUVRM_H_
#define _POLULUVRM_H_

#include <Arduino.h>
/**
 * @brief The PoluluVoltageReg (VR) is the implementation class for the D36V50FX voltage regulator which
 * takes in a voltage (5-50V) from Vin on the PCB and outputs its regulated voltage. This class can only enable low power mode 
 * if the VCC2_EN jumper is in place on the PCB.
 *  
 */


class PoluluVRM {
private:
  uint8_t m_en;

public:
/* @param uint8_t en Represents the microcontroller's pin number connected to the EN pin.*/
  PoluluVRM(uint8_t en);

  /* When enabled, the VR will output its rated power depending on the model.*/
  void enable();

  /* When disabled, the VR will only draw enough power to power itself, around 10-20uA. */
  void disable();
};

#endif // _POLULUVRM_H_
