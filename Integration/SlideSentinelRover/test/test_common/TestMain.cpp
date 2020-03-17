#include "unity.h"
#include "TestTinyFSM.h"

#ifdef UNIT_TEST

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