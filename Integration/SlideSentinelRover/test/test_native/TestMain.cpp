#if defined(UNIT_TEST) && !defined(ARDUINO)

#include "unity.h"
#include "SampleTest.h"
#include "TestSatComm.h"

void process() {
    UNITY_BEGIN();
    // Status
    RUN_TEST(TestTesting);
    // SatComm
    RUN_TEST(TestNoSignal);
    RUN_TEST(TestNoPackets);
    RUN_TEST(TestTransmitNoRx);
    RUN_TEST(TestTransmitNoRxBadSignal);
    RUN_TEST(TestTransmitRx);
    RUN_TEST(TestTransmitRxBadSignal);
    RUN_TEST(TestRingAlert);
    UNITY_END();
}

int main(int argc, char **argv) {
    process();
    return 0;
}

#endif