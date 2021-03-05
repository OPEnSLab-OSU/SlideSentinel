#ifndef _COMCONTROLLER_H_
#define _COMCONTROLLER_H_

#define REQ 1
#define UPL 2
#define RES 3

#include <Arduino.h>
#include "Controller.h"
#include "FreewaveRadio.h"
#include "HardwareSerial.h"
#include "MAX3243.h"
#include "RHReliableDatagram.h"
#include "RH_Serial.h"
#include "SN74LVC2G53.h"
#include "SSInterface.h"
#include "Timer.h"

/**
 * COMController class is used for communication with the base station via Freewave radio. The functions in here 
 * are used for requesting data from the base & uploading to the base. Radio communication takes part in many 
 * classes, this is the first level, then SSInterface is used for sending/receiving packets from the base (which 
 * uses RHReliableDatagram library for complicated comm functionality). 
 */
class COMController : public Controller {
private:
  SSInterface m_interface;  ///< SSInterface class deals with the actual sending/receiving of bytes.
  Freewave &m_radio;        ///< Freewave radio class, used for specifying the radio type and resetting the Freewave radio.
  MAX3243 &m_max3243;       ///< Used for enabling/disabling RS232 data line for sending data over serial.
  SN74LVC2G53 &m_mux;       ///< mux used for switching RS232 data line between The Piksi and Freewave Radio.
  
  HardwareSerial &m_serial; ///< Used for creating serial communication.
  Timer m_timer;            ///< Declaring a Timer object. Timer is only used in channelBusy(), which is used in the test script only. 

  int m_dropped_pkts;       ///< diagnostic for # of dropped packets
  uint8_t m_threshold;      ///< Threshold property determines whether packet will be sent, if its RTK mode is >= threshold.
  char m_buf[MAX_DATA_LEN]; ///< Buffer that contains the data to be sent via Freewave radio to the base.

  /**
   * Clears m_buf, sets all chars to NULL chars. Important to 
   * call before setting new data in the buffer.
   */
  void m_clearBuffer();

  /**
   * Setter function for the timeout property. Sets the property in the SSInterface class.
   * @param timeout the timeout property (How long before communication fails)
   */
  void m_setTimeout(uint16_t timeout);

  /**
   * Setter function for the retries property. Sets the property in the SSInterface class.
   * @param retries the retries property (How many attempts before communication gives up?)
   */
  void m_setRetries(uint16_t retries);

  /**
   * Setter function for the threshold property. Determines the minimum RTK value needed 
   * for a GNSS packet to be sent, otherwise, 'null' is sent to the base for the GNSS data 
   * section of the packet. 
   * @param threshold Threshold prop to be set in this class.
   */
  void m_setThreshold(uint8_t threshold);

  /**
   * Increments the # of dropped paackets (DIAGNOSTIC). This is called if a piece of data failed to send 
   * via radio communication between the base/rover.
   */
  void m_droppedPkt();

public:

/**
 * Class constructor for COMController. This passes in variables to 
 * set up the mux, Serial between radio and Feather, client/server ids, 
 * timeout & retires props, and also calls the constructor for the m_interface 
 * class. 
 * @param radio reference to Freewave radio object. 
 * @param max3243 reference to max3243 class, an RS232 line driver.
 * @param baud radio baud rate for communication with base
 * @param clientId ID of this specific rover (unique for each rover in network), essential for radio comm.
 * @param serverId ID of the base station (Always 0).
 * @param timeout The timeout property. Determines how long communication attempt should take before retrying.
 * @param retries Retries property. Determines how mnay retries radios should try before giving up.
 */ 
  COMController(Freewave &radio, MAX3243 &max3243, SN74LVC2G53 &mux,
                HardwareSerial &serial, uint32_t baud, uint8_t clientId,
                uint8_t serverId, uint16_t timeout, uint8_t retries);
  
  /**
   * This function is called during the HANDSHAKE state in main(). This attempts 
   * to make a request, sending the diagnostic data to the base station, and then
   * the base station sends back property data. 
   * 
   * 1. Mux is flipped so serial data goes from the Radio -> feather.
   * 2. Rover sends a REQ signal to base as well as current system diagnostics, asking for it to send back props.
   * If the base station is already servicing another rover in the network, then this function 
   * fails and returns false. If fails, dropped_pkts diag is incremented. 
   * 3. If successful, then the rover recieves the packet and puts in in m_buf. 
   * 4. The mux is then flipped so that data goes from Radio -> GNSS (preparing for 
   * the POLL phase). This allows RTK correctional data to go to the piksi.
   * 5. The properties stored in buffer are handled by handleRes() of SSModel, which updates 
   * the properties (if any changes props were obtained).
   * 
   * @param model reference to SSModel class, necessary for updating properties obtained from base
   * @return Bool: true if request was successful, false if comm failed (Base could be servicing another rover).
   */
  bool request(SSModel &model);

  /**
   * This function uploads the GNSS data packet to the base station after the POLLing phase is complete. 
   * 
   * 1. m_buff is cleared,then the mux is flipped so data goes from the Radio -> Feather. 
   * 2. Attempts to send GNSS data to base station. It sends 'null' if the data's RTK mode 
   * was less than the threshold property. If comm fails then dropped packets diag is incremented. 
   * 
   * @param model reference to SSModel class. The data packet to be sent is contained here.
   * @return Bool: True if upload was successful, false if it failed.
   */
  bool upload(SSModel &model);

  /**
   * The init function for the COMController. This initializes the SSInterface class, 
   * disables the MAX3243 RS232 data line, sets the mux so data goes from Radio -> Feather. 
   * @return Bool: True if successful.
   */
  bool init();

  /**
   * Status function for the COMController. Updates the properties TIMEOUT, RETRIES, and 
   * THRESHOLD in SSModel with the current values in this class. Also, updates the diagostic 
   * m_dropped_pkts with the current running total # dropped packets since the system has been on. 
   * 
   * @param model reference to SSModel class, necessary to call setters for updating values.
   */
  void status(SSModel &model);

  /**
   * Updates the properties TIMEOUT, RETRIES, and THRESHOLD in this class to the values contained in SSModel. 
   * @param model reference to SSModel class, so that the props can be obtained and set in the COMController object.
   */
  void update(SSModel &model);

  /**
   * Resets the Radio.
   */
  void resetRadio();

  /**
   * This function is called if the radio channel is busy. This function 
   * is only called by the ManualTest script. 
   * 
   * @param model reference to SSModel class, used to set an error.
   * @return Bool: True if the channel is busy, false if it is available.
   */
  bool channelBusy(SSModel &model);
};

#endif // _COMCONTROLLER_H_