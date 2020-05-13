#ifndef _GNSSDATA_H_
#define _GNSSDATA_H_

#include "ArduinoJson.h"
#include "SwiftPiksi.h"
#include <Arduino.h>

#define SS_GNSS_DATA "GNSS"
#define MAX_GNSS_LEN 1000
#define FIX_MODE 0
#define GPS_TIME_WN 1
#define GPS_TIME_TOW 2
#define POS_LLH_LAT 3
#define POS_LLH_LON 4
#define POS_LLH_HEIGHT 5
#define POS_LLH_N_SATS 6
#define BASELINE_N 7
#define BASELINE_E 8
#define BASELINE_D 9
#define VEL_N 10
#define VEL_E 11
#define VEL_D 12
#define DOPS_GDOP 13
#define DOPS_HDOP 14
#define DOPS_PDOP 15
#define DOPS_TDOP 16
#define DOPS_VDOP 17

// Data handler which can be used on both the rover and the base station
class GNSSData {
private:
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

public:
  GNSSData();
  void write(JsonDocument &doc);
  void read(char *buf);
  void setGpsTimeWn(uint16_t wn);
  void setGpsTimeTow(uint32_t tow);
  void setPosLat(double lat);
  void setPosLon(double lon);
  void setPosLat(double lat);
  void setPosHeight(double height);
  void setPosMode(uint8_t mode);
  void setPosNumSats(uint8_t n_sats);
  void setBaseN(int32_t n);
  void setBaseE(int32_t e);
  void setBaseD(int32_t d);
  void setVelN(int32_t n);
  void setVelD(int32_t d);
  void setVelE(int32_t e);
  void setDopGdop(uint16_t gdop);
  void setDopHdop(uint16_t hdop);
  void setDopPdop(uint16_t pdop);
  void setDopTdop(uint16_t tdop);
  void setDopVdop(uint16_t vdop);

  void clear();
};

#endif // _GNSSDATA_H_
