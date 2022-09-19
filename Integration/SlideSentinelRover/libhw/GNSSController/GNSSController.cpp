#include "GNSSController.h"
#include "SwiftController.h"
#include "Arduino.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
GNSSController::GNSSController(
      HardwareSerial& serial, 
      uint32_t baud, 
      uint8_t rx, 
      uint8_t tx, 
      int logFreq) 
      : m_serial(serial), m_baudRate(baud), m_rx(rx), m_tx(tx), m_logFreq(logFreq) {}
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
bool GNSSController::init(){

  // Start the serial interface
  m_serial.begin(m_baudRate);

  // Allow us to use more serial interfaces than the board actually has
  pinPeripheral(m_tx, PIO_SERCOM);
  pinPeripheral(m_rx, PIO_SERCOM);
  MARK;

  // SBP Setup
  sbp_setup();
  console.debug("GNSSController initialized.\n");
  return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * -------------- RTK --------------
 * The msg_pos_llh_t struct contains an 8-bit, six digit flag member
 * variable. This variable, the 3 least significant bits
 * ---------------------------------
 * | 0 | Invalid     |
 * | 1 | SPP         |
 * | 2 | DGNSS       |
 * | 3 | Float RTK   |
 * | 4 | Fixed RTK   |
 * | 5 | Dead Reckon |
 * | 6 | SBAS        |
 * ---------------------------------
 *
 * -------------- ACCURACY ---------
 * The lower these values the better,
 * gdop is the most holistic measurement
 * for accuracy
 *
 * GDOP (latitude, longitude, height, clock)
 * PDOP (latitude, longitude, height)
 * HDOP (latitude, longitude, height)
 *
 * -------------- RETURN ----------
 * 0 - no data collected
 * 1 - data collected
 * 2 - data collected, quality RTK fix reached, terminate polling to save power
 * 
 * NOTE: MUST BE CALLED AS FREQUENTLY AS POSSIBLE (<15ms), see github issue for more info
 */
uint8_t GNSSController::poll(){
  // Read data from the GNSS module
  m_readGNSS();

  // Process the GNSS data and return the error code
  s8 ret = sbp_process(&sbp_state, fifo_read);

  // If we didn't have a clean return code we should know
  if(ret != 0){
   //console.debug(String("Error occurred when processing GNSS Data : " + String(ret) + "\n").c_str());
  }

  // Updates local variables from GNSS data
  m_updateGNSSInformation();

  // TODO: Implement return codes
  return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////////////////////////
void GNSSController::reset(){
  m_resetStructs();
  m_updateGNSSInformation();
}
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
String GNSSController::getRTKModeString(){
  switch(m_rtkMode){
    case 0:
      return "Invalid";
    case 1:
      return "SPP";
    case 2:
      return "DGNSS";
    case 3:
      return "Float RTK";
    case 4:
      return "Fixed RTK";
    case 5:
      return "Dead Reckoning";
    case 6:
      return "SBAS";
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * 
 * Populates the Rover message response
 * Message JSON Format:
 * Message{
 *  GNSS{
 *    RTK Mode  :
 *    Week :
 *    Seconds :
 *    Latitude :
 *    Longitude :
 *    Height :
 *    Satellites :
 *    GDOP :
 *    HDOP :
 *    PDOP :
 *    TDOP :
 *    VDOP :
 *  }
 * }
 * 
 * @author Will Richards
 * @param msgJson Reference to the message key as a json object
 */ 
void GNSSController::populateGNSSMessage() {
  JsonObject msgJson;
  JsonObject json = msgJson.createNestedObject("GNSS");
  json["RTK Mode"] = getRTKModeString();
  json["Week"] = m_gps_time.wn;
  json["Seconds"] = m_gps_time.tow;
  json["Latitude"] = (m_gpsPos.lat);
  json["Longitude"] = String(m_gpsPos.lon);
  json["Height"] = String(m_gpsPos.height);
  json["Satellites"] = m_gpsPos.n_sats;
  json["GDOP"] = m_dataPecision.gdop;
  json["HDOP"] = m_dataPecision.hdop;
  json["PDOP"] = m_dataPecision.pdop;
  json["TDOP"] = m_dataPecision.tdop;
  json["VDOP"] = m_dataPecision.vdop;
}

bool GNSSController::isNewData(){
  return (m_gpsPos.lat!=0);
}

void GNSSController::populateGNSS() {
  doc.clear();
  doc["RTK Mode"] = getRTKModeString();
  doc["Week"] = String(m_gps_time.wn);
  doc["Seconds"] = String(m_gps_time.tow);

  //Critical info 
  doc["Latitude"] =  String(m_gpsPos.lat,17);
  doc["Longitude"] =String(m_gpsPos.lon,17);
  doc["Height"] = String(m_gpsPos.height,17);
  m_resetStructs();

  /*
    Stores every GNSS data points in their respective array
    using a dictionary/JSON.

    "GNSS" = {
      "Seconds" = 11,
      "Latitude" = 11,
      "Longitude" = 11, 
      "Height" = 11,
      .
      .
      .
    }
  */
}


//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t GNSSController::m_getRTKMode(){
  return pos_llh.flags & FIX_MODE_MASK;
}
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
void GNSSController::m_readGNSS(){
  if(m_serial.available()){
    fifo_write(m_serial.read());
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
bool GNSSController::m_compareFixMode(){
    // If we have a better fix than the current one we shouldn't update
    return !(m_rtkMode > m_getRTKMode() && (m_getRTKMode() != 6));
}
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
void GNSSController::m_updateGNSSInformation(){
 // Serial.println("update gnss info called, sample: " + String(pos_llh.lat) + "");
  m_gpsPos = pos_llh;
  m_rtkBaseline = baseline_ned;
  m_velocity = vel_ned;
  m_dataPecision = dops;
  m_gps_time = gps_time;
  m_rtkMode = m_getRTKMode();
}
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
void GNSSController::m_resetStructs(){
  gps_time.wn = 0;
  gps_time.tow = 0;
  pos_llh.flags &= FIX_MODE_CLR;
  pos_llh.lat = 0;
  pos_llh.lon = 0;
  pos_llh.height = 0;
  pos_llh.n_sats = 0;
  baseline_ned.n = 0;
  baseline_ned.e = 0;
  baseline_ned.d = 0;
  vel_ned.n = 0;
  vel_ned.e = 0;
  vel_ned.d = 0;
  dops.gdop = 0;
  dops.hdop = 0;
  dops.pdop = 0;
  dops.tdop = 0;
  dops.vdop = 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
StaticJsonDocument<MAX_DATA_LEN> GNSSController::getGNSSData(){
  return doc;
}
//////////////////////////////////////////////////////////////////////////////////////////////////

