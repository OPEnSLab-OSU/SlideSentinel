#pragma once

#include "common.h"
#include "navigation.h"
#include "sbp.h"
#include "FeatherTrace.h"

/**
 * Manages Swift Navigation functions
 */ 

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
};
void sbp_baseline_ned_callback(u16 sender_id, u8 len, u8 msg[], void *context) {
  baseline_ned = *(msg_baseline_ned_t *)msg;
};
void sbp_vel_ned_callback(u16 sender_id, u8 len, u8 msg[], void *context) {
  vel_ned = *(msg_vel_ned_t *)msg;
};
void sbp_dops_callback(u16 sender_id, u8 len, u8 msg[], void *context) {
  dops = *(msg_dops_t *)msg;
};
void sbp_gps_time_callback(u16 sender_id, u8 len, u8 msg[], void *context) {
  gps_time = *(msg_gps_time_t *)msg;
};

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
};

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
};

/* Return 1 if true, 0 otherwise. */
u8 fifo_full(void) {
  if (((tail + 1) % FIFO_LEN) == head) {
    return 1;
  }
  return 0;
};

/*
 * Append a character to our SBP message fifo.
 * Returns 1 if char successfully appended to fifo.
 * Returns 0 if fifo is full.
 */
u8 fifo_write(char c) {
  // if it's already full, do nothing
  if (fifo_full())
    return 0;

  // otherwise: fill the tail with a char and append it
  sbp_msg_fifo[tail] = c;
  tail = (tail + 1) % FIFO_LEN;
  return 1;
};

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
};

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
};
