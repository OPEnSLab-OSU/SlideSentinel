#ifndef _DIAGNOSTICS_H_
#define _DIAGNOSTICS_H_

#include <Arduino.h>
#include "ArduinoJson.h"

/**
 * Declaring Macros for indexing the 
 * array of Diagnostic data. This shows 
 * what each index's value corresponds to.
 */
#define SS_DIAG "DIAG"
#define MAX_DIAG_LEN 200
#define IMU_FLAG 0
#define BAT 1
#define SPACE 2
#define CYCLES 3
#define DROPPED_PKTS 4
#define ERR_COUNT 5
#define CONV_TIME 6


/**
 * Diagnostics Class. This class holds 
 * the current system Diagnostics. These diagnostics 
 * are sent to the base station during the HANDSHAKE phase.
 */
class Diagnostics {
private:
  bool m_imu_flag; ///< Bool: Did system wake due to accelerometer?
  float m_bat;     ///< Current battery voltage of rover.
  float m_space;   ///< Free SD Space (In MB).
  int m_cycles;    ///< Total Number of wake cycles.
  int m_dropped_pkts; ///< Total # dropped packets.
  int m_err_count;    ///< Total # of errors.
  float m_convergence_time; ///< Running avg. time it takes to reach RTK fix.

public:
  /**
   * Class constructor for Diagnostics. This constructor
   * takes no parameters, and simply calls clear(), which 
   * initializes all diagnostics to 0.
   */
  Diagnostics();

  /**
   * Creates a serialized data packet in the form of 
   * an array of diagnostic data. Each data index 
   * corresponds to the name of the macro defined at the 
   * top of this file. Serializing the data is necessary 
   * to send the packet over radio to the base.
   * @param doc JSON document necessary to format diag packet
   */
  void write(JsonDocument &doc);

  /**
   * Reads the values of the serialized JSON packet 
   * of diagnostic data. It sets the member variables of the 
   * this class with these values. 
   * @param buf Serialized JSON array of diag data.
   */
  void read(char *buf);

  // getters

  /**
   * Gets the current status of the imu flag
   * @return True if system woke from accelerometer, else false.
   */
  bool imu();

  /**
   * Gets current battery voltage of system.
   * @return Battery voltage of rover.
   */
  float bat();

  /**
   * Gets current free SD space of system.
   * @return Number of free MB in SD card
   */
  float space();

  /**
   * Gets the number of wake cycles the system had 
   * since it's been turned on.
   * @return number of wake cycles.
   */
  int cycles();

  /**
   * Gets the number of dropped packets as a result of 
   * bad communication between the rover and base 
   * (Can happen during HANDSHAKE or UPLOAD).
   * @return number of dropped packets during communication.
   */
  int droppedPkts();

  /**
   * Gets the number of errors the system has encountered. 
   * Gererally, this number is the same as the droppedPkts 
   * diagnostic. 
   * @return number of system errors.
   */
  int errCount();

  /**
   * Gets the current running average convergence time. 
   * This means the average time the system took to reach 
   * an RTK fix on successful polling cycles.
   */
  float convergenceTime();

  // setters

  /**
   * Sets the diagnostic member variable m_imu_flag 
   * with whether the system woke from accelerometer or not.
   * @param flag state of imu flag from current wake cycle.
   */
  void setFlag(bool flag);

  /**
   * Sets the diagnostic member var m_bat with the  
   * current battery voltage read by the voltage reader.
   * NOTE: See Battery.h
   * @param bat current battery voltage read by system
   */
  void setBat(float bat);

  /**
   * Sets the current free space left in the SD card.
   * This function is called from the FSController Status() 
   * function. 
   * @param space free MB left in SD card 
   */
  void setSpace(float space);

  /**
   * Sets the current # of wae cycles the system has had. 
   * This is obtained from the Status() func FSController.
   * @param cycles Running total # of wake cycles
   */
  void setCycles(int cycles);

  /**
   * Sets the current number of dropped packets the 
   * system has had since it has been on.
   * @param drop Total # of dropped packets.
   */
  void setDroppedPkts(int drop);

  /**
   * Sets the total number of errors the system has 
   * had since it's been turned on. 
   * @param errCount total number of system errors
   */
  void setErrCount(int errCount);

  /**
   * Sets the average convergence time of getting 
   * RTK fixes. This is obtained from the GNSSController.
   * @param conv average RTK Fix convergence time.
   */
  void setConvergenceTime(float conv);
  
  /**
   * Sets all of the diagnostics to 0.
   */
  void clear();

  /**
   * Prints the current diagnostic values to 
   * the Serial Monitor.
   */
  void print();
};

#endif // _DIAGNOSTICS_H_
