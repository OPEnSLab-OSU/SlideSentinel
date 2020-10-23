#include "GNSSController.h"
#include "Console.h"
#include "FeatherTrace.h"

/*
 * State of the SBP message parser.
 * Must be statically allocated.
 */
sbp_state_t sbp_state;

/* SBP structs that messages from Piksi will feed. */
msg_pos_llh_t pos_llh;
msg_baseline_ned_t baseline_ned;
msg_vel_ned_t vel_ned;
msg_dops_t dops;
msg_gps_time_t gps_time;

/*
 * SBP callback nodes must be statically allocated. Each message ID / callback
 * pair must have a unique sbp_msg_callbacks_node_t associated with it.
 */
sbp_msg_callbacks_node_t pos_llh_node;
sbp_msg_callbacks_node_t baseline_ned_node;
sbp_msg_callbacks_node_t vel_ned_node;
sbp_msg_callbacks_node_t dops_node;
sbp_msg_callbacks_node_t gps_time_node;

/*
 * Callback functions to interpret SBP messages.
 * Every message ID has a callback associated with it to
 * receive and interpret the message payload.
 */
void sbp_pos_llh_callback(u16 sender_id, u8 len, u8 msg[], void *context) {
  pos_llh = *(msg_pos_llh_t *)msg;
}
void sbp_baseline_ned_callback(u16 sender_id, u8 len, u8 msg[], void *context) {
  baseline_ned = *(msg_baseline_ned_t *)msg;
}
void sbp_vel_ned_callback(u16 sender_id, u8 len, u8 msg[], void *context) {
  vel_ned = *(msg_vel_ned_t *)msg;
}
void sbp_dops_callback(u16 sender_id, u8 len, u8 msg[], void *context) {
  dops = *(msg_dops_t *)msg;
}
void sbp_gps_time_callback(u16 sender_id, u8 len, u8 msg[], void *context) {
  gps_time = *(msg_gps_time_t *)msg;
}

/*
 * Set up SwiftNav Binary Protocol (SBP) nodes; the sbp_process function will
 * search through these to find the callback for a particular message ID.
 *
 * Example: sbp_pos_llh_callback is registered with sbp_state, and is associated
 * with both a unique sbp_msg_callbacks_node_t and the message ID SBP_POS_LLH.
 * When a valid SBP message with the ID SBP_POS_LLH comes through the UART,
 * written to the FIFO, and then parsed by sbp_process, sbp_pos_llh_callback is
 * called with the data carried by that message.
 */
void sbp_setup(void) {
  /* SBP parser state must be initialized before sbp_process is called. */
  MARK;
  sbp_state_init(&sbp_state);
  MARK;

  /* Register a node and callback, and associate them with a specific message
   * ID. */
  sbp_register_callback(&sbp_state, SBP_MSG_GPS_TIME, &sbp_gps_time_callback,
                        NULL, &gps_time_node);
  sbp_register_callback(&sbp_state, SBP_MSG_POS_LLH, &sbp_pos_llh_callback,
                        NULL, &pos_llh_node);
  sbp_register_callback(&sbp_state, SBP_MSG_BASELINE_NED,
                        &sbp_baseline_ned_callback, NULL, &baseline_ned_node);
  sbp_register_callback(&sbp_state, SBP_MSG_VEL_NED, &sbp_vel_ned_callback,
                        NULL, &vel_ned_node);
  sbp_register_callback(&sbp_state, SBP_MSG_DOPS, &sbp_dops_callback, NULL,
                        &dops_node);
}

/*
 * FIFO to hold received UART bytes before libsbp parses them.
 */
#define FIFO_LEN 512
char sbp_msg_fifo[FIFO_LEN];
u16 head = 0;
u16 tail = 0;

/* Return 1 if true, 0 otherwise. */
u8 fifo_empty(void) {
  if (head == tail)
    return 1;
  return 0;
}

/*
 * Append a character to our SBP message fifo.
 * Returns 1 if char successfully appended to fifo.
 * Returns 0 if fifo is full.
 */
u8 fifo_write(char c) {
  if (fifo_full())
    return 0;

  sbp_msg_fifo[tail] = c;
  tail = (tail + 1) % FIFO_LEN;
  return 1;
}

/*
 * Read 1 char from fifo.
 * Returns 0 if fifo is empty, otherwise 1.
 */
u8 fifo_read_char(char *c) {
  if (fifo_empty())
    return 0;

  *c = sbp_msg_fifo[head];
  head = (head + 1) % FIFO_LEN;
  return 1;
}

/*
 * Read arbitrary number of chars from FIFO. Must conform to
 * function definition that is passed to the function
 * sbp_process().
 * Returns the number of characters successfully read.
 */
u32 fifo_read(u8 *buff, u32 n, void *context) {
  int i;
  for (i = 0; i < n; i++)
    if (!fifo_read_char((char *)(buff + i)))
      break;
  return i;
}

/* Return 1 if true, 0 otherwise. */
u8 fifo_full(void) {
  if (((tail + 1) % FIFO_LEN) == head) {
    return 1;
  }
  return 0;
}

// GNSSController method definitions
GNSSController::GNSSController(HardwareSerial &serial, uint32_t baud,
                               uint8_t rx, uint8_t tx, int logFreq)
    : m_serial(serial), m_baud(baud), m_rx(rx), m_tx(tx), m_logFreq(logFreq),
      m_FORMAT("<RTK Mode>,<Week>,<Seconds>,"
               "<Latitude>,<Longitude,<Height>,<Satellites>,<GDOP>,<HDOP>"
               ",<PDOP>,<TDOP>,<VDOP>"), m_timer(), m_convergenceTime(0), m_pollCycles(0) {}

bool GNSSController::init() {
  m_serial.begin(m_baud);
  pinPeripheral(m_tx, PIO_SERCOM);
  pinPeripheral(m_rx, PIO_SERCOM);
  MARK;
  sbp_setup();
  console.debug("GNSSController initialized.\n");
  return true;
}

// determine the fix mode
void GNSSController::m_getModeStr(msg_pos_llh_t pos_llh, char rj[]) {
  switch (m_getMode()) {
  case 0:
    sprintf(rj, "Invalid");
    break;
  case 1:
    sprintf(rj, "SPP");
    break;
  case 2:
    sprintf(rj, "DGNSS");
    break;
  case 3:
    sprintf(rj, "Float RTK");
    break;
  case 4:
    sprintf(rj, "Fixed RTK");
    break;
  case 5:
    sprintf(rj, "Dead Reckoning");
    break;
  case 6:
    sprintf(rj, "SBAS");
    break;
  }
}

uint8_t GNSSController::m_getMode() { return pos_llh.flags & FIX_MODE_MASK; }

void GNSSController::m_GNSSread() {
  if (m_serial.available())
    fifo_write(m_serial.read());
}

// TODO preemptively terminate the polling cycle if RTK is reached early
void GNSSController::m_isFixed(uint8_t &flag) { flag = 1; }

// TODO this function needs to be tuned using real data and weighted averages.
bool GNSSController::m_compare() {
  // first check if the fix mode is better
  if ((m_mode > m_getMode()) && (m_getMode() != 6))
    return false;



  return true;
}

void GNSSController::m_setBest() {
  m_pos_llh = pos_llh;
  m_baseline_ned = baseline_ned;
  m_vel_ned = vel_ned;
  m_dops = dops;
  m_gps_time = gps_time;
  m_mode = m_getMode();
}

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
uint8_t GNSSController::poll(SSModel &model) {
  m_GNSSread();
  uint8_t datFlag = 0;
  s8 ret = sbp_process(&sbp_state, fifo_read);

  if (ret < 0)
    printf("sbp_process error: %d\n", (int)ret);

  DO_EVERY(m_logFreq, MARK;
           // check if the current reading is better than the running best
           if (m_compare()) 
            m_setBest();

            // log the current reading from Swift
            model.setPos_llh(pos_llh); 
            model.setBaseline_ned(baseline_ned);
            model.setMsg_vel_ned_t(vel_ned); 
            model.setMsg_dops_t(dops);
            model.setMsg_gps_time_t(gps_time); 
            model.setMode(m_getMode());
            model.setProp(LOG_FREQ, m_logFreq);

            // check if a fix occured on this wake cycle
            // calculate the new running average for fix convergence
            
            if(m_getMode() == 4 && !m_convFlag){
              m_pollCycles++;
              m_convergenceTime *= (m_pollCycles - 1);

              m_convFlag = true; 
              //m_convergenceTime = ((m_convergenceTime + m_timer.stopwatch()) / float(m_pollCycles));
              m_convergenceTime += m_timer.stopwatch();
              m_convergenceTime /= m_pollCycles;
            }
            

            // check if we acheived an RTK fix, reset internal variables
            m_isFixed(datFlag); 
            m_reset();
          );

  return datFlag;
}

void GNSSController::m_reset() {
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

char *GNSSController::getFormat() { return (char *)m_FORMAT; }

void GNSSController::m_setLogFreq(int logFreq) { m_logFreq = logFreq; }

void GNSSController::status(SSModel &model) {
  model.setPos_llh(m_pos_llh);
  model.setBaseline_ned(m_baseline_ned);
  model.setMsg_vel_ned_t(m_vel_ned);
  model.setMsg_dops_t(m_dops);
  model.setMsg_gps_time_t(m_gps_time);
  model.setMode(m_mode);
  model.setProp(LOG_FREQ, m_logFreq);
  model.setConv(m_convergenceTime);
}

void GNSSController::update(SSModel &model) {
  m_setLogFreq(model.getProp(LOG_FREQ));
}

// clear any serial data in the pipe and reinit best
void GNSSController::reset() {
  while (m_serial.available())
    uint8_t c = m_serial.read();
  m_reset();
  m_setBest();
  m_convFlag = false; 
  m_timer.stopTimer();
}

void GNSSController::setup(){
  m_timer.startStopwatch();
}

// #define MAX_POS_STR 200
// #define MAX_POS_FIELD 30

// char rj[MAX_POS_FIELD];
// char str[MAX_POS_STR];
// int str_i;

// doc["type"] = m_HEADER;
// JsonArray data = doc.createNestedArray("data");
// data.add((int)gps_time.wn);
// data.add(((float)gps_time.tow) / 1e3);
// data.add(m_getMode());
// data.add(pos_llh.lat);
// data.add(pos_llh.lon);
// data.add(pos_llh.height);
// data.add(pos_llh.n_sats);
// data.add((int)baseline_ned.n);
// data.add((int)baseline_ned.e);
// data.add((int)baseline_ned.d);
// data.add((int)vel_ned.n);
// data.add((int)vel_ned.e);
// data.add((int)vel_ned.d);
// data.add(((float)dops.gdop / 100));
// data.add(((float)dops.hdop / 100));
// data.add(((float)dops.pdop / 100));
// data.add(((float)dops.tdop / 100));
// data.add(((float)dops.vdop / 100));

// doc["type"] = m_HEADER;
// doc["GPS.wn"] = (int)gps_time.wn;
// doc["GPS.tow"] = (int)gps_time.tow / 1e3;
// doc["Mode"] = m_getMode();
// doc["lat"] = pos_llh.lat;
// doc["lon"] = pos_llh.lon;
// doc["height"] = pos_llh.height;
// doc["sats"] = pos_llh.n_sats;
// doc["baseline_n"] = (int)baseline_ned.n;
// doc["baseline_e"] = (int)baseline_ned.e;
// doc["baseline_d"] = (int)baseline_ned.d;
// doc["vel_n"] = (int)vel_ned.n;
// doc["vel_e"] = (int)vel_ned.e;
// doc["vel_d"] = (int)vel_ned.d;
// doc["dops.gdop"] = ((float)dops.gdop / 100);
// doc["dops.hdop"] = ((float)dops.hdop / 100);
// doc["dops.pdop"] = ((float)dops.pdop / 100);
// doc["dops.tdop"] = ((float)dops.tdop / 100);
// doc["dops.vdop"] = ((float)dops.vdop / 100);
// serializeJsonPretty(doc, terminal);
// terminal.println();

// 10000, str_i = 0; memset(str, 0, sizeof(str));
// str_i += sprintf(str + str_i, "\n\n\n\n");
// str_i += sprintf(str + str_i, "GPS Time:\n");
// str_i += sprintf(str + str_i, "\tWeek\t\t: %6d\n",(int)gps_time.wn);
// sprintf(rj, "%6.2f", ((float)gps_time.tow) / 1e3);
// str_i += sprintf(str + str_i, "\tSeconds\t: %9s\n", rj);
// str_i += sprintf(str + str_i, "\n");
// str_i += sprintf(str + str_i, "Absolute Position:\n");
// sprintf(rj, "%4.10lf", pos_llh.lat);
// str_i += sprintf(str + str_i, "\tLatitude\t: %17s\n", rj);
// sprintf(rj, "%4.10lf", pos_llh.lon);
// str_i += sprintf(str + str_i, "\tLongitude\t: %17s\n", rj);
// sprintf(rj, "%4.10lf", pos_llh.height);
// str_i += sprintf(str + str_i, "\tHeight\t: %17s\n", rj);
// str_i +=
// sprintf(str + str_i, "\tSatellites\t:     %02d\n", pos_llh.n_sats);
// m_getModeStr(pos_llh, rj);
// str_i += sprintf(str + str_i, "\tFix Mode\t: %17s\n", rj);
// str_i += sprintf(str + str_i, "\n");
// str_i += sprintf(str + str_i, "Baseline (mm):\n");
// str_i += sprintf(str + str_i, "\tNorth\t\t: %6d\n",
// (int)baseline_ned.n); str_i += sprintf(str + str_i, "\tEast\t\t:
// %6d\n", (int)baseline_ned.e); str_i += sprintf(str + str_i,
// "\tDown\t\t: %6d\n", (int)baseline_ned.d); str_i += sprintf(str +
// str_i, "\n");
// str_i += sprintf(str + str_i, "Velocity (mm/s):\n");
// str_i += sprintf(str + str_i, "\tNorth\t\t: %6d\n", (int)vel_ned.n);
// str_i += sprintf(str + str_i, "\tEast\t\t: %6d\n", (int)vel_ned.e);
// str_i += sprintf(str + str_i, "\tDown\t\t: %6d\n", (int)vel_ned.d);
// str_i += sprintf(str + str_i, "\n");
// str_i += sprintf(str + str_i, "Dilution of Precision:\n");
// sprintf(rj, "%4.2f", ((float)dops.gdop / 100));
// str_i += sprintf(str + str_i, "\tGDOP\t\t: %7s\n", rj);
// sprintf(rj, "%4.2f", ((float)dops.hdop / 100));
// str_i += sprintf(str + str_i, "\tHDOP\t\t: %7s\n", rj);
// sprintf(rj, "%4.2f", ((float)dops.pdop / 100));
// str_i += sprintf(str + str_i, "\tPDOP\t\t: %7s\n", rj);
// sprintf(rj, "%4.2f", ((float)dops.tdop / 100));
// str_i += sprintf(str + str_i, "\tTDOP\t\t: %7s\n", rj);
// sprintf(rj, "%4.2f", ((float)dops.vdop / 100));
// str_i += sprintf(str + str_i, "\tVDOP\t\t: %7s\n", rj);
// str_i += sprintf(str + str_i, "\n"); terminal.println(str);

//  // create data packet for writing to SD
//  memset(str, '\0', sizeof(char) * MAX_POS_STR);
//  memset(rj, '\0', sizeof(char) * MAX_POS_FIELD);
//  str_i = 0;
//  doc["ID"] = m_HEADER;
//  str_i += sprintf(str + str_i, "%6d,", (int)gps_time.wn);
//  sprintf(rj, "%6.2f", ((float)gps_time.tow) / 1e3);
//  str_i += sprintf(str + str_i, "%9s,", rj);
//  m_getModeStr(pos_llh, rj);
//  str_i += sprintf(str + str_i, "%17s,", rj);
//  sprintf(rj, "%4.10lf", pos_llh.lat);
//  str_i += sprintf(str + str_i, "%17s,", rj);
//  sprintf(rj, "%4.10lf", pos_llh.lon);
//  str_i += sprintf(str + str_i, "%17s,", rj);
//  sprintf(rj, "%4.10lf", pos_llh.height);
//  str_i += sprintf(str + str_i, "%17s,", rj);
//  str_i += sprintf(str + str_i, "%04d,", pos_llh.n_sats);
//  str_i += sprintf(str + str_i, "%6d,", (int)baseline_ned.n);
//  str_i += sprintf(str + str_i, "%6d,", (int)baseline_ned.e);
//  str_i += sprintf(str + str_i, "%6d,", (int)baseline_ned.d);
//  str_i += sprintf(str + str_i, "%6d,", (int)vel_ned.n);
//  str_i += sprintf(str + str_i, "%6d,", (int)vel_ned.e);
//  str_i += sprintf(str + str_i, "%6d,", (int)vel_ned.d);
//  sprintf(rj, "%4.2f", ((float)dops.gdop / 100));
//  str_i += sprintf(str + str_i, "%7s,", rj);
//  sprintf(rj, "%4.2f", ((float)dops.hdop / 100));
//  str_i += sprintf(str + str_i, "%7s,", rj);
//  sprintf(rj, "%4.2f", ((float)dops.pdop / 100));
//  str_i += sprintf(str + str_i, "%7s,", rj);
//  sprintf(rj, "%4.2f", ((float)dops.tdop / 100));
//  str_i += sprintf(str + str_i, "%7s,", rj);
//  sprintf(rj, "%4.2f", ((float)dops.vdop / 100));
//  str_i += sprintf(str + str_i, "%7s", rj);
//  doc["MSG"] = str;

//  // test data
//  msg_pos_llh_t test_pos_llh; test_pos_llh.lat = 123.45678910;
//  test_pos_llh.lon = 89.12345678; test_pos_llh.height = 12.34;
//  test_pos_llh.n_sats = 10;

//  msg_baseline_ned_t test_baseline_ned; test_baseline_ned.n = 10;
//  test_baseline_ned.e = 11; test_baseline_ned.d = 12;

//  msg_vel_ned_t test_vel_ned; test_vel_ned.n = 13; test_vel_ned.e = 14;
//  test_vel_ned.d = 15;

//  msg_dops_t test_dops; test_dops.gdop = 1.1; test_dops.hdop = 1.2;
//  test_dops.pdop = 1.3; test_dops.tdop = 1.4; test_dops.vdop = 1.5;

//  msg_gps_time_t test_gps_time; test_gps_time.wn = 99;
//  test_gps_time.tow = 88;

//  uint8_t test_mode = 4;

//  model.setPos_llh(test_pos_llh);
//  model.setBaseline_ned(test_baseline_ned);
//  model.setMsg_vel_ned_t(test_vel_ned);
//  model.setMsg_dops_t(test_dops);
//  model.setMsg_gps_time_t(test_gps_time);
//  model.setMode(test_mode);
//  model.setProp(LOG_FREQ, m_logFreq);