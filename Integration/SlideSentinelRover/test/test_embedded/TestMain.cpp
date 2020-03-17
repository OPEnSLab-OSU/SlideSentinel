#include "unity.h"

#if defined(UNIT_TEST) && defined(ARDUINO)

void setUp() {
    // ?
}

void tearDown() {
    // ?
}

void process() {
    UNITY_BEGIN();
    // Register your tests here
    // RUN_TEST(test_dosomething);
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