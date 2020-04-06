#pragma once

#ifdef UNIT_TEST

#include "unity.h"
#include <typeinfo>

#define TEST_ASSERT_SAME_TYPE(expected, actual) TEST_ASSERT_EQUAL_STRING(typeid(expected).name(), typeid(actual).name())

#endif