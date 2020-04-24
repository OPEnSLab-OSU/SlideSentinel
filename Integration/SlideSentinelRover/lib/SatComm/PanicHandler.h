//
// Created by Noah on 4/23/2020.
//

#ifndef SLIDESENTINELROVER_PANICHANDLER_H
#define SLIDESENTINELROVER_PANICHANDLER_H

#ifdef UNIT_TEST
#include "unity.h"

#define PANIC(msg) \
    TEST_FAIL_MESSAGE(msg)

#else // UNIT_TEST

#define PANIC(msg) { Serial.print("PANIC: "); Serial.println(msg); }

#endif // UNIT_TEST
#endif //SLIDESENTINELROVER_PANICHANDLER_H
