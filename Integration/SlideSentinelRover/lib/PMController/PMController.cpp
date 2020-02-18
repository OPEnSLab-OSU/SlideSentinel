#include "PMController.h"
#include "Console.h"
#define DEBUG true

PMController::PMController(MAX4280 *max4280, PoluluVoltageReg *vcc2,
                           Battery *bat, bool GNSSrail2, bool radioRail2)
    : Controller("PM"), m_max4280(max4280), m_vcc2(vcc2), m_bat(bat), m_GNSSRail2(GNSSrail2),
      m_RadioRail2(radioRail2) {
        
  // Enable sprintf function on SAMD21
  asm(".global _printf_float");
}

void PMController::init() {
  // Set the XOSC32K to run in standby, external 32 KHz clock must be used for
  // interrupt detection in order to catch falling edges
  SYSCTRL->XOSC32K.bit.RUNSTDBY = 1;

  // INIT EXTERNAL OSCILLATOR FOR RISING AND FALLING interrupts  // Configure
  // EIC to use GCLK1 which uses XOSC32K This has to be done after the first
  // call to attachInterrupt()
  GCLK->CLKCTRL.reg =
      GCLK_CLKCTRL_ID(GCM_EIC) | GCLK_CLKCTRL_GEN_GCLK1 | GCLK_CLKCTRL_CLKEN;
}

void PMController::enableGNSS() {
  if (m_GNSSRail2)
    m_vcc2->enable();
  m_max4280->assertRail(2);
  console.debug("GNSS on");
}

void PMController::disableGNSS() {
  if (m_GNSSRail2 && !m_RadioRail2)
    m_vcc2->disable();
  m_max4280->assertRail(3);
  console.debug("GNSS off");
}

void PMController::enableRadio() {
  if (m_RadioRail2)
    m_vcc2->enable();
  m_max4280->assertRail(0);
  console.debug("Radio on");
}

void PMController::disableRadio() {
  if (m_RadioRail2)
    m_vcc2->disable();
  m_max4280->assertRail(1);
  console.debug("radio off");
}

float PMController::readBat() { return m_bat->read(); }

void PMController::readBatStr(char buf[]) { sprintf(buf, "%.2f", readBat()); }

void PMController::sleep() {
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

void PMController::update(JsonDocument &doc) {}

void PMController::status(uint8_t verbosity, JsonDocument &doc) {}

