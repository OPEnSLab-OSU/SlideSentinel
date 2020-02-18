#ifndef SWIFTPIKSI_H
#define SWIFTPIKSI_H

#include "common.h"
#include "navigation.h"
#include "sbp.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#define DO_EVERY(n, cmd)                                                       \
  do {                                                                         \
    static u32 do_every_count = 0;                                             \
    if (do_every_count % (n) == 0) {                                           \
      cmd;                                                                     \
    }                                                                          \
    do_every_count++;                                                          \
  } while (0)

/* FIFO functions */
u8 fifo_empty(void);
u8 fifo_full(void);
u8 fifo_write(char c);
u8 fifo_read_char(char *c);
u32 fifo_read(u8 *buff, u32 n, void *context);

void sbp_pos_llh_callback(u16 sender_id, u8 len, u8 msg[], void *context);
void sbp_baseline_ned_callback(u16 sender_id, u8 len, u8 msg[], void *context);
void sbp_vel_ned_callback(u16 sender_id, u8 len, u8 msg[], void *context);
void sbp_dops_callback(u16 sender_id, u8 len, u8 msg[], void *context);
void sbp_gps_time_callback(u16 sender_id, u8 len, u8 msg[], void *context);
void sbp_setup(void);

#endif