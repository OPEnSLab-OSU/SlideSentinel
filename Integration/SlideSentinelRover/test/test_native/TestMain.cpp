#if defined(UNIT_TEST) && !defined(ARDUINO)

#include "unity.h"
#include "SampleTest.h"

void process() {
    UNITY_BEGIN();
    RUN_TEST(TestTesting);
    UNITY_END();
}

int main(int argc, char **argv) {
    process();
    return 0;
}

#endif