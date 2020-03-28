#pragma once

#if !defined(ARDUINO) && defined(UNIT_TEST)

#include "unity.h"

// write tests here
// tests must be registered in TestMain.cpp

void TestTesting() {
    TEST_ASSERT_TRUE(true);
}

#endif