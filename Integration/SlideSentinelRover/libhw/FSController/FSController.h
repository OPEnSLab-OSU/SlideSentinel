#ifndef _FSCONTROLLER_H_
#define _FSCONTROLLER_H_

#include <Arduino.h>
#include <string.h>
#include "Controller.h"
#include "SdFat.h"

#define MAIN "data" //< The name of the root directory on the SD card.
#define SD_SIZE 16 //< Size in GB of SD card.

// TODO consider deep copying over the timestamp so we dont depend on a string
// not within the FSController class

/**
  * This class contains all functionality for logging 
  * to the SD card on the PCB. It uses the SDFat external library 
  * to complete most of the functionality.
  */
class FSController : public Controller {
private:
  SdFat m_sd;       ///< Provides functions for SD manipulation.
  SdFile m_file;  
  SdFile m_root;
  uint8_t m_cs;     ///< pin on feather
  uint8_t m_rst;    ///< pin on feather
  bool m_did_begin; ///< flag for if SD initialized

  const char *m_DATA; ///< String of GPS data collected
  const char *m_DIAG; ///< String of Diagnostic information.
  char *m_curDir;  ///< current dir in the SD

  // TODO add error count diagnostic info
  float m_spaceMB; // diagnostic
  int m_cycle;     // diagnostic

  /** 
  * This function increments the variable m_cycles 
  * by +1. This function is called every wake cycle. 
  * This var is used as the diagnostic var for wake cycles.
  */
  void m_cycles();

  /**
  * This function calculates the free MB of space 
  * on the SD card. Value is stored in m_spaceMB, 
  * which is used as a diagnostic variable.
  */
  void m_SDspace();

  /**
  * This function writes a message to a file. 
  * @param msg The message to be written to the file
  * @param file the name of the file to write to
  * @return True for success, false if failed
  */
  bool m_logMsg(const char *msg, const char *file);

  /**
  * prints a message to the currently open 
  * file. This function is called from m_logmsg().
  * @param msg the message to be written to the file
  * @return boolean denoting success of operation.
  */
  bool m_write(char *msg);

  /**
  * Makes a new file if it does not already 
  * exist. 
  * @param name The name of the file to create
  * @return True if file created, false if file already exists.
  */
  bool m_mkFile(const char *name);

  /**
  * Sets the current open file to the file specified
  * in the parameter.
  * @param file the name of the file to open
  * @return true if opened, false if failed
  */
  bool m_setFile(const char *name);

public:

  /**
  * Class constructor for the File System Controller.
  * This constructor takes in two parameters, which are the 
  * cs pin and the rst pin for controlling the SD card.
  */
  FSController(uint8_t cs, uint8_t rst);

  /**
  * Calls m_logMsg with parameters that will 
  * log the most recent GNSS data to the current 
  * wake cycle file on the SD card.
  * @param data GNSS data to be written to the SD card.
  */
  void logData(char *data);

  /**
  * Calls m_logMsg with parameters that will log 
  * the most recent diagnostic packet to the current 
  * wake cycle file.
  * @param data Diagnostic serialized packet
  */
  void logDiag(char *data);

  /**
  * This is called at the beginning of every wake cycle. 
  * It creates a drectory for the current wake cycle with the 
  * current timestamp as the directory name. Two files are created 
  * inside this directory: DATA.json and DIAG.json, which are populated 
  * with data as the system collects data.
  * @param timestamp Current timestamp, used as name of new dir.
  * @param format A string written to the top of DATA.json which labels the items in the packets.
  */
  bool setupWakeCycle(char *timestamp, char *format);

  /**
  * Attempts to initialize the SD card. The success of this 
  * function depends on if the SD card has a good connection with the 
  * SD card holder on the PCB. This function is essential for writing 
  * to the SD card. If it fails, then no SD functionality will work, but 
  * the system will still run.
  * @return bolean flag: true for success, else false.
  */
  bool init();

  /**
  * Checks whether the SD card intialized. This is used to 
  * disable function calls to SD logging functionality throughout 
  * the program by checking if the SD card initialized. If not,
  * the function call is skipped.
  * @return bool denoting whether the SD is initialized or not.
  */
  bool check_init();

  /**
  * Status function for the FSController. This function call 
  * updates the Diagnostic values SD_Space & # Wake_Cycles with the values
  * from this class, m_SDspace & m_cycle.
  * 
  */
  void status(SSModel &model);

  /**
  * Update function for FSController. For this class, there is nothing 
  * to update, but all Controller classes have an update function so the 
  * that COM_Manager can call Update easily.
  */
  void update(SSModel &model);
};

#endif // _FSCONTROLLER_H_
