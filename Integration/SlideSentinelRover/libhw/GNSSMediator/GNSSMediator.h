
#ifndef _GNSSMediator_H_
#define _GNSSMediator_H_
#include <Arduino.h>
#include "SwiftPiksi.h"

class GNSSMediator {

private:

    /* Variables for additional Serial Interface */
    HardwareSerial &m_serial;
    int m_baud;
    uint8_t m_rx;
    uint8_t m_tx;

    /* State of the SBP message parser.*/
    sbp_state_t m_sbp_state;

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

    GNSSMediator();
    poll();

};

#endif  _GNSSMediator_H_