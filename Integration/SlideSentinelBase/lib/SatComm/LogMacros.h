//
// Created by Noah on 4/23/2020.
//

#ifndef SLIDESENTINELROVER_PANICHANDLER_H
#define SLIDESENTINELROVER_PANICHANDLER_H

#ifdef UNIT_TEST
#include "unity.h"

#define MSG_PANIC(msg) TEST_FAIL_MESSAGE(msg)
#define MSG_WARN(msg) TEST_MESSAGE(msg)
#define MSG_INFO(msg) TEST_MESSAGE(msg)

#define TRANSIT(state_from, state_to) ({ const char* msg = #state_from " -> " #state_to; MSG_INFO(msg); state_from::template transit<state_to>(); })

#else // UNIT_TEST

#include <Plog.h>
#define MSG_PANIC(msg) ({ LOGF << "PANIC: " << msg; })
#define MSG_WARN(msg) ({ LOGW << msg; })
#define MSG_INFO(msg) ({ LOGD << msg; })

#define TRANSIT(state_from, state_to) ({ LOGD << #state_from << " -> " << #state_to; state_from::template transit<state_to>(); })

#endif // UNIT_TEST
#endif //SLIDESENTINELROVER_PANICHANDLER_H
