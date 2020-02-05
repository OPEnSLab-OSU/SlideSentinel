#ifndef SWIFTPIKSI_H
#define SWIFTPIKSI_H

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "sbp.h"
#include "navigation.h"
#include "Tutorial.h"

void sbp_pos_llh_callback(u16 sender_id, u8 len, u8 msg[], void *context);
void sbp_baseline_ned_callback(u16 sender_id, u8 len, u8 msg[], void *context);
void sbp_vel_ned_callback(u16 sender_id, u8 len, u8 msg[], void *context);
void sbp_dops_callback(u16 sender_id, u8 len, u8 msg[], void *context);
void sbp_gps_time_callback(u16 sender_id, u8 len, u8 msg[], void *context);
void sbp_setup(void);
uint8_t getFixMode(msg_pos_llh_t pos_llh, char rj[]);
void poll(Serial_ & terminal);

#endif