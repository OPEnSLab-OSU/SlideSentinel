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
 */
uint8_t GNSSController::poll(){
  // Read data from the GNSS module
  m_readGNSS();

  // Process the GNSS data and return the error code
  s8 ret = sbp_process(&sbp_state, fifo_read);

  // If we didn't have a clean return code we should know
  if(ret != 0){
    console.debug(String("Error occurred when processing GNSS Data : " + String(ret) + "\n").c_str());
  }

  // Update the data per the logging frequency
  DO_EVERY(m_logFreq, if(m_compareFixMode()) m_updateGNSSInformation());

  // TODO: Implement return codes
  return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
void GNSSController::update(SSModel &model){
  m_logFreq = model.getProp(LOG_FREQ);
}
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
void GNSSController::status(SSModel &model){
  model.setPos_llh(m_gpsPos);
  model.setBaseline_ned(m_rtkBaseline);
  model.setMsg_vel_ned_t(m_velocity);
  model.setMsg_dops_t(m_dataPecision);
  model.setMsg_gps_time_t(m_gps_time);
  model.setMode(m_rtkMode);
  model.setProp(LOG_FREQ, m_logFreq);
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
  json["Latitude"] = String(m_gpsPos.lat);
  json["Longitude"] = String(m_gpsPos.lon);
  json["Height"] = String(m_gpsPos.height);
  json["Satellites"] = m_gpsPos.n_sats;
  json["GDOP"] = m_dataPecision.gdop;
  json["HDOP"] = m_dataPecision.hdop;
  json["PDOP"] = m_dataPecision.pdop;
  json["TDOP"] = m_dataPecision.tdop;
  json["VDOP"] = m_dataPecision.vdop;
}

void GNSSController::populateGNSSMessage_Ben(JsonDocument &test) {
  //StaticJsonDocument<MAX_DATA_LEN> doc = test.createNestedObject("GNSS");
  //doc.createNestedObject("GNSS");
  // doc["RTK Mode"] = getRTKModeString();
  // doc["Week"] = m_gps_time.wn;
  // doc["Seconds"] = 11;
  // doc["Latitude"] = String(22.7);
  // doc["Longitude"] = String(4.6);
  // doc["Height"] = String(43.8);
  // doc["Satellites"] = m_gpsPos.n_sats;
  // doc["GDOP"] = m_dataPecision.gdop;
  // doc["HDOP"] = m_dataPecision.hdop;
  // doc["PDOP"] = m_dataPecision.pdop;
  // doc["TDOP"] = m_dataPecision.tdop;
  // doc["VDOP"] = m_dataPecision.vdop;

  //size_t temp = serializeJson(doc, Serial);
  
  // return temp;
  JsonArray data = test.to<JsonArray>();
  // data.add(m_mode);
  // data.add(m_gps_time.wn);
  // data.add(11);
  data.add(String(22.7));
  data.add(String(4.6));
  data.add(String(4.6));
  data.add(m_gpsPos.n_sats);

}

char *GNSSController::populateGNSS() {
  StaticJsonDocument<MAX_DATA_LEN> doc;
  doc["GNSS"]["RTK Mode"] = getRTKModeString();
  doc["GNSS"]["Week"] = m_gps_time.wn;
  doc["GNSS"]["Seconds"] = 11;
  doc["GNSS"]["Latitude"] = 22.7;
  doc["GNSS"]["Height"] = 4.6;
  doc["GNSS"]["Longitude"] = 43.8;
  doc["GNSS"]["Satellites"] = m_gpsPos.n_sats;
  doc["GNSS"]["GDOP"] = m_dataPecision.gdop;
  doc["GNSS"]["HDOP"] = m_dataPecision.hdop;
  doc["GNSS"]["PDOP"] = m_dataPecision.pdop;
  doc["GNSS"]["TDOP"] = m_dataPecision.tdop;
  doc["GNSS"]["VDOP"] = m_dataPecision.vdop; 

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

  //serializeJson(doc, Serial);
  String temp = (doc.as<JsonObject>())["GNSS"];
  return (char *)temp.c_str();
}

// char *GNSSController::getFormat() {
//   return (char*)populateGNSS_return();
// }


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


