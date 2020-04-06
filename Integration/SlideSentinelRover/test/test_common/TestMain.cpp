#include "unity.h"
#include "TestTinyFSM.h"
#include "TestCircularBuffer.h"
#include "TestSatComm.h"

#ifdef UNIT_TEST

void process() {
    UNITY_BEGIN();
    // Register your tests here
    // TinyFSM
    RUN_TEST(TestInitial);
    RUN_TEST(TestReset);
    RUN_TEST(TestToggle);
    RUN_TEST(TestInvalid);
    // Circular Buffer
    RUN_TEST(TestInsertion);
    RUN_TEST(TestIteration);
    RUN_TEST(TestRemove);
    RUN_TEST(TestFront);
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

#ifdef ARDUINO

#include <Arduino.h>
void setup() {
    Serial.begin(115200);
    while(!Serial)
        yield();

    process();
}

void loop() {
    digitalWrite(13, HIGH);
    delay(100);
    digitalWrite(13, LOW);
    delay(500);
}

#else // ARDUINO

int main(int argc, char **argv) {
    process();
    return 0;
}

#endif // ARDUINO

#endif // UNIT_TEST