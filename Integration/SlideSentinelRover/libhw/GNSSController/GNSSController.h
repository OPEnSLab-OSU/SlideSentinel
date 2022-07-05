#ifndef _GNSSCONTROLLER_H_
#define _GNSSCONTROLLER_H_
#define FIX_MODE_MASK 0b00000111
#define FIX_MODE_CLR 0b11111000

#include <Arduino.h>
#include "Controller.h"
#include "HardwareSerial.h"
#include "SwiftPiksi.h"
#include "wiring_private.h" // Pin peripheral
#include "Timer.h"

  /**
   * GNSSController Class: 
   *
   * This class controls all communication between
   * the Swift Piksi Multi and the feather. It tells
   * the Piksi when to poll, and reads the data receievd
   * by the Piksi.
   */
class GNSSController : public Controller {

private:
  HardwareSerial &m_serial; ///< Serial Communication to Piksi
  int m_baud;               ///< Baud rate of Serial comm
  uint8_t m_rx;             ///< rx pin on feather
  uint8_t m_tx;             ///< tx pin on feather 
  int m_logFreq;            ///< state
  const char *m_FORMAT;     ///< csv format string

  
  Timer m_timer; ///< used for collecting the convergence time
  float m_convergenceTime; ///< DIAG: time taken to get RTK fix
  bool m_convFlag;         ///< when RTK is reached, set true
  int m_pollCycles;        ///< DIAG: #poll cycles rover has had

  /**
   * Reads data sent from Piksi to the feather 
   * via Serial Communication. This contains the  
   * data collected by the Piksi
   */
  void m_GNSSread();

  /**
   * Getter function for the current best 
   * RTK mode collected by the Piksi.
   * @return Current RTK mode (1,2,3,4,6)
   */
  uint8_t m_getMode();

  /**
   * Determines the fix mode obtained by calling 
   * m_getMode(). Performs a switch statement on the mode 
   * to write the name of the mode as a str in rj.
   * @param pos_llh data struct
   * @param rj, char array to write name of RTK mode into
   */
  void m_getModeStr(msg_pos_llh_t pos_llh, char rj[]);

  /**
   * This function is called every polling cycle. It checks 
   * if the newly obtained fix mode is greater than the previous 
   * best. Best mode can be a 4 (RTK fix). There is logic to prevent 
   * a 6 (SBAS) from overwriting a 4 (Note that 6 is meter-level accurate).
   * @return boolean value: True if mode is better, else false
   */
  bool m_compare();

  /**
   * This function is called if m_compare() returns true. 
   * It updates the data structs with the new positional data 
   * collected and updates the RTK mode using m_getMode().
   */
  void m_setBest();

  /**
   * This function is called when an RTK fix is reached. 
   * It sets the value of datFlag boolean to True. 
   * NOTE: As it stands, this function is not used for anything. 
   * It can be used later to terminate polling cycle early if a Fix 
   * is reached. 
   * @param flag passes datFlag by reference and sets it to 1
   */
  void m_isFixed(uint8_t &flag);

  /**
   * Sets all of the variables in the GNSS data structs to 0.
   * This is called at the end of a wake cycle to ensure no garbage
   * values are left over in the next wake cycle.
   */
  void m_reset();

  /**
   * Sets the internal member variable m_logFreq to the log frequency 
   * specified by the Properties Class of the system.
   * NOTE: Log frequency determines how often a packet of positional 
   * data is collected from the Piksi. See DO_EVERY() in poll() function.
   * @param logFreq Current Log frequency in the Properties Class
   */
  void m_setLogFreq(int logFreq);
  
  // void sbp_pos_llh_callback(u16, u8, u8[], void*);
  // void sbp_baseline_ned_callback(u16, u8, u8[], void*);
  // void sbp_vel_ned_callback(u16, u8, u8[], void*);
  // void sbp_dops_callback(u16, u8, u8[], void*);
  // void sbp_gps_time_callback(u16, u8, u8[], void*);

  

public:
  msg_pos_llh_t m_pos_llh;
  msg_baseline_ned_t m_baseline_ned;
  msg_vel_ned_t m_vel_ned;
  msg_dops_t m_dops;
  msg_gps_time_t m_gps_time;
  uint8_t m_mode;
  GNSSController(HardwareSerial &serial, uint32_t baud, uint8_t rx, uint8_t tx,
                 int logFreq);

  /**
   * Intitializes the GNSSController class. Sets up Serial 
   * communication between piksi and feather.
   * @return Returns true. 
   */
  bool init();
  uint8_t poll(/*SSModel &model*/);
  char *getFormat();

  /**
   * The status() function of the GNSSController class. 
   * This updates the SSModel Positonal data structs with the 
   * current values stored in the equivalent structs of this class.
   * @param model SSModel object
   */
  void status(SSModel &model);

  /**
   * The update function of the GNSSController class. 
   * This updates the member variable m_logFreq with the 
   * Log frequency currently in the Properties class of the system 
   * by calling m_setLogFreq().
   * @param model SSModel object
   */
  void update(SSModel &model);

  /**
   * This function call the startStopwatch() function in the 
   * timer class. This is called before each POLL cycle (In main) 
   * to start a counter used for the RTK Convergence timer. The 
   * stopwatch terminates if an RTK mode is reached, and it adds the 
   * time to the current running average for m_convergenceTime. 
   */
  void setup();

  /**
   * This function is called at the end of each wake cycle. 
   * It flushes any remaining garbage values in the Serial, 
   * resets the data struct values to 0 as well as the structs 
   * for m_setBest(). Also stops the convergence timer (in case 
   * no RTK fix was reached to stop the timer otherwise).
   */
  void reset();

  /**
   * Not used. 
   *
   *
   */
  void startTimer();


};

#endif // _GNSSCONTROLLER_H_
