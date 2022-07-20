#ifndef _SSMODEL_H_
#define _SSMODEL_H_

#include <Arduino.h>
#include <ArduinoJson.h>
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
   * Copies over the data in the struct collected in GNSSController to the 
   * equivalent struct contained in SSModel. 
   * 
   * @param pos_llh struct containing lat, long, and height GNSS data
   */
  void setPos_llh(msg_pos_llh_t pos_llh);

  /**
   * Copies over the data in the struct collected by GNSS to the 
   * equivalent struct contained in SSModel.
   * 
   * @param baseline_ned struct containing baseline N-E-D coordinates
   */
  void setBaseline_ned(msg_baseline_ned_t baseline_ned);

    /**
   * Copies over the data in the struct collected by GNSS to the 
   * equivalent struct contained in SSModel.
   * 
   * @param vel_ned struct containing velocity of North East Down coordinates.
   */
  void setMsg_vel_ned_t(msg_vel_ned_t vel_ned);

   /**
   * Copies over the data in the struct collected by GNSS to the 
   * equivalent struct contained in SSModel.
   * 
   * @param dops struct containing various DOPs from the GNSS data. 
   * The lower these values, the more accurate.
   */
  void setMsg_dops_t(msg_dops_t dops);

   /**
   * Copies over the data in the struct collected by GNSS to the 
   * equivalent struct contained in SSModel.
   * 
   * @param gps_time struct containing GPS time of week & week number.
   */
  void setMsg_gps_time_t(msg_gps_time_t gps_time);

   /**
   * Sets the mode of the last polled GNSS data (meaning RTK value). A value of 
   * 4 indicates tahn an RTK Fix was acheieved. If so, then the convergence timer will 
   * stop and calculate the running average in the diagnostic.
   * 
   * @param mode Best RTK mode set in here from polled data.
   */
  void setMode(uint8_t mode);

  // diagnostic
  /**
   * Setter function for setting the diagnostic "DroppedPkts" in Diagnostics.h file. 
   * Dropped packets are accrued from failed Handshakes/Uploads in COMController.
   * 
   * @param dropped_pkts the total # of dropped packets since the system has been on.
   */
  void setDroppedPkts(uint16_t dropped_pkts);

  /**
   * Setter function for setting the IMU flag in Diagnostics.h. This flag is set to True 
   * if the system woke up due to accelerometer interrupt, false otherwise.
   * 
   * @param imu_flag Boolean flag resulting in True if the system woke due to accelerometer interrupt. 
   */
  void setIMUflag(bool imu_flag);

  /**
   * A setter function for the battery voltage diagnostic.
   * 
   * @param bat battery voltage read from potentiometer.
   */
  void setBat(float bat);

  /**
   * Sets the remaining space of the SD card in Diagnostics.h.
   * 
   * @param space Current space, in MB, left on the SD card.
   */
  void setSpace(uint32_t space);

  /**
   * Sets the number of wake cycles the system's had in Diagnostics.h.
   * 
   * @param cycles Total # of wake cycles since the system has been on.
   */
  void setCycles(uint16_t cycles);

  /**
   * Sets the diagnostic denoting the number of errors the system has ran into. Also, 
   * stores the desc. of the error into the m_err buffer.
   * 
   * @param err string containg error message.
   */
  void setError(const char *err);

  /**
   * Sets the running average convergence time of obtaining RTK fixes in the convergence 
   * time diangostic.
   * 
   * @param conv The running average converge time for RTK fixes.
   * 
   */
  void setConv(float conv);

  /**
   * handles response data from the base station COMController::request().
   * 
   * @param buf String recieved from the base.
   */
  void handleRes(char *buf);

  /** 
   * creates a char[] of Diagnostic data
   * 
   * @return An array of Diagnostic data
   */
  char *toDiag();

  /** 
   * creates a char[] of property data
   * 
   * @return An array of Property data
   */
  char *toProp();

  /** 
   * creates a char[] of GNSS data. if m_mode > threshold, 
   * the packet will be returned, else null will be returned.
   * 
   * @param threshold minimum required RTK value for data to be sent.
   * @return An array of GNSS data
   */
  char *toData(int threshold);

  /**
   * Returns the last error message that was contained in char* m_err.
   * 
   * @return The last error message contained in m_err
   */
  char *toError();

  /**
   * Prints out all of the Diagnostic data, Property data, and GNSS data to the 
   * Serial monitor. 
   */
  void print();

  /**
   * Clears the states of Diagnostic/Preoprty data as well as 
   * the state of GNSS data (contents from char* toData() function).
   */
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