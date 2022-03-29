#include "GNSSMediator.h"


GNSSMediator::GNSSMediator(){
    /* Configure additional Serial Interface */
    m_serial.begin(m_baud);
    pinPeripheral(m_tx, PIO_SERCOM);
    pinPeripheral(m_rx, PIO_SERCOM);

    sbp_setup();
}

GNSSMediator::poll(){
    
}

void sbp_setup(){
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