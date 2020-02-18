#include "SwiftPiksi.h"

char rj[30];
char str[1000];
int str_i;

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
  sbp_state_init(&sbp_state);

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

uint8_t getFixMode(msg_pos_llh_t pos_llh, char rj[]) {
  uint8_t mode = pos_llh.flags & 0b0000111;
  switch (mode) {
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
  return mode;
}

void poll(Serial_ &terminal) {
  s8 ret = sbp_process(&sbp_state, &fifo_read);

  if (ret < 0)
    printf("sbp_process error: %d\n", (int)ret);

  DO_EVERY(
      10000, str_i = 0; memset(str, 0, sizeof(str));

      str_i += sprintf(str + str_i, "\n\n\n\n");

      str_i += sprintf(str + str_i, "GPS Time:\n");
      str_i += sprintf(str + str_i, "\tWeek\t\t: %6d\n", (int)gps_time.wn);
      sprintf(rj, "%6.2f", ((float)gps_time.tow) / 1e3);
      str_i += sprintf(str + str_i, "\tSeconds\t: %9s\n", rj);
      str_i += sprintf(str + str_i, "\n");

      str_i += sprintf(str + str_i, "Absolute Position:\n");
      sprintf(rj, "%4.10lf", pos_llh.lat);
      str_i += sprintf(str + str_i, "\tLatitude\t: %17s\n", rj);
      sprintf(rj, "%4.10lf", pos_llh.lon);
      str_i += sprintf(str + str_i, "\tLongitude\t: %17s\n", rj);
      sprintf(rj, "%4.10lf", pos_llh.height);
      str_i += sprintf(str + str_i, "\tHeight\t: %17s\n", rj);
      str_i +=
      sprintf(str + str_i, "\tSatellites\t:     %02d\n", pos_llh.n_sats);

      /* -------------- RTK --------------
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
       */

      getFixMode(pos_llh, rj);
      str_i += sprintf(str + str_i, "\tFix Mode\t: %17s\n", rj);
      str_i += sprintf(str + str_i, "\n");

      str_i += sprintf(str + str_i, "Baseline (mm):\n");
      str_i += sprintf(str + str_i, "\tNorth\t\t: %6d\n", (int)baseline_ned.n);
      str_i += sprintf(str + str_i, "\tEast\t\t: %6d\n", (int)baseline_ned.e);
      str_i += sprintf(str + str_i, "\tDown\t\t: %6d\n", (int)baseline_ned.d);
      str_i += sprintf(str + str_i, "\n");

      str_i += sprintf(str + str_i, "Velocity (mm/s):\n");
      str_i += sprintf(str + str_i, "\tNorth\t\t: %6d\n", (int)vel_ned.n);
      str_i += sprintf(str + str_i, "\tEast\t\t: %6d\n", (int)vel_ned.e);
      str_i += sprintf(str + str_i, "\tDown\t\t: %6d\n", (int)vel_ned.d);
      str_i += sprintf(str + str_i, "\n");

      str_i += sprintf(str + str_i, "Dilution of Precision:\n");
      sprintf(rj, "%4.2f", ((float)dops.gdop / 100));
      str_i += sprintf(str + str_i, "\tGDOP\t\t: %7s\n", rj);
      sprintf(rj, "%4.2f", ((float)dops.hdop / 100));
      str_i += sprintf(str + str_i, "\tHDOP\t\t: %7s\n", rj);
      sprintf(rj, "%4.2f", ((float)dops.pdop / 100));
      str_i += sprintf(str + str_i, "\tPDOP\t\t: %7s\n", rj);
      sprintf(rj, "%4.2f", ((float)dops.tdop / 100));
      str_i += sprintf(str + str_i, "\tTDOP\t\t: %7s\n", rj);
      sprintf(rj, "%4.2f", ((float)dops.vdop / 100));
      str_i += sprintf(str + str_i, "\tVDOP\t\t: %7s\n", rj);
      str_i += sprintf(str + str_i, "\n"); terminal.println(str););
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
