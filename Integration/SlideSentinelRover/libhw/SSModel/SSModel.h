#ifndef _SSMODEL_H_
#define _SSMODEL_H_

#include <Arduino.h>
#include "ArduinoJson.h"
#include "Diagnostics.h"
#include "Properties.h"
#include "SwiftPiksi.h"
#include "constants.hpp"

#define SS_DATA "GNSS"

using namespace errorMsg;


/**
 * This class stores all of the relevant data picked up from the sensor.
 * 
 * This class can be considered at the top of the hierarchy in the code. 
 * All data collected is stored in the SSModel. This class also contains 
 * instances of the Property, Diagnostic, and GNSS 'DATA' classes.
 */
class SSModel {
private:
  Properties m_props; ///< Object stores all property data.
  Diagnostics m_diag; ///< Object stores all diagnostic data.

  // GNSSController Data
  msg_pos_llh_t m_pos_llh;           ///< data
  msg_baseline_ned_t m_baseline_ned; ///< data
  msg_vel_ned_t m_vel_ned;           ///< data
  msg_dops_t m_dops;                 ///< data
  msg_gps_time_t m_gps_time;         ///< data
  uint8_t m_mode;                    ///< data
  char *m_err;                       ///< error

 
  char m_buffer[MAX_DATA_LEN];  ///< container for all constructed data.

  /**
   * Adds the data stored from the GNSSController Data above and
   * attaches it to a JSON document.
   * 
   * @param doc JSON doc used to combine all data collected.
   */
  void m_addData(JsonDocument &doc); 

  /**
   * Completely clears m_buffer by setting each array index to '\0'.
   */
  void m_clear();

  /**
   * Serializes the JSON document full of Property, diganostic, and GNSS data
   * and stores it in m_buffer. m_buffer will later be sent over COMcontroller to the
   * base station.
   * 
   * @param doc JSON document containing all relevent data collected by the rover.
   */
  void m_serializePkt(JsonDocument &doc);

public:

  /**
   * Constructor for the SSModel() class. Initializes property data
   * and sets all Variables in SSModel() to 0.
   */
  SSModel();

  /**
   * A getter function that obtains a single proeprty from Property() class.
   * 
   * @param prop index of property to be obtained.
   * @return Data stored at index of property data.
   */
  int getProp(int prop);

  /**
   * A setter function that sets the property value at a given index.
   * 
   * @param prop The index of the property array to edit.
   * @param val the new value to be assigned to the prop.
   */
  void setProp(int prop, int val);



  //The following functions are used to copy GNSS data from structs created in the navigation.h file for GNSS.
  /**
   * Copies over the data in the struct collected by GNSS to the
   * equivalent struct contained in SSModel. 
   * 
   * @param pos_llh struct containing lat, long, and height GNSS data
   */
  void setPos_llh(msg_pos_llh_t pos_llh);

  /**
   * Copies over the data in the struct collected by GNSS to the
   * equivalent struct contained in SSModel.
   * 
   * @param baseline_ned struct containing lat, long, and height GNSS data
   */
  void setBaseline_ned(msg_baseline_ned_t baseline_ned);
  void setMsg_vel_ned_t(msg_vel_ned_t vel_ned);
  void setMsg_dops_t(msg_dops_t dops);
  void setMsg_gps_time_t(msg_gps_time_t gps_time);
  void setMode(uint8_t mode);

  // diagnostic
  void setDroppedPkts(uint16_t dropped_pkts);
  void setIMUflag(bool imu_flag);
  void setBat(float bat);
  void setSpace(uint32_t space);
  void setCycles(uint16_t cycles);
  void setError(const char *err);
  void setConv(float conv);

  // handles response data from the base station COMController::request()
  void handleRes(char *buf);

  // creates a char[] of data relevant to the sorted packet type
  char *toDiag();
  char *toProp();
  char *toData(int threshold);
  char *toError();

  void print();
  void clear();
};

#endif // _SSMODEL_H_

// /******** Data ********/
// #define FIX_MODE 0
// #define GPS_TIME_WN 1
// #define GPS_TIME_TOW 2
// #define POS_LLH_LAT 3
// #define POS_LLH_LON 4
// #define POS_LLH_HEIGHT 5
// #define POS_LLH_N_SATS 6
// #define BASELINE_N 7
// #define BASELINE_E 8
// #define BASELINE_D 9
// #define VEL_N 10
// #define VEL_E 11
// #define VEL_D 12
// #define DOPS_GDOP 13
// #define DOPS_HDOP 14
// #define DOPS_PDOP 15
// #define DOPS_TDOP 16
// #define DOPS_VDOP 17