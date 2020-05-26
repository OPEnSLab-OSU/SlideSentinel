//
// Created by Noah on 4/23/2020.
//

#ifndef SLIDESENTINELROVER_PANICHANDLER_H
#define SLIDESENTINELROVER_PANICHANDLER_H

#ifdef UNIT_TEST
#include "unity.h"
#include <cstdio>
#include <typeinfo>

#define MSG_PANIC(msg) TEST_FAIL_MESSAGE(msg)
#define MSG_WARN(msg) TEST_MESSAGE(msg)
#define MSG_INFO(msg) TEST_MESSAGE(msg)

#define TRANSIT(_class, _state) ({ char buf[256]; snprintf(buf, sizeof buf, "%s -> %s", typeid(_class).name(), typeid(_state).name()); MSG_INFO(buf); _class::template transit<_state>(); })

#else // UNIT_TEST

#include <Plog.h>
#define MSG_PANIC(msg) ({ LOGF << "PANIC: " << msg; })
#define MSG_WARN(msg) ({ LOGW << msg; })
#define MSG_INFO(msg) ({ LOGD << msg; })

#define TRANSIT(_class, _state) ({ LOGD << typeid(_class).name() << "->" << typeid(_state).name(); _class::template transit<_state>(); })

#endif // UNIT_TEST
#endif //SLIDESENTINELROVER_PANICHANDLER_H
