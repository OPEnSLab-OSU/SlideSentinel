#pragma once

#ifdef UNIT_TEST

#include "unity.h"
#include <typeinfo>
#include <typeindex>

#define TEST_ASSERT_SAME_TYPE(expected_type, actual_type_index) \
    if (!(std::type_index(typeid(expected_type)) == actual_type_index))     \
        TEST_ASSERT_EQUAL_STRING(typeid(expected_type).name(), actual_type_index.name())

#endif