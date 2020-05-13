#include "GNSSData.h"
#include "Console.h"

GNSSData::GNSSData();
void GNSSData::write(JsonDocument &doc);
void GNSSData::read(char *buf);

void GNSSData::setGpsTimeWn(uint16_t wn) { m_wn = wn; }
void GNSSData::setGpsTimeTow(uint32_t tow);
void GNSSData::setPosLat(double lat);
void GNSSData::setPosLon(double lon);
void GNSSData::setPosLat(double lat);
void GNSSData::setPosHeight(double height);
void GNSSData::setPosMode(uint8_t mode);
void GNSSData::setPosNumSats(uint8_t n_sats);
void GNSSData::setBaseN(int32_t n);
void GNSSData::setBaseE(int32_t e);
void GNSSData::setBaseD(int32_t d);
void GNSSData::setVelN(int32_t n);
void GNSSData::setVelD(int32_t d);
void GNSSData::setVelE(int32_t e);
void GNSSData::setDopGdop(uint16_t gdop);
void GNSSData::setDopHdop(uint16_t hdop);
void GNSSData::setDopPdop(uint16_t pdop);
void GNSSData::setDopTdop(uint16_t tdop);
void GNSSData::setDopVdop(uint16_t vdop);


  uint16_t m_wn;
  uint32_t m_tow;
  double m_lat;
  double m_lon;
  double m_lat;
  double m_height;
  uint8_t m_mode;
  uint8_t m_n_sats;
  int32_t m_base_n;
  int32_t m_base_e;
  int32_t m_base_d;
  int32_t m_vel_n;
  int32_t m_vel_e;
  int32_t m_vel_d;
  uint16_t m_gdop;
  uint16_t m_hdop;
  uint16_t m_pdop;
  uint16_t m_tdop;
  uint16_t m_vdop;