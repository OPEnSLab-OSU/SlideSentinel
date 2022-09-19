#ifndef _GNSSCONTROLLER_H_
#define _GNSSCONTROLLER_H_

#include <Arduino.h>
#include "wiring_private.h" // Pin peripheral
#include "HardwareSerial.h"

#define FIX_MODE_MASK 0b00000111
#define FIX_MODE_CLR 0b11111000

//#include "SwiftController.h"



#include "Console.h"
#include "FeatherTrace.h"
#include "Controller.h"
#include "Properties.h"

#include "Timer.h"

  /**
   * GNSSController Class: 
   *
   * This class controls all communication between
   * the Swift Piksi Multi and the feather. It tells
   * the Piksi when to poll, and reads the data receievd
   * by the Piksi.
   */
class GNSSController {
  public:

    /**
     * Construct a new GNSS Controller
     * 
     * @param serial Hardware serial to communicate with the GNSS module over
     * @param baud Serial baud rate to communicate at
     * @param rx RX Pin
     * @param tx TX Pin
     * @param logFreq Frequency at which data is logged
     */ 
    GNSSController(
      HardwareSerial& serial, 
      uint32_t baud = 115200, 
      uint8_t rx = 12, 
      uint8_t tx = 11, 
      int logFreq = 30);

    /**
     * Initialize the GNSS module
     */ 
    bool init();

    /**
     * Poll GNSS Module for updated date
     */ 
    uint8_t poll();

    /**
     * Reset the stored GNSS data
     */ 
    void reset();

    /**
     * Get the string matching the RTK mode
     */ 
    String getRTKModeString();

    /**
    * Populates a json object with the desired GNSS data
    * 
    * @author Will Richards
    */ 
    //void populateGNSSMessage(JsonObject msgJson);
    void populateGNSSMessage();

    void populateGNSSMessage_Ben(JsonDocument &doc);
    String populateGNSS();

    /* Replacing SSModel */
    char *toData(int);
    void m_serializePkt(JsonDocument &doc);
    void m_clear();
    int getProp(int);
    char temp[512];




    /**
     * Get latitude position
     */ 
    double getLatitude() { return m_gpsPos.lat; };

    /**
     * Get longitude position
     */ 
    double getLongitude() { return m_gpsPos.lon; };
  
    /**
     * Get height
     */ 
    double getHeight() { return m_gpsPos.height; };

    //returns true if new data is recorded (gets wiped when getchar is called)
    bool isNewData();
    
    StaticJsonDocument<MAX_DATA_LEN> doc;
   

  private:

    /* Serial Communication */
    HardwareSerial& m_serial;             // Serial interface to communicate with the GNSS module

    uint32_t m_baudRate;                  // Serial baud rate to communicate at
    uint8_t m_rx;                         // RX Pin to communicate over
    uint8_t m_tx;                         // TX Pin to communicate over
    int m_logFreq;                        // Log Frequency

    /* GNSS Data */
    
    char m_buffer[MAX_DATA_LEN];
    msg_pos_llh_t m_gpsPos;               // This position solution message reports the absolute geodetic coordinates and the status
    msg_vel_ned_t m_velocity;             // Velocity of the rover in North, East, Down (NED) Vector coordinates.
    msg_baseline_ned_t m_rtkBaseline;     // This baseline is the relative vector distance from the base station to the rover receiver
    msg_dops_t m_dataPecision;            // This dilution of precision (DOP) message describes the effect of navigation satellite geometry on positional measurement precision.
    msg_gps_time_t m_gps_time;            // This message reports the GPS time, representing the time since the GPS epoch began

    uint8_t m_rtkMode;                    // Current RTK fix of the GNSS Controller

    uint8_t m_getRTKMode();               // Get the current RTK fix mode
    void m_readGNSS();                    // Read the serial data from the GNSS module
    bool m_compareFixMode();              // Determine if we have a better fix mod than last time

    void m_resetStructs();                // Reset the GNSS Structs
    
    void m_updateGNSSInformation();       // Update variables that store GNSS related data

    Properties m_props; ///< Object stores all property data.
    uint8_t m_mode;                    ///< data

    
};

#endif // _GNSSCONTROLLER_H_
