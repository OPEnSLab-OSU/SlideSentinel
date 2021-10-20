#include "PMController.h"
#include "Console.h"
#include "FeatherTrace.h"

PMController::PMController(MAX4280 &max4280, PoluluVRM &vcc2,
                           Battery &bat, bool GNSSrail2, bool radioRail2)
    : m_max4280(max4280), m_vcc2(vcc2), m_bat(bat), m_GNSSRail2(GNSSrail2),
      m_RadioRail2(radioRail2) {

  // Enable sprintf function on SAMD21
  asm(".global _printf_float");
}

bool PMController::init() { MARK;
  // Set the XOSC32K to run in standby, external 32 KHz clock must be used for
  // interrupt detection in order to catch falling edges
  SYSCTRL->XOSC32K.bit.RUNSTDBY = 1;

  // INIT EXTERNAL OSCILLATOR FOR RISING AND FALLING interrupts  // Configure
  // EIC to use GCLK1 which uses XOSC32K This has to be done after the first
  // call to attachInterrupt()
  GCLK->CLKCTRL.reg =
      GCLK_CLKCTRL_ID(GCM_EIC) | GCLK_CLKCTRL_GEN_GCLK1 | GCLK_CLKCTRL_CLKEN;
  disableGNSS();
  disableRadio();
  console.debug("PMController initialized.\n");
  return true;
}

void PMController::enableGNSS() { MARK;
  if (m_GNSSRail2)
    m_vcc2.enable();
  m_max4280.assertRail(2);
  console.debug("GNSS on\n");
}

void PMController::disableGNSS() { MARK;
  if (m_GNSSRail2 && !m_RadioRail2)
    m_vcc2.disable();
  m_max4280.assertRail(3);
  console.debug("GNSS off\n");
}

void PMController::enableRadio() { MARK;
  if (m_RadioRail2)
    m_vcc2.enable();        //Polulu object
  m_max4280.assertRail(0);

  // delay for radio to initialize
  console.debug("Initializing radio...\n");
  for (int i = 0; i < 20; i++) { MARK;
    console.debug(i);
    console.debug(" ");
    delay(1000);
  }
  console.debug("\nRadio on\n");
}

void PMController::disableRadio() { MARK;
  if (m_RadioRail2)
    m_vcc2.disable();
  m_max4280.assertRail(1);
  console.debug("Radio off\n");
}

float PMController::readBat() { return m_bat.read(); }

char *PMController::readBatStr() {
  memset(m_volt, '\0', sizeof(char) * MAX_VOLT_LEN);
  sprintf(m_volt, "%.2f", readBat());
  return m_volt;
}

void PMController::sleep() { MARK;
  console.debug("Going to sleep...");
  // Disable USB
  USB->DEVICE.CTRLA.reg &= ~USB_CTRLA_ENABLE;

  // Enter sleep mode
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
  __DSB();
  __WFI();
  // ...Sleep

  // Enable USB
  USB->DEVICE.CTRLA.reg |= USB_CTRLA_ENABLE;
}

void PMController::status(SSModel &model) {
  model.setBat(readBat());
}

void PMController::update(SSModel &model) {}