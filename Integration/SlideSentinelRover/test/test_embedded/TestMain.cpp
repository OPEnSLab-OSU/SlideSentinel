#if defined(UNIT_TEST) && defined(ARDUINO)

#include "unity.h"
#include "SampleTest.h"
#include "TestSatCommDriver.h"

void process() {
    UNITY_BEGIN();
    RUN_TEST(TestTesting);
    UNITY_END();
}

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

#endif