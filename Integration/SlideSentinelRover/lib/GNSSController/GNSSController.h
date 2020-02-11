#ifndef _GNSSCONTROLLER_H_
#define _GNSSCONTROLLER_H_

#include "ArduinoJson.h"
#include "HardwareSerial.h"
#include "common.h"
#include "navigation.h"
#include "sbp.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#define FIFO_LEN 512
#define DO_EVERY(n, cmd)                                                       \
  do {                                                                         \
    static u32 do_every_count = 0;                                             \
    if (do_every_count % (n) == 0) {                                           \
      cmd;                                                                     \
    }                                                                          \
    do_every_count++;                                                          \
  } while (0)

class GNSSController {

private:
  HardwareSerial *m_serial; // Postional data

  u8 fifo_empty(void);
  u8 fifo_full(void);
  u8 fifo_write(char c);
  u8 fifo_read_char(char *c);
  u32 fifo_read(u8 *buff, u32 n, void *context);
  void GNSSread();

  void sbp_pos_llh_callback(u16 sender_id, u8 len, u8 msg[], void *context);
  void sbp_baseline_ned_callback(u16 sender_id, u8 len, u8 msg[],
                                 void *context);
  void sbp_vel_ned_callback(u16 sender_id, u8 len, u8 msg[], void *context);
  void sbp_dops_callback(u16 sender_id, u8 len, u8 msg[], void *context);
  void sbp_gps_time_callback(u16 sender_id, u8 len, u8 msg[], void *context);
  void sbp_setup(void);

  uint8_t getFixMode(msg_pos_llh_t pos_llh, char rj[]);

  // Printing Data
  char rj[30];
  char str[1000];
  int str_i;

  // FIFO Data
  u16 head = 0;
  u16 tail = 0;
  char sbp_msg_fifo[FIFO_LEN];

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

public:
  GNSSController(HardwareSerial *serial, uint32_t baud);
  void sbp_setup(void);
  bool poll(Serial_ &terminal, JsonDocument &doc);
};

#endif // _GNSSCONTROLLER_H_
